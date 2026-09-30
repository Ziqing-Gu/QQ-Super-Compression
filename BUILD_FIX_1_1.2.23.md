# QQ Super Compression 1.2.23 — Build Fix 1

## Scope

Build Fix 1 does **not** modify product `Source/`. It corrects the Windows/JUCE `revision1223` test setup and isolates the CMake build directory.

## Why the first 1.2.23 run stopped

The failing assertion was:

```text
FAIL: Limiter Dual UP numeric entry escaped 1:8 floor
```

The test was exercising the absolute UP range while `dualRatioLink` was still at its product default of ON. The existing Dual Ratio LINK preserves `UP * DOWN`; with Limiter DOWN capped at `1000:1`, a linked gesture can legitimately stop UP above `1:8` before the absolute UP floor is reached. That is established LINK behaviour, not a range escape.

Build Fix 1 disables `dualRatioLink` only for the absolute range/numeric-entry assertion, then leaves the existing dedicated Ratio LINK regression to validate product-preserving coupling.

## CMake cache isolation

Default Windows build directory is:

```text
D:\Codex\Temp\QQSC1223-BF1-Build
```
