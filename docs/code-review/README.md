# Source review and implementation tracking

Open [dashboard.html](dashboard.html) in a browser for charts, the phased roadmap,
and an editable implementation table. No server or network connection is needed.
Read [PLAN.md](PLAN.md) for the review, priorities, evidence, and acceptance criteria.

`progress.json` is the version-controlled source of truth. All implementation
items start as **Backlog**: completing this review does not mean the fixes exist.
Browser edits are saved locally when storage is available. Use **Export progress**
to save them durably; replace `progress.json` with that export and regenerate:

```sh
rtk proxy python3 tools/code_review/render.py
```

Use **Import progress** to transfer a saved export to another browser. Imports
update progress fields only; finding definitions remain tied to the review.
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

Artifact validation performed during creation: both Python tools passed syntax
checks; the dashboard was exercised in headless Chrome for search, status/evidence
rules, dependencies, charts, local persistence, export/import, invalid imports,
mobile overflow and dark mode. Desktop/mobile screenshots were visually checked.
Temporary QA status changes were cleared. Current implementation statuses come
from `progress.json`; see [M01 validation](validation/M01.md),
[M02/M03 validation](validation/M02-M03.md),
[M04/B01 validation](validation/M04-B01.md), and
[M05/B02 validation](validation/M05-B02.md) for implementation evidence.
