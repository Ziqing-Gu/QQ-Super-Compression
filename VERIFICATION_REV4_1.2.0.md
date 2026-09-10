# 1.2.0 Rev4 verification — promoted to Stable

2026-09-10: the user accepted Rev4 as Stable and authorized B → C → D. Existing measurements below are retained; no rebuild or reinstallation was needed. The completed 23-page PDFs are retained unchanged by explicit user choice.

Historical candidate checkpoint: Rev3 Stable approval was withdrawn after user-reported Range
crackling. No GitHub commit/push/tag, macOS build or Release was performed.
Plan C staging was cleared without deleting its worktree changes. Release HOLD
is recorded in both workspaces. Frozen source snapshots were not accessed.

## Change

Finite downward Range uses an inner smoothstep blend from the original gain
to unity. Linear-level width is min(upper/2, (upper-lower)/2), at most about
6 dB. At/above Range gain remains exactly unity. Range OFF retains the original
downward equation. A finite upward gate uses the same inner continuity principle
with width min(lower, (anchor-lower)/2); no boost occurs at/below the gate.
These are static curves applied to the existing future-window detector: no
new Attack/Release envelope, detector window or latency. Processing within the
transition region changes relative to Rev3; the user subsequently accepted Rev4.

Ten appended automatable enable parameters cover UP/DOWN in ST/L/R/M/S.
They default ON, retain Ratio/threshold values, save with projects and A/B, and
default ON when an older project is restored. Ratio LINK does not toggle them.
Each enable applies a 10ms linear processed/unprocessed crossfade at the internal
sample rate, independent of Ratio smoothing. Interruptions start from the current
fade value. The matching Display law and history projection use branch enables.
Small controls are placed beside each Ratio, scaled for compact LR/MS, with no
overlap with Ratio or LINK hit areas. Off Ratios stay editable and are dimmed.

## Verification

- Actual compiled DSP, state, A/B, legacy migration and existing tests pass.
- Old compiled baseline measured 12.4926 dB Range and 16.9261 dB upward-gate
  discontinuities. Continuity is now a required assertion, including narrow ranges.
- 600Hz carrier, 3Hz amplitude modulation across Range: max adjacent gain change
  0.000819147 at 44.1kHz and 0.000484943 at 48/96kHz. Latency stays 26ms.
- Independent UP/DOWN switching at 44.1/48/96kHz passes 10ms fade and rapid
  reversal checks. In 0ms/8x and 16x, the audible 10%-90% slope spans 384 host
  samples at 48kHz, corresponding to 10ms. Neutral output matches the inherited
  oversampling filter response, rather than assuming ideal DC unity.
- Actual editor controls/host gestures and Display boost/cut/output reprojection
  pass across all five domains. A/B and host-state restore pass. Light/Dark/Classic
  full/minimum-size rendering and geometric checks pass; representative pages
  visually inspected. UI fixtures are synthetic, not DAW playback screenshots.
- All ten inherited core/threshold/Mix/link/sidechain/Display checks pass.
- Build/output/installed bundle hashes match. ZIP CRC and both contained file
  hashes match. Previous installed Rev3 bundle is retained under output Rollback.

These are measured regression results, not a claim of zero noise for all signals
or abrupt detector-level jumps. Current acceptance is recorded in STABLE_1.2.0.md.

Source: D:/Codex/Workspaces/QQSuperCompression-1.2.0-UpDown
Output: D:/Codex/Outputs/QQ Super Compression 1.2.0 Rev4 Candidate
Raw evidence: output Verification and Preview directories.

Installed binary SHA-256: `6AAA598C05346862316B7E05E4C4A5B17E369DE74DF83417250A40AB392472F2`.
