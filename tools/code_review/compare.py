"""Compare completed clang-tidy scan logs with the recorded review baseline."""

import argparse
from collections import Counter
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
WARNING = re.compile(r"^(.*?):\d+:\d+: warning: (.*?) \[([^]]+)\]$")


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

    # Counts preserve repeated identical messages while tolerating line shifts.
    # Deduplicate included-header reports across translation units by location.
    locations = set()
    for result in results:
        log = ROOT / result["log"]
        for line in log.read_text().splitlines():
            match = WARNING.match(line)
            if not match:
                continue
            path, message, check = match.groups()
            if not check.startswith("clang-analyzer-") or check.startswith("clang-analyzer-optin.performance."):
                continue
            try:
                relative = Path(path).resolve().relative_to(ROOT).as_posix()
            except ValueError:
                continue
            if relative.startswith(("build/", "builds/", "third_party/")):
                continue
            locations.add((line, relative, check, message))
    current = Counter((path, check, message) for _, path, check, message in locations)
    baseline = Counter((d["file"], d["check"], d["message"]) for d in json.loads(
        (ROOT / "docs/code-review/diagnostics.json").read_text())
        if d["check"].startswith("clang-analyzer-") and not d["check"].startswith("clang-analyzer-optin.performance."))
    additions = current - baseline
    for (path, check, message), count in sorted(additions.items()):
        print(f"{path}: {check}: {message} ({count} new)")
    print(f"Compared {sum(current.values())} current analyzer findings; {sum(additions.values())} new")
    if additions:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
