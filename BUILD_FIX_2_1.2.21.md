# QQ Super Compression 1.2.21 — Build Fix 2

## Scope

Build Fix 2 does **not** modify product `Source/`. It corrects the Windows/JUCE retrospective Display regression harness and gives this candidate a unique default CMake build directory.

## Why Build Fix 1 stopped at `Limiter toggle cleared retrospective Display history`

`DynamicDisplay` starts with source-tracking sentinels (`lastMode = -1`, `lastKeySource = -1`, `lastCaptureGeneration = 0`). In the real plug-in, the 60 Hz Display timer runs before any visible history can exist; the first tick establishes those source trackers.

The Build Fix 1 regression injected synthetic private history **before the first timer tick**. Its first timer call was delayed until after toggling Limiter. That tick then performed the normal first-tick source initialisation (`mode/key source/capture generation`), which calls `clearHistories()` by design. The test incorrectly attributed that initialisation clear to the Limiter toggle.

Build Fix 2 first aligns Normal/Limiter to the same ST + Internal source and executes one real Display timer tick. Only then does it inject the fixed evidence window. The subsequent Limiter OFF/ON assertion therefore tests the intended contract: changing Limiter as a projection choice must preserve history when the evidence source itself did not change.

## CMake cache isolation

Default Windows build directory is now:

```text
D:\Codex\Temp\QQSC1221-BF2-Build
```

This prevents another source-directory mismatch with previous 1.2.21 candidates.
