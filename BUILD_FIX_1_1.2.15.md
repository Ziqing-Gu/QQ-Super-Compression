# QQ Super Compression 1.2.15 — Build Fix 1

This build fix changes **tests/build plumbing only**. Product code under `Source/` is byte-for-byte identical to the original 1.2.15 candidate.

## Why the first Windows regression failed

The TP visual regression used the steady two-sample sequence `+1, -0.7`. That is not a reliable reconstruction-overshoot stimulus in steady state, so the test could report `TP attenuation missing` even when the TP telemetry implementation was correct.

The replacement stimulus is the four-sample quarter-rate sequence `+1, +1, -1, -1`. Its PCM sample peak is exactly 0 dBFS while the ideal band-limited waveform reaches `sqrt(2)` (+3.01 dBTP). Therefore:

- Sample-Peak mode should need essentially no ceiling attenuation.
- True-Peak mode must produce clearly visible ceiling gain reduction.
- The same stimulus is used for both `QQSCCeilingCheck` and the real processor meter/display regression.

The default Windows build directory is now `D:\Codex\Temp\QQSC1215-BF1-Build` to avoid CMake-cache collisions with the first 1.2.15 build.

No audio DSP, limiter, ceiling, display/meter product logic, parameter layout, or state schema was changed.
