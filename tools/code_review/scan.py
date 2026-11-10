"""Run the review's selected clang-tidy sample without modifying engine sources."""

import argparse
import concurrent.futures
import json
from pathlib import Path
import re
import shlex
import subprocess


ROOT = Path(__file__).resolve().parents[2]


def configuration(entry):
    """Read CMake's multi-config marker, regardless of database command format."""
    args = entry.get("arguments") or shlex.split(entry["command"], posix=True)
    for arg in args:
        if arg.startswith("-DCMAKE_INTDIR="):
            return arg.split("=", 1)[1].strip('"')
    return None


def select_entries(database, sources, config, single_config):
    selected = {}
    for entry in database:
        path = Path(entry["file"])
        if not path.is_absolute():
            path = Path(entry["directory"]) / path
        name = str(path.resolve())
        if name not in sources:
            continue
        marker = configuration(entry)
        if marker is None:
            if single_config != config:
                continue
        elif marker != config:
            continue
        selected.setdefault(name, entry)
    return selected


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--jobs", type=int, default=3)
    parser.add_argument("--config", default="Debug", choices=["Debug"],
                        help="configuration of the recorded baseline (currently Debug only)")
    parser.add_argument("--clang-tidy", default="clang-tidy",
                        help="clang-tidy executable (must report version 18.1.3)")
    args = parser.parse_args()
    build = (ROOT / args.build_dir).resolve()
    out = build / "code-review"
    out.mkdir(parents=True, exist_ok=True)
    # A failed or interrupted run must never leave a previous successful result usable.
    (out / "results.json").unlink(missing_ok=True)
    version = subprocess.run([args.clang_tidy, "--version"], capture_output=True, text=True, check=True)
    if not re.search(r"\b18\.1\.3\b", version.stdout):
        raise SystemExit("The review baseline requires clang-tidy 18.1.3")
    baseline = json.loads((ROOT / "docs/code-review/progress.json").read_text())
    scan = baseline["scan"]
    database = json.loads((build / "compile_commands.json").read_text())
    cache = build / "CMakeCache.txt"
    cache_text = cache.read_text() if cache.exists() else ""
    match = re.search(r"^CMAKE_BUILD_TYPE:[^=]*=(.*)$", cache_text, re.MULTILINE)
    multi_config = re.search(r"^CMAKE_CONFIGURATION_TYPES:[^=]*=(.+)$", cache_text, re.MULTILINE)
    single_config = match.group(1).strip() if match and not multi_config else None
    sources = {str(ROOT / p) for p in scan["files"]}
    unique = select_entries(database, sources, args.config, single_config)
    missing = [p for p in scan["files"] if str(ROOT / p) not in unique]
    if missing:
        raise SystemExit(f"Sources absent from {args.config} compilation database: " + ", ".join(missing))
    (out / "compile_commands.json").write_text(json.dumps(
        [unique[str(ROOT / p)] for p in scan["files"]], indent=2) + "\n")

    def run(source):
        log = out / (source.replace("/", "__") + ".log")
        command = [args.clang_tidy, source, "-p=" + str(out),
                   "--checks=" + scan["checks"],
                   "--header-filter=^" + re.escape(str(ROOT))
                   + "/(Descent3|networking|linux|sndlib|cfile|ddio|mem|ui|renderer)/"]
        try:
            result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, timeout=150)
            log.write_text(result.stdout + result.stderr)
            code = result.returncode
        except subprocess.TimeoutExpired as exc:
            log.write_bytes((exc.stdout or b"") + (exc.stderr or b""))
            code = "timeout"
        try:
            log_path = str(log.relative_to(ROOT))
        except ValueError:
            log_path = str(log)
        return {"file": source, "exit_code": code, "log": log_path}

    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        results = list(pool.map(run, scan["files"]))
    (out / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(results, indent=2))
    if any(r["exit_code"] != 0 for r in results):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
