# QQ Super Compression 1.2.21 — Build Fix 1

## Cause

The original 1.2.21 Windows integration run compiled the VST3 successfully, then failed only at:

```text
FAIL: Ceiling did not retrospectively update TP contribution to blue
```

That expectation was wrong. In Limiter mode, `getActiveOutputGainDb()` includes the current Ceiling offset, while `OutputCeiling` uses the same Ceiling as its target. A Ceiling change therefore shifts guard input and target together; it must not be interpreted as extra TP gain reduction.

## Fix

Only verification/build documentation is changed. Product `Source/` is unchanged. The corrected regression now verifies that Ceiling:

- increments the Display projection revision;
- reprojects the same historical evidence;
- leaves Blue (`Dynamics + Mix + actual TP GR`) unchanged when TP GR is mathematically unchanged;
- shifts Orange/final projected output.

The Windows build directory is changed to `D:\Codex\Temp\QQSC1221-BF1-Build` to prevent stale build/test artefacts from the failed candidate run.
