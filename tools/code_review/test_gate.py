"""End-to-end checks of the review gate using real clang-tidy diagnostics."""

import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import unittest


TOOLS = Path(__file__).resolve().parent


class GateTests(unittest.TestCase):
    def test_real_diagnostics_and_failed_scans(self):
        if not all(shutil.which(tool) for tool in ("git", "clang-tidy", "c++")):
            self.skipTest("git, clang-tidy and a C++ compiler are required")
        version = subprocess.run(["clang-tidy", "--version"], capture_output=True, text=True, check=True)
        if "18.1.3" not in version.stdout:
            self.skipTest("the gate requires clang-tidy 18.1.3")
        with tempfile.TemporaryDirectory(prefix="descent3-review-gate-") as directory:
            root = Path(directory)
            scripts = root / "tools/code_review"
            docs = root / "docs/code-review"
            build = root / "build"
            source = root / "networking/seed.cpp"
            for path in (scripts, docs, build, source.parent):
                path.mkdir(parents=True, exist_ok=True)
            for name in ("scan.py", "compare.py"):
                shutil.copy2(TOOLS / name, scripts / name)
            (docs / "progress.json").write_text(json.dumps({"scan": {
                "files": ["networking/seed.cpp"],
                "checks": "-*,clang-analyzer-core.NullDereference",
                "commit": "unused-with-empty-baseline",
            }}))
            (docs / "diagnostics.json").write_text("[]")
            (build / "compile_commands.json").write_text(json.dumps([{
                "directory": str(root), "file": str(source),
                "arguments": [shutil.which("c++"), '-DCMAKE_INTDIR="Debug"', "-std=c++17", "-c", str(source)],
            }]))

            def run(script, *args):
                return subprocess.run([sys.executable, str(scripts / script), *args],
                                      cwd=root, capture_output=True, text=True)

            source.write_text("int example() { return 1; }\n")
            result = run("scan.py")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = run("compare.py")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

            # A third-party diagnostic in the log does not affect the gate.
            log = build / "code-review/networking__seed.cpp.log"
            with log.open("a") as output:
                output.write(f"{root}/third_party/library.cpp:1:1: warning: test [clang-analyzer-core.NullDereference]\n")
            self.assertEqual(run("compare.py").returncode, 0)

            source.write_text("int example() { int *p = nullptr; return *p; }\n")
            result = run("scan.py")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = run("compare.py")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("clang-analyzer-core.NullDereference", result.stdout)

            # Pin a real diagnostic to a Git baseline, then replace it with an
            # identical message in another function. A count-only gate misses it.
            original = ("int old() {\n int *p = nullptr;\n return *p;\n}\n"
                        "int newer() {\n int *p = nullptr;\n return 1;\n}\n")
            replacement = ("int old() {\n int *p = nullptr;\n return 1;\n}\n"
                           "int newer() {\n int *p = nullptr;\n return *p;\n}\n")
            source.write_text(original)
            self.assertEqual(run("scan.py").returncode, 0)
            warning = re.search(r":(\d+):(\d+): warning: (.*?) \[(clang-analyzer-core.NullDereference)\]",
                                log.read_text())
            self.assertIsNotNone(warning)

            def git(*args):
                return subprocess.run(["git", *args], cwd=root,
                                      capture_output=True, text=True, check=True)

            git("init", "-q")
            git("add", "networking/seed.cpp")
            git("-c", "user.name=Review Test", "-c", "user.email=test@example.invalid", "commit", "-qm", "baseline")
            commit = git("rev-parse", "HEAD").stdout.strip()
            (docs / "progress.json").write_text(json.dumps({"scan": {
                "files": ["networking/seed.cpp"], "checks": "-*,clang-analyzer-core.NullDereference",
                "commit": commit,
            }}))
            (docs / "diagnostics.json").write_text(json.dumps([{
                "file": "networking/seed.cpp", "line": int(warning[1]), "column": int(warning[2]),
                "message": warning[3], "check": warning[4],
            }]))
            self.assertEqual(run("compare.py").returncode, 0)
            source.write_text("// inserted line\n" + original)
            self.assertEqual(run("scan.py").returncode, 0)
            self.assertEqual(run("compare.py").returncode, 0)
            source.write_text(replacement)
            self.assertEqual(run("scan.py").returncode, 0)
            result = run("compare.py")
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("networking/seed.cpp:7:", result.stdout)
            (docs / "progress.json").write_text(json.dumps({"scan": {
                "files": ["networking/seed.cpp"], "checks": "-*,clang-analyzer-core.NullDereference",
                "commit": "missing-baseline-revision",
            }}))
            result = run("compare.py")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Cannot read baseline source", result.stderr)
            (docs / "progress.json").write_text(json.dumps({"scan": {
                "files": ["networking/seed.cpp"], "checks": "-*,clang-analyzer-core.NullDereference",
                "commit": commit,
            }}))

            # Compilation errors and timed-out invocations cannot pass comparison.
            source.write_text("this is not valid C++\n")
            self.assertNotEqual(run("scan.py").returncode, 0)
            self.assertNotEqual(run("compare.py").returncode, 0)
            results_path = build / "code-review/results.json"
            results = json.loads(results_path.read_text())
            results[0]["exit_code"] = "timeout"
            results_path.write_text(json.dumps(results))
            self.assertNotEqual(run("compare.py").returncode, 0)

            # Early failures invalidate the previous results as well.
            (build / "compile_commands.json").unlink()
            self.assertNotEqual(run("scan.py").returncode, 0)
            self.assertFalse(results_path.exists())
            self.assertNotEqual(run("compare.py").returncode, 0)

            # Build directories can also be outside the source tree.
            source.write_text("int example() { return 1; }\n")
            with tempfile.TemporaryDirectory(prefix="descent3-review-build-") as external:
                external_build = Path(external)
                (external_build / "compile_commands.json").write_text(json.dumps([{
                    "directory": str(root), "file": str(source),
                    "arguments": [shutil.which("c++"), '-DCMAKE_INTDIR="Debug"', "-c", str(source)],
                }]))
                result = run("scan.py", "--build-dir", external)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertEqual(run("compare.py", "--build-dir", external).returncode, 0)

                # CMAKE_BUILD_TYPE does not identify an unmarked command in a multi-config build.
                (external_build / "compile_commands.json").write_text(json.dumps([{
                    "directory": str(root), "file": str(source),
                    "arguments": [shutil.which("c++"), "-c", str(source)],
                }]))
                (external_build / "CMakeCache.txt").write_text(
                    "CMAKE_BUILD_TYPE:STRING=Debug\nCMAKE_CONFIGURATION_TYPES:STRING=Debug;Release\n")
                self.assertNotEqual(run("scan.py", "--build-dir", external).returncode, 0)
                self.assertNotEqual(run("compare.py", "--build-dir", external).returncode, 0)


if __name__ == "__main__":
    unittest.main()
