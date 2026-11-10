# Source review and implementation tracking

Open [dashboard.html](dashboard.html) in a browser for charts, the phased roadmap,
and an editable implementation table. No server or network connection is needed.
Read [PLAN.md](PLAN.md) for the review, priorities, evidence, and acceptance criteria.

`progress.json` is the version-controlled source of truth. New implementation
items start as **Backlog**; completed items include validation evidence.
Browser edits are saved locally when storage is available. Use **Export progress**
to save them durably; replace `progress.json` with that export and regenerate:

```sh
rtk proxy python3 tools/code_review/render.py
```

Use **Import progress** to transfer a saved export to another browser. Imports
update progress fields only; finding definitions remain tied to the review.
On reopening an updated dashboard, each item's newer repository/browser edit
wins and both change histories are retained. Explicit imports still apply the
selected export, allowing a deliberate restoration of older progress.
Done requires validation evidence and completed dependencies; Dismissed requires
a written reason. Dismissed items count as resolved, never as implemented.
The charts use item counts, not estimated percentages of effort. The history
records actual edits; no schedule or forecast is invented.

The scan is a risk-based Linux Debug sample, not an exhaustive audit. Raw logs
and the selected compilation database are in ignored `build/code-review/`.
[diagnostics.json](diagnostics.json) preserves deduplicated first-party warning
locations and messages. Header warnings are included; third-party code is excluded.
The scan summary in `progress.json` records coverage and tool settings.

To reproduce the same sample after configuring a build with a compilation database:

```sh
rtk proxy python3 tools/code_review/scan.py --build-dir build
```

That command writes new evidence under `build/code-review/`; it does not overwrite
the committed review baseline or any implementation status. Compare and triage
new diagnostics before deliberately updating the baseline. Source links and line
numbers describe the reviewed commit and may move during implementation.
The baseline covers Debug only: the scan selects Debug entries explicitly from a
multi-config database, or requires a single-config build with `CMAKE_BUILD_TYPE=Debug`.
It removes the previous results before starting, so an interrupted or failed scan
cannot be compared as if it had succeeded. Run the configuration-selection tests
with `rtk proxy python3 -m unittest discover -s tools/code_review -p 'test_*.py'`.

For a preliminary analyzer comparison, run:

```sh
rtk proxy python3 tools/code_review/compare.py --build-dir build
```

For a single local gate command that runs the regression tests, scans, and
compares in order, run `rtk proxy python3 tools/code_review/gate.py --build-dir build`.
After configuring CMake, automation can invoke the optional target
`rtk proxy cmake --build build --config Debug --target code_review_gate`.
The end-to-end test creates an isolated temporary source tree: it verifies a real
clang-tidy null-dereference warning fails comparison, a clean scan passes, and
third-party log entries, compiler failures, timeouts, and stale results are handled.
The real-tool test skips if its prerequisites are unavailable; the gate itself
still requires the pinned clang-tidy and a configured Debug compilation database.

The comparison maps baseline warning locations through unchanged source lines,
then checks file, location, analyzer check and message, deduplicating repeated
header reports. It requires the baseline Git revisions to be available; shallow
clones must fetch them. Changed or ambiguously mapped lines can produce findings
that need review, and identical moved code can still be matched incorrectly. The
three separately reviewed post-baseline findings are in `gate-baseline.json`.
Missing, failed or timed-out scan invocations fail the comparison. The target is
available for CI, but no CI workflow is installed by this project.

Artifact validation performed during creation: both Python tools passed syntax
checks; the dashboard was exercised in headless Chrome for search, status/evidence
rules, dependencies, charts, local persistence, export/import, invalid imports,
mobile overflow and dark mode. Desktop/mobile screenshots were visually checked.
Temporary QA status changes were cleared. Current implementation statuses come
from `progress.json`; see [M01 validation](validation/M01.md),
[M02/M03 validation](validation/M02-M03.md),
[M04/B01 validation](validation/M04-B01.md),
[M05/B02 validation](validation/M05-B02.md),
[M06/T03 validation](validation/M06-T03.md),
[B04/T03 validation](validation/B04-T03.md),
[A01 validation](validation/A01.md),
[C01 validation](validation/C01.md),
[C02 validation](validation/C02.md),
[C03 validation](validation/C03.md),
[B03/D01–D03 validation](validation/B03-D01-D03.md), and
[T01/T02 validation](validation/T01-T02.md), and
[A02 coverage batch](validation/A02.md) for implementation evidence.
