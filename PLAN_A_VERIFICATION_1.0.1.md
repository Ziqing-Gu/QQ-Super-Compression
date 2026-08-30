# QQ Super Compression 1.0.1 Plan A verification

## Build identity

- Version: 1.0.1
- Revision: Display 0…-90 dB Scale Polish
- Framework: JUCE 8.0.15
- Toolchain: Visual Studio 2022 / MSVC, Release, Windows x64
- Format: VST3

## Completed checks

- Source manifest: 48 entries, no missing or mismatched files.
- Threshold rebuild self-test: passed.
- Transparent future-window core self-test: passed.
- Relative domain LINK self-test: passed.
- Independent LR/MS Mix self-test: passed.
- Display scale self-test: passed for 0…-90 dB and 15 dB grid spacing.
- BS.1770 self-test: passed; 1 kHz 0 dBFS mono measured -3.0036 LUFS and the 6 dB difference check returned 6 dB.
- Windows x64 VST3 Release build: passed.
- PE architecture: x86_64 (`0x8664`).
- Installed system binary and local delivered binary SHA-256: identical.

## Windows binary

SHA-256:

`3D453C41B627C974A44ADFA8FA1644F190662239F68035E5ECD84A2CD68F9C63`

## Validation boundary

Steinberg's validator was not available in the local environment, so no validator pass is claimed. Cubase listening, UI inspection, PDC, Mix/Bypass alignment, automation, A/B, Undo/Redo, and legacy-project migration remain manual user checks.
