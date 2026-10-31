"""Run the review's selected clang-tidy sample without modifying engine sources."""

import argparse
import concurrent.futures
import json
from pathlib import Path
import re
import subprocess


ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--jobs", type=int, default=3)
    args = parser.parse_args()
    version = subprocess.run(["rtk", "proxy", "clang-tidy", "--version"], capture_output=True, text=True, check=True)
    if not re.search(r"\b18\.1\.3\b", version.stdout):
        raise SystemExit("The review baseline requires clang-tidy 18.1.3")
    baseline = json.loads((ROOT / "docs/code-review/progress.json").read_text())
    scan = baseline["scan"]
    build = (ROOT / args.build_dir).resolve()
    out = build / "code-review"
    out.mkdir(parents=True, exist_ok=True)
    database = json.loads((build / "compile_commands.json").read_text())
    # First entry per source: the original multi-config database selected Debug.
    unique = {entry["file"]: entry for entry in reversed(database)}
    missing = [p for p in scan["files"] if str(ROOT / p) not in unique]
    if missing:
        raise SystemExit("Sources absent from compilation database: " + ", ".join(missing))
    (out / "compile_commands.json").write_text(json.dumps(
        [unique[str(ROOT / p)] for p in scan["files"]], indent=2) + "\n")

    def run(source):
        log = out / (source.replace("/", "__") + ".log")
        command = ["rtk", "proxy", "clang-tidy", source, "-p=" + str(out),
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
        return {"file": source, "exit_code": code, "log": str(log.relative_to(ROOT))}

    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        results = list(pool.map(run, scan["files"]))
    (out / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    print(json.dumps(results, indent=2))
    if any(r["exit_code"] != 0 for r in results):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
