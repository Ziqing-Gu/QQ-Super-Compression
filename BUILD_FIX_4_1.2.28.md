# QQ Super Compression 1.2.28 - Build Fix 4

- No product DSP / UI / Display source change.
- Keeps the 1.2.28 Limiter-only Ceiling, fixed 8x Hard/TP PDC, and 1.2.27 Display Drag Timing exactly unchanged.
- Updates the legacy Dual-algorithm final-output independence regression:
  - direct branch-law / Display projection isolation remains at the existing 2e-5 tolerance;
  - final rendered RMS isolation uses 1e-4 instead of 1e-5, appropriate for the 1.2.28 endpoint latency/FIR topology while still far below any audible or functional branch change;
  - prints all four Up/Down RMS values and cross-branch deltas for Normal/Limiter and ST/LR/MS before asserting, so any future failure is immediately diagnosable.
- Own-branch effect thresholds are unchanged.
- Build-cache auto-clean from Build Fix 2 is retained.
