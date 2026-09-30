# QQ Super Compression 1.2.13 — Verification Notes

## Baseline

Baseline: `QQSC-1.2.12-TruePeak-Ceiling-Target-BuildFix1-Source.zip` / the corresponding extracted Build Fix 1 tree. The 1.2.12 TP Ceiling behavior is retained.

## Product-source changes

Product changes are limited to display telemetry and display projection:

- `Source/OutputCeiling.h`: exposes the real time-dependent Sample-Peak/TP limiting envelope for display only.
- `Source/MeterState.h`: adds one display-only ceiling-GR atomic.
- `Source/PluginProcessor.cpp`: captures the maximum OutputCeiling attenuation within each host block and publishes it to MeterState.
- `Source/DynamicDisplay.h/.cpp`: records captured ceiling attenuation per history point and adds it to the existing Mix-aware compressor gain-change trace.
- `CMakeLists.txt`: version 1.2.13.

No parameter ID, state schema, compression curve, Link rule, Ceiling target, oversampling factor, audio buffer sample, PDC value, or True-Peak meter behavior is intentionally changed.

## Display semantics

Existing compressor gain change is still statically reprojected from detector history, so Threshold/Ratio/Mix edits retain the existing responsive history behavior. OutputCeiling is time-dependent and cannot be reconstructed from a static transfer curve, so 1.2.13 stores the attenuation actually applied at capture time.

```text
DisplayedGainChange_dB = MixAwareCompressorGainChange_dB + CapturedOutputCeilingAttenuation_dB
```

Positive values mean net cut, negative values mean net boost. If upward compression and OutputCeiling act simultaneously, the trace shows their net result.

The dedicated right-side compressor GR meters remain compressor-only by design; only Dynamic Display history/`GAIN (MIX)` includes OutputCeiling.

## Windows regression required

Run `BUILD_WINDOWS.cmd`. Default build directory:

```text
D:\Codex\Temp\QQSC1213-Build
```

Expected tests:

- `QQSCLimiterCheck ... revision1211`
- `QQSCLimiterCheck ... continuity`
- `QQSCLimiterCheck ... dual`
- `QQSCCeilingCheck`

`QQSCCeilingCheck` retains the 1.2.12 16x Ceiling bounds/tight-0-dBTP checks and adds:

1. a sample-safe/reconstruction-hostile waveform where TP display GR must exceed native Sample-Peak display GR;
2. a TP OFF→ON switch check ensuring display telemetry becomes non-zero and remains finite/non-negative through the existing 10 ms crossfade.

## Manual Cubase acceptance

1. New instance, Limiter ON, drive signal into Ceiling.
2. Observe yellow Output and Cut/Boost history with TP OFF.
3. Toggle TP ON while audio continues.
4. If TP performs additional limiting, the Cut/GAIN history should visibly deepen/reshape during the same timeline; history must not clear merely because TP was toggled.
5. Toggle TP OFF again; the display should return continuously rather than jump/reset.
6. Confirm audio null/level behavior is unchanged versus 1.2.12 under identical settings apart from display telemetry.

## Current environment

No Windows VST3 was built here. Source/structure checks do not replace the Windows/Cubase integration test.
