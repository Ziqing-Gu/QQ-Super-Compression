# 1.2.7 Stable - accepted Unity Monitor verification

Source: `D:\Codex\Workspaces\QQSuperCompression-1.2.7-Unity-Monitor`

Build and logs: `D:\Codex\Temp\QQSC-1.2.7-Unity-Monitor`

Baseline: `QQSuperCompression-1.2.7-Adaptive-Ceiling`. Final output guard DSP is unchanged. The following checks were run for the candidate, subsequently installed and promoted unchanged to 1.2.7 Stable. The user then authorized matching bilingual manuals and Plan C/D publication. See STABLE_1.2.7.md and RELEASE_NOTES_1.2.7.md for current release scope.

## Accepted changes

- TP switch retained, default ON for new instances. Explicit saved TP / Peak choices retained, including A/B. Inactive and hidden outside Limiter.
- Small headphone switch left of Output Gain; default OFF, project-local, independent of A/B.
- Listening compensation occurs after the complete Ceiling, cancelling the visible Limiter Output Gain. Threshold / Makeup links remain operational.
- Converted MON ceiling is Ceiling minus Output Gain; actual output TP meter includes listening compensation.
- MATCH compares aligned original input and actual post-Ceiling monitored output; common Makeup correction preserves domain balance and accounts for dry/wet correlation.
- Headphone and bypass transitions use 20 ms smoothing; dry bypass bypasses the Ceiling guard and retains exact delay alignment. The limiter continues running behind bypass.

## Verification

- `unity-final.log`: 18 processor waveform-scaling cases, Classic / Super, ST / MS / LR, positive and negative Output Gain. Maximum sample error after expected scaling: 1.20656e-7. Limiter OFF null and bypass exact dry passed.
- 12 linked MATCH cases, two algorithms, three domain modes and 100% / 50% Mix: maximum independent measured mismatch 0.09314 LU on the deterministic test programme. This is a measured test result, not a guarantee for arbitrary changing material or unreachable gain targets.
- Headphone / bypass transition DC adjacent-sample change <= 0.000104025, settled bypass bit-exact.
- Equal monitored A/B levels, including rapid retarget: Peak maximum level error 5.59e-9; TP maximum level error 1.97e-5 and adjacent change 1.63e-6 at DC input 0.02.
- `vst-final.log`: original formal Qscp identity and parameter IDs/order retained. 120 actual VST3 Limiter OFF cases bit-identical to accepted detector after reported delay compensation; old 1.2.6 wrapper state recall passed.
- 12 actual VST3 Limiter ON cases (TP / Peak, both algorithms, all three modes) bit-identical to installed Adaptive Ceiling with headphones OFF. With headphones ON, maximum scaled-null sample error 2.26235e-8.
- `limiter-final.log`: 24 actual-processor Ceiling cases; maximum TP relative to ceiling -0.06897 dB in these cases. Peak mode permits intersample overshoot as designed. Host/plugin bypass, constant latency below 40 ms with default 26 ms lookahead, linked controls, save/recall, A/B, Ceiling edit/reset/undo, TP hold/expiry/reset passed.
- Layout previews cover three skins and ST / MS / LR; final UI-only pass also verifies the bottom controls have followed the changed mode.

## Binary identity

Candidate DLL SHA-256: `292F588A9E91FF8EC5BBD609BDFB3E9CD6165DDA1D2B22C2C28427AE2B2DAAC6`

Previous installed baseline DLL SHA-256: `CD63D82685DBF9EF186F585D4A0CCCE73E8D239A1DAEBEB0E0422A94DE580851`

The release includes matching 35-page Chinese and English manuals in docs/manuals. The historical candidate listening procedure remains in `试听说明.md`; current manuals take precedence.

## Authorized installation

Installed on 2026-09-27 at 19:27 +08:00 after checking host processes were closed. Both installed bundle files match the verified candidate by SHA-256. Installation location: `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`.

Previous bundle backed up and verified at `D:\Codex\Archives\QQSuperCompression-1.2.7-before-Unity-Monitor-20260927-192702\QQ Super Compression.vst3`. Full installation record: `INSTALLATION.json`.
