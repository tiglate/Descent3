"""Run the reproducible Debug analyzer gate and its regression tests."""

import argparse
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--jobs", type=int, default=3)
    args = parser.parse_args()
    commands = [
        [sys.executable, "-m", "unittest", "discover", "-s", "tools/code_review", "-p", "test_*.py", "-v"],
        [sys.executable, "tools/code_review/scan.py", "--build-dir", args.build_dir, "--jobs", str(args.jobs)],
        [sys.executable, "tools/code_review/compare.py", "--build-dir", args.build_dir],
    ]
    for command in commands:
        print("Running:", " ".join(command), flush=True)
        result = subprocess.run(command, cwd=ROOT)
        if result.returncode:
            return result.returncode
    return 0


if __name__ == "__main__":
    sys.exit(main())
