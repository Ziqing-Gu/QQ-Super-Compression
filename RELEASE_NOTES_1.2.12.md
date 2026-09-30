# QQ Super Compression 1.2.12 — True-Peak Ceiling Target Candidate

Base: 1.2.11 strict 1:1 Limiter Link source.

## User-visible change
- TP OFF is unchanged: Ceiling remains the final sample-peak output target/trim.
- TP ON now treats the displayed Ceiling as the actual final true-peak upper target.
- Removed the fixed -0.02 dB main TP reserve and the fixed -0.10 dB residual reserve.
- The main 8x guard targets Ceiling directly.
- After downsampling, a separate 16x reconstruction analysis checks the true peak. Residual correction is triggered only when the measured peak exceeds Ceiling.
- When residual correction is required, its target is Ceiling - 0.01 dB, providing only a tiny numerical/interpolation tolerance.
- Material that does not reach Ceiling is never boosted or normalized.

## Preserved
- 1.2.11 strict 1:1 Limiter Link: DOWN Threshold / Makeup / Output only. Ratio, Mix and algorithm changes never move Output.
- Normal/Limiter bank independence; Normal Makeup ±30 dB; Limiter Makeup ±120 dB.
- Limiter Single/Dual threshold continuity.
- TP/Peak 10 ms switching crossfade and constant reported latency architecture.

### Build Fix 1
- Fixed the Windows `QQSCLimiterCheck` test target compilation by correcting the Limiter DOWN Enable parameter reference in `strict_one_to_one_link_checks.inc`.
- Product DSP/source behavior is unchanged from the original 1.2.12 candidate.
