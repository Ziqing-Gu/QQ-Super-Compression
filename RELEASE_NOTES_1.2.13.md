# QQ Super Compression 1.2.13 — TP Display Gain Reduction Candidate

## Purpose

Make True-Peak/Sample-Peak output protection visible in the Dynamic Display without changing audible DSP.

## Changes

- `OutputCeiling` exposes a display-only attenuation value derived from its real limiter gain envelopes.
- Sample-Peak mode reports the native lookahead limiter envelope.
- True-Peak mode combines the 8x main TP guard and residual post guard, then follows the existing 10 ms TP crossfade.
- `PluginProcessor` captures the strongest OutputCeiling attenuation in each processed audio block.
- `DynamicDisplay` stores that captured attenuation with each history point and adds it to the existing Mix-aware compressor gain-change trace.
- ST/LR/MS all receive the same extra ceiling attenuation because OutputCeiling is stereo-linked.
- Dedicated compressor GR meters are intentionally unchanged.
- Audio samples, Ceiling target algorithm, strict 1:1 Link, compressor algorithms and state layout are unchanged.

## Build/test status

The full Windows JUCE/MSVC build has not been run in this environment. `QQSCCeilingCheck` now includes telemetry checks that must pass on Windows before installing the candidate.
