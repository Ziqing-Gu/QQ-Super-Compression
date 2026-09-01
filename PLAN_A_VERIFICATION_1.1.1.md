# QQ Super Compression 1.1.1 - Plan A Verification

**Date:** 2026-09-02
**Status:** Plan A complete; Windows x64 VST3 generated for user verification
**Stable classification:** Plan B in the same run records v1.1.1 as Stable
**Previous Stable rollback:** v1.1.0 External Key / Sidechain

## Implemented scope

- Added an appended keyHpfHz sound parameter: default OFF; logarithmic active range 20-500 Hz.
- Added a smoothed second-order Butterworth high-pass after INT/EXT key source selection and after EXT Key Gain.
- Filtered signal feeds detector, Key Level meter, and latency-aligned SC Listen only; carrier remains untouched.
- OFF preserves v1.1.0 full-band key behaviour; old state and old A/B snapshots migrate explicitly to OFF.
- HPF participates in automation, project state, Undo/Redo, A/B selection and A/B copy. State schema is 10.
- Side Chain popup grows leftward to 330x146 with identical LIGHT/CLASSIC layout and no main-layout movement.

## Build and tests

- Generator: Visual Studio 17 2022, x64 Release.
- Compiler: MSVC 19.44.35227.0.
- JUCE: 8.0.15.
- Target: QQSuperCompression_VST3.
- Build: PASS, no new compiler warnings.
- Nine Python/source/math self-tests: PASS.
- Standalone BS.1770: PASS (-3.0036 LUFS; 6.00 dB Match).
- Steinberg validator/pluginval: not run because no executable is available in the checked local environment.

## Artifact identity and parity

- Binary FileVersion/ProductVersion: 1.1.1.
- VST3 module metadata: 1.1.1.
- Build/output/install: 2 files, 6,716,507 bytes each.
- Tree SHA-256: 50FB3109C22DDB55E591941301C81A034CFEC097501C2639BA0E7FF9D273CFB6.
- Main binary SHA-256: C40883E83B87DCC2B810B8998A93ADCB03DFA57A87469591019FC89332969F6C.
- moduleinfo.json SHA-256: B1FAB01E12B0B221507D6F7B3437BADD30934626F6843D284423C26030FFA9A3.

Locations:

- Build: verified local Release VST3 bundle; machine-specific path omitted from public source.
- Output: verified local Plan A formal output; machine-specific path omitted from public source.
- Installed: C:\Program Files\Common Files\VST3\QQ Super Compression.vst3
- Pre-install v1.1.0 backup: verified internal rollback copy; machine-specific path omitted from public source.

Cubase/DAW processes were absent immediately before installation. No DAW was launched by Codex.

## User validation boundary

Cubase scan/load, HPF listening and automation, old-project migration, A/B, SC Listen/PDC, INT/EXT, ST/LR/MS, both themes, and all established audio regressions remain manual user checks. No Plan C/D, GitHub, Actions, macOS artifact, package, or Release work was performed.
