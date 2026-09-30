# QQ Super Compression 1.2.12 — Verification Notes

## Intended TP Ceiling contract
Ceiling is an upper bound, not an automatic normalizer. With TP ON and sustained overload, Ceiling = 0 dBTP should approach 0.00 dBTP as closely as practical without exceeding it. A fixed -0.10 dB reserve is not part of the contract.

Implementation: 8x main TP guard targets the user Ceiling directly. The post-downsample validation path now analyses reconstruction at 16x. Residual attenuation is triggered only if that measured peak exceeds Ceiling; when triggered, the target is Ceiling - 0.01 dB. TP OFF keeps the existing sample-peak path.

## Isolation/source checks executed in this session
- `strict_one_to_one_link_isolated_test.py`: re-run to ensure the 1.2.11 Link contract remains unchanged.
- `limiter_mode_continuity_isolated_test.py --sanitize`: re-run to ensure Single/Dual continuity remains unchanged.
- `true_peak_ceiling_target_source_audit.py`: checks that OutputCeiling has no fixed -0.02/-0.10 reserve, uses a 16x final reconstruction detector, triggers residual correction at the user Ceiling, and uses only a 0.01 dB correction landing tolerance.

These checks do not replace a JUCE/MSVC build or actual audio execution of OutputCeiling.

## Windows acceptance
Run `BUILD_WINDOWS.cmd`. Default build directory: `D:\Codex\Temp\QQSC1212-Build`. It builds the VST3, QQSCLimiterCheck and QQSCCeilingCheck; then runs the 1.2.11 Link/continuity/dual regressions plus the independent true-peak ceiling checker. The ceiling checker includes 16x final metering and a sustained-overload test requiring Ceiling 0 to remain <= +0.005 dBTP and >= -0.05 dBTP.

The build helper does not install the plug-in, execute Plan B, push GitHub, or publish anything.

## Build Fix 1 — Windows test target compile repair

The first Windows build successfully compiled and linked the 1.2.12 VST3, but the QQSCLimiterCheck target failed because `tests/strict_one_to_one_link_checks.inc` referenced a non-existent helper array `qqsc::params::limiterDownEnabledIds`.

Build Fix 1 changes only that test reference to the already-existing Limiter mode parameter ID at `qqsc::params::limiterModeIds[21 + d]` (the five Limiter DOWN Enable parameters). No file under `Source/` is changed by this build fix; the 1.2.12 True-Peak Ceiling algorithm is byte-for-byte unchanged from the prior candidate.
