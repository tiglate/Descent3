"""Render the Markdown plan and portable dashboard from versioned progress data."""

from collections import Counter
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "docs/code-review"
PHASES = [
    (0, "Baseline and triage", "Pin tools, classify warnings and establish the regression gate."),
    (1, "Resource and parser safety", "Fix ownership and bounds issues with synthetic fixtures and sanitizers."),
    (2, "Runtime policy and correctness", "Expose validated options with unchanged defaults; fix file-size handling."),
    (3, "Cleanup and measured modernization", "Remove proven dead code; apply small reviewed tidy batches."),
    (4, "Coverage and release verification", "Extend platform coverage and record runtime validation."),
]


def main():
    data = json.loads((OUT / "progress.json").read_text())
    items, scan = data["items"], data["scan"]
    ids = {i["id"] for i in items}
    assert len(ids) == len(items)
    assert all(set(i["dependencies"]) <= ids for i in items)
    statuses = Counter(i["status"] for i in items)
    lines = ["# Descent 3 source review and implementation plan", "",
             f"Reviewed {scan['date']} at commit `{scan['commit']}`.", "",
             "## Scope and interpretation", "",
             f"Risk-based source review plus an actual {scan['tool']} run on **{scan['scanned_translation_units']} "
             f"of {scan['unique_translation_units']} unique translation units** "
             f"({100 * scan['scanned_translation_units'] / scan['unique_translation_units']:.1f}% of the local database). "
             f"All {scan['successful_translation_units']} selected invocations completed with exit code 0. "
             f"Configuration: {scan['configuration']}. The 1,266-entry multi-config database was deduplicated by source. "
             "This denominator describes this build, not every file in the repository.", "",
             f"The run produced **{scan['unique_diagnostics']} distinct first-party diagnostic locations/messages**, "
             "including included headers. These are not confirmed bug counts. Deduplication uses file, line, "
             "column, check and message; dependencies/system headers are excluded. Manual reference searches "
             "also covered disabled blocks, allocation/cleanup paths, timeouts, and HTTP/audio configuration.", "",
             "The initial baseline scan did not include engine fixes, game launch, sanitizer execution, "
             "cross-platform compilation, or gameplay verification. Later implementation and validation "
             "evidence is recorded per item below. Windows, editor, most gameplay/physics and "
             "plugins require further analysis. Dead-code claims are limited to permanently disabled blocks, "
             "specific dead stores and explicitly labeled removal candidates. Platform-only code is not dead code.", "",
             "## Next implementation priorities", ""]
    pending = sorted((i for i in items if i["status"] in {"Backlog", "In progress"}),
                     key=lambda i: (i["priority"], i["phase"], i["id"]))
    for number, item in enumerate(pending[:4], 1):
        lines.append(f"{number}. [{item['id']}](#{item['id'].lower()}): {item['title']}.")
    lines += ["", "Items marked Review have implementation or triage evidence but still have an acceptance "
             "gate open; see their notes for the remaining work.", "",
             "P1 = address before lower-risk cleanup; P2 = schedule next; P3 = cleanup/measurement. "
             "S/M/L are relative effort estimates, not delivery dates. Confidence is independent of priority: "
             "source-confirmed means visible control/data flow, not a runtime reproduction.", "",
             "## Roadmap", "", "| Phase | Outcome | Items | Exit gate |", "| --- | --- | --- | --- |"]
    for phase, title, gate in PHASES:
        members = ", ".join(i["id"] for i in items if i["phase"] == phase)
        lines.append(f"| {phase} — {title} | {gate} | {members} | Each item's acceptance evidence recorded |")
    lines += ["", "Phase 0 triage can run alongside phase 1 safety fixes. Dependencies in the register are hard "
              "prerequisites; phases express preferred order. Split each item into reviewable patches, add "
              "regressions for changed behavior, and preserve serialized IDs, pilot files, module ABI and game assets.", "",
              "```mermaid", "flowchart LR", '  A["0 · Baseline / triage"] --> C["2 · Runtime policy"]',
              '  B["1 · Resource / parser safety"] --> C', '  C --> D["3 · Cleanup / modernization"]',
              '  A --> E["4 · Platform / release validation"]', '  D --> E', '  B --> E', "```", "",
              "## Implementation register", "", "| ID | Priority | Category | Effort | Confidence | Status |",
              "| --- | --- | --- | --- | --- | --- |"]
    for i in items:
        lines.append(f"| [{i['id']}](#{i['id'].lower()}) | {i['priority']} | {i['category']} | {i['size']} | {i['confidence']} | {i['status']} |")
    for i in items:
        lines += ["", f"## {i['id']}", "", f"### {i['title']}", "",
                  "Sources: " + "; ".join(f"[{x['file']}, line {x['line']}](../../{x['file']})" for x in i["locations"]) + ".", "",
                  f"**Evidence ({i['confidence']}):** {i['evidence']}", "", f"**Change:** {i['action']}", "",
                  f"**Acceptance:** {i['validation']}", "",
                  f"**Dependencies:** {', '.join(i['dependencies']) or 'None'}. "
                  f"**Checks:** {', '.join('`' + c + '`' for c in i['checks']) or 'Manual review / targeted regression'}.", "",
                  f"- [{'x' if i['status'] == 'Done' else ' '}] Implemented and validated ({i['status']})"]
        if i["owner"]:
            lines += ["", "Owner: " + i["owner"]]
        if i["notes"]:
            lines += ["", "Progress evidence / disposition: " + i["notes"]]
    lines += ["", "## Configuration boundaries", "",
              "Prefer configurable operational policy: connection/idle timeouts, HTTP timeouts and supported "
              "audio buffering. Keep defaults compatible and define units, ranges, precedence and invalid-value "
              "handling. Separate connect timeout from established-peer lifetime.", "",
              "Do not expose every numeric literal as a setting. Packet sizes/IDs, RIFF tags, serialized IDs, "
              "sample alignment, angle units and the 22050-Hz asset interpretation encode contracts. Use named "
              "constants where helpful; changing these requires a separate compatibility or resampling design. "
              "The existing asset-directory configuration already avoids embedding workstation paths.", "",
              "## Clang-tidy rollout", "",
              "The initial exploratory check set was intentionally broad. Keep correctness gating focused "
              "on triaged analyzer and bugprone findings; run modernization/performance as advisory until "
              "baselines are stable. No automatic global fixes or blanket warning suppression.", "",
              "```text", scan["checks"], "```", "",
              "Useful follow-up checks: `misc-unused-parameters` and `misc-unused-private-member` for local "
              "cleanup; `clang-analyzer-deadcode.DeadStores` for assignments. None proves that externally "
              "visible functions are unreachable. Use reference/export audits and platform link coverage.", "",
              "The [official clang-tidy documentation](https://clang.llvm.org/extra/clang-tidy/) describes "
              "compilation-database use, check selection and fix controls. "
              "[unix.Malloc](https://clang.llvm.org/extra/clang-tidy/checks/clang-analyzer/unix.Malloc.html) "
              "is the relevant memory-management analyzer family; custom allocators, global ownership, "
              "callbacks and threads still need manual/runtime analysis. Online documentation may describe "
              "newer releases; verify checks with the pinned binary's `--list-checks`.", "",
              "## Verification workflow", "",
              "Reproduce the selected scan (requires an already configured compilation database):", "",
              "```sh", "rtk proxy python3 tools/code_review/scan.py --build-dir build", "```", "",
              "For implementation patches, enable tests outside the source root, build and run the relevant suite:", "",
              "```sh", "rtk proxy cmake -S . -B build -DBUILD_TESTING=ON",
              "rtk proxy cmake --build build --config Release --parallel",
              "rtk proxy ctest --test-dir build -C Release --output-on-failure",
              "rtk proxy ctest --test-dir build -C Release -R 'Control(Presets|Bindings)' --output-on-failure", "```", "",
              "Use a separate build directory for ASan/LSan/UBSan and another for TSan; instrument both "
              "compile and link steps, avoid combining incompatible sanitizers, and retain failure logs. "
              "Use synthetic redistributable WAV/INF fixtures and temporary XDG_DATA_HOME for pilot/input "
              "tests. UI/input changes also require dialog, saved-binding and gameplay checks. Document "
              "actual results per item; planned commands are not evidence of execution.", "",
              "## Progress tracking", "",
              f"Current: {statuses['Done']} implemented, {statuses['Dismissed']} dismissed, "
              f"{len(items) - statuses['Done'] - statuses['Dismissed']} unresolved items.", "",
              "Open [dashboard.html](dashboard.html) for live charts, priority/category breakdowns, roadmap, "
              "editable owners/status/notes and actual change history. Changes use browser storage when "
              "available. Export progress, replace `progress.json`, and run "
              "`rtk proxy python3 tools/code_review/render.py` to update both this plan and the dashboard. "
              "Import transfers progress without changing the finding definitions. Dismissal is resolution "
              "with a reason, not implementation. Done requires validation evidence and Done dependencies.", "",
              "Evidence: [diagnostics.json](diagnostics.json) and scan metadata in [progress.json](progress.json). "
              "Full local logs: `build/code-review/*.log` (ignored build output).", ""]
    (OUT / "PLAN.md").write_text("\n".join(lines))
    template = (ROOT / "tools/code_review/dashboard.html.in").read_text()
    encoded = json.dumps(data).replace("<", "\\u003c").replace(">", "\\u003e").replace("&", "\\u0026")
    (OUT / "dashboard.html").write_text(template.replace("__REVIEW_DATA__", encoded))
    print(f"Rendered {len(items)} items to docs/code-review/PLAN.md and dashboard.html")


if __name__ == "__main__":
    main()
