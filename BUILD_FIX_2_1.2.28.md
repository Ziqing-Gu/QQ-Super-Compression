# QQ Super Compression 1.2.28 — Build Fix 2

Build-only follow-up to the 1.2.28 Limiter Ceiling OS candidate.

- Resolves ambiguous `OutputCeiling::prepare/set` calls in `peak_guard_processor_checks.h` after the 1.2.28 API split.
- Updates the direct guard regression to test TP/Ceiling switching inside the fixed Limiter-only pipeline; Limiter ON/OFF remains covered through the real processor/PDC tests.
- Makes `ceiling_compare.cpp` use an explicit float Ceiling literal so both current and previous guard APIs resolve cleanly.
- `BUILD_WINDOWS.ps1` now detects a CMake cache created from another source folder and clears that build directory automatically, preventing build-fix folder-name cache mismatches.
- No DSP, latency, Ceiling, Display, parameter, state, or UI behavior changed in this build fix.
