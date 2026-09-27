# QQ Super Compression 1.2.7 — Output linkage and independent Dual algorithms

Plan B checkpoint requested on 2026-09-28. The Windows build is validated and installed. See `PLAN_B_LOCATION.json` for the formal source snapshot and its verification report. This checkpoint keeps version 1.2.7; it does not publish a new release.

License: Qing Audio Non-Commercial Source-Share License 1.0. See `LICENSE` and `LICENSE_POLICY_CHANGE.md`.

Source checkpoint: `D:\Codex\Workspaces\QQSuperCompression-1.2.7-Output-Link-Fix`.

Limiter Output Gain has an explicit control whitelist: interactive Down Threshold and Makeup edits may move it while Limiter LINK is on. Ratio, reciprocal Ratio Link, Classic/Super, Single/Dual, ST/LR/MS, branch on/off, Mix, Input, Range, TP, Ceiling and monitor selection do not rewrite Output. Turning LINK on preserves the current values. Later allowed edits use their own current starting point, so they do not compensate earlier unrelated edits.

MATCH applies only the relative Output correction caused by its Makeup edit; it retains the user's existing output offset. Its correction continues to use the actual dry/wet mix. Manual Makeup/Down Threshold linking remains independent of Mix.

At unity Ratio or another condition with no downward threshold response, Output remains adjustable without moving a threshold. Explicit A/B, Limiter/normal bank recall, project load, undo/redo and host automation may restore/set stored Output values. The separate normal-mode Input/Output LINK retains its existing behavior.

The strict Output fix was completed and verified first; its source and binary are recorded in `STRICT_OUTPUT_CHECKPOINT.json`.

Single's Classic/Super selector now sits beside Ratio. Dual has separate Up and Down selectors, each shared across L/R and M/S, with independent 10 ms crossfades. Limiter occupies the former header algorithm-button position. Branch selectors never link to Output. Single retains its own choice. Each A/B slot and Normal/Limiter bank stores the two branch choices; new instances remember the last choices, while saved sessions take priority. Older sessions initialize each branch from that bank's old algorithm to preserve its sound.

The plugin identity and version remain unchanged. Four host parameters are appended without changing existing IDs/order: Up Algorithm, Down Algorithm and the Limiter equivalents. State schema is 22. No detector, lookahead, Ceiling or LUFS processing change is intended.

Build: `build-unity.cmd QQSuperCompression_VST3 QQSCLimiterCheck QQSCUnityCheck QQSCLoudnessCheck QQSCLimiterVSTCheck`.
Build and test artifacts: `D:\Codex\Temp\QQSC-1.2.7-Strict-Output-Link`.

Final validation: `VERIFICATION_FINAL.md` and the four `final-*-check.log` files. Older intermediate logs are retained as history; use the final logs for this checkpoint.

`docs/` preserves the previous 1.2.7 bilingual manuals, their editable sources, images and earlier document history. Those PDFs predate the latest Ratio limits, mode memory, LUFS and Output/Dual algorithm changes. Their historical release labels apply to the earlier build, not this checkpoint; see `docs/CURRENT_DOCUMENT_STATUS.md`. No manual revision is claimed here.

For an offline rebuild from the complete Plan B snapshot, see `docs/BUILD_PLAN_B.md`.
