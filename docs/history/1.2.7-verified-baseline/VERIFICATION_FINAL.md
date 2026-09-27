# Local candidate verification — 2026-09-28

Version/identity: QQ Super Compression 1.2.7, existing VST3 identity retained. This report records the completed local validation. Plan B was subsequently requested on 2026-09-28; its independently verified snapshot report is referenced by `PLAN_B_LOCATION.json`. No Plan C/D publication is included.

## Completed in the requested order

1. Strict Limiter Output linkage was repaired and verified first. Source and actual VST3 checkpoint: `STRICT_OUTPUT_CHECKPOINT.json`.
2. Single algorithm selector moved beside Ratio. Dual gets independently selectable Up/Down algorithms, shared across channels in LR/MS. Limiter moved into the old header algorithm position.

## Output write audit

- Interactive Down Threshold and Makeup are the only linked sources; Limiter LINK must be on. Mix is excluded from their gesture reference. Each gesture captures current values, so earlier unrelated changes are not compensated later.
- Removed Ratio, algorithm, Single/Dual, ST/LR/MS and Down-enable reconciliation callbacks. New branch algorithm buttons never call the Output reconciler.
- MATCH preserves the current Output offset and applies only the change caused by its Makeup correction, using the actual mixed signal for this measured correction.
- Unity Ratio/inactive downward transfer does not lock manual Output editing.
- Explicit Output editing/host automation, A/B or Normal/Limiter bank recall, project load, undo/redo, and first Limiter-bank initialization remain valid state operations. Normal-mode Input/Output LINK is unchanged.
- First-bank reference calibration is retained; unrelated interactive controls do not rewrite it.

## Final checks (all exit code 0)

`final-ui-check.log`:

- 144 Mix-then-linked-edit scenarios; Mix-dependent Output differences: 0 dB.
- 3,120 unrelated GUI control edits left Output and calibration exactly unchanged, including Single Ratio -> 1:1 -> Up/Down, text entry, Alt reset, Ratio Link and the new Up/Down algorithm controls.
- All 69 active values across A/B x Normal/Limiter persist through switch/copy/load/undo/redo.
- 600 branch-specific display projections and 48 actual processor renders: changing Up does not change steady Down audio, and vice versa; all four algorithm combinations work in ST/LR/MS and Normal/Limiter.
- Independent 10 ms algorithm crossfades and reversal tested at 44.1/48/96 kHz. Maximum adjacent-sample DC step: 0.000140354.
- Old sessions inherit each bank's own algorithm, including the active bank; JUCE's automatic missing-node insertion is accounted for before state replacement.
- 18 actual editor layouts cover three themes, ST/LR/MS and Single/Dual. Selector visibility/nearby button overlap checked; rendered Light, Dark and Classic layouts inspected. Last-choice preferences yield to saved state.

`final-unity-check.log`:

- MATCH preserves manual Output offset after Ratio -> 1:1 with LINK on/off.
- 24 measured MATCH scenarios; maximum post-Ceiling mismatch: 0.099708 LU.
- 1:1 monitoring, TP/Peak, bypass, and rapid A/B transitions passed.

`final-lufs-check.log`:

- LUFS calibration, gates, final-output measurement, editor layouts, stopped hold and playback restart passed. Worst histogram/exact-reference discrepancy: 0.00901601 LU.

`final-vst-check.log`:

- Actual previous/current VST3s load with the same identity, existing parameter ID/order and latency. Exactly four new branch algorithm parameters are appended.
- 120 normal-mode and 24 Limiter audio comparisons with matching algorithms: zero residual.
- Previous VST3 wrapper state migrates successfully.

Build artifacts: `D:\Codex\Temp\QQSC-1.2.7-Strict-Output-Link`.
Binary hashes and installation status: `CANDIDATE_ARTIFACTS.json` and, after installation, `INSTALLATION_DUAL_ALGORITHMS.json`.
