"""Compare completed clang-tidy scan logs with the recorded review baseline."""

import argparse
from difflib import SequenceMatcher
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
WARNING = re.compile(r"^(.*?):(\d+):(\d+): warning: (.*?) \[([^]]+)\]$")


def unchanged_lines(before, after):
    """Map baseline line numbers to current line numbers for unchanged lines."""
    def matches(first, second):
        matcher = SequenceMatcher(None, first, second, autojunk=False)
        return {i + 1: j + 1 for tag, a, b, c, _ in matcher.get_opcodes() if tag == "equal"
                for i, j in zip(range(a, b), range(c, c + b - a))}

    old_lines, new_lines = before.splitlines(), after.splitlines()
    forward = matches(old_lines, new_lines)
    reverse = matches(new_lines, old_lines)
    # An ambiguous repeated block can be paired with a different occurrence
    # depending on direction. Do not silently accept either pairing.
    return {old: new for old, new in forward.items() if reverse.get(new) == old}


def baseline_locations(diagnostics, default_commit):
    """Map recorded diagnostics through unchanged source lines, failing closed."""
    result = set()
    mappings = {}
    for diagnostic in diagnostics:
        check = diagnostic["check"]
        if not check.startswith("clang-analyzer-") or check.startswith("clang-analyzer-optin.performance."):
            continue
        path = diagnostic["file"]
        commit = diagnostic.get("commit", default_commit)
        key = (commit, path)
        if key not in mappings:
            old = subprocess.run(["rtk", "proxy", "git", "show", f"{commit}:{path}"], cwd=ROOT,
                                 capture_output=True, text=True)
            if old.returncode:
                raise SystemExit(f"Cannot read baseline source {commit}:{path}: {old.stderr.strip()}")
            current = ROOT / path
            mappings[key] = unchanged_lines(old.stdout, current.read_text()) if current.is_file() else {}
        current_line = mappings[key].get(diagnostic["line"])
        if current_line is not None:
            result.add((path, current_line, diagnostic["column"], check, diagnostic["message"]))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", default="build")
    args = parser.parse_args()
    out = (ROOT / args.build_dir).resolve() / "code-review"
    progress = json.loads((ROOT / "docs/code-review/progress.json").read_text())
    results = json.loads((out / "results.json").read_text())
    expected = set(progress["scan"]["files"])
    if len(results) != len(expected) or {r["file"] for r in results} != expected:
        raise SystemExit("Scan results do not cover the recorded source set")
    if any(r["exit_code"] != 0 for r in results):
        raise SystemExit("Scan contains failed or timed-out invocations")

    # Deduplicate included-header reports across translation units by location.
    locations = set()
    for result in results:
        log = ROOT / result["log"]
        for line in log.read_text().splitlines():
            match = WARNING.match(line)
            if not match:
                continue
            path, line_number, column, message, check = match.groups()
            if not check.startswith("clang-analyzer-") or check.startswith("clang-analyzer-optin.performance."):
                continue
            try:
                relative = Path(path).resolve().relative_to(ROOT).as_posix()
            except ValueError:
                continue
            if relative.startswith(("build/", "builds/", "third_party/")):
                continue
            locations.add((relative, int(line_number), int(column), check, message))
    diagnostics = json.loads((ROOT / "docs/code-review/diagnostics.json").read_text())
    supplement = ROOT / "docs/code-review/gate-baseline.json"
    if supplement.exists():
        diagnostics.extend(json.loads(supplement.read_text()))
    baseline = baseline_locations(diagnostics, progress["scan"]["commit"])
    additions = locations - baseline
    for path, line_number, column, check, message in sorted(additions):
        print(f"{path}:{line_number}:{column}: {check}: {message}")
    print(f"Compared {len(locations)} current analyzer findings; {len(additions)} new")
    if additions:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
