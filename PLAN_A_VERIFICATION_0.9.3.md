# QQ Super Compression 0.9.3 — Plan A verification

Date: 2026-08-27  
Status: Windows x64 VST3 build verified; DAW acceptance remains a user step.

## Build

- Target: `QQSuperCompression_VST3`
- Configuration: Release, Visual Studio 17 2022 x64
- JUCE: local JUCE 8.0.14 checkout (FetchContent disabled)
- Bundle: `QQ Super Compression.vst3`
- Build path: `build/QQSuperCompression_artefacts/Release/VST3/QQ Super Compression.vst3` (local Plan A/Plan C staging)
- PE architecture: x64 (`8664 machine`)
- Binary size: 6,582,784 bytes
- SHA-256: `A23917FBAAAC3547C942563B512923B5765888BE0411CAEA9B150CE716BF36BD`
- `moduleinfo.json`: version `0.9.3`

## Tests

- `tests/bs1770_match_selftest.cpp`: PASS
  - 1 kHz 0 dBFS mono: `-3.0036 LUFS`
  - 6 dB fixed difference, ST Match: `6 dB`
- `STATIC_CHECK_0.9.3.txt`: PASS
- Steinberg `vst3validator.exe`: not installed in the available Windows toolchain; bundle structure, module metadata, and x64 PE headers were checked instead.

## Installation

Cubase was confirmed closed before replacement. The verified bundle was copied to:

`C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`

Installed binary SHA-256 matches the build SHA-256 above. The previous installed bundle was preserved in the Plan A temporary directory for rollback.

