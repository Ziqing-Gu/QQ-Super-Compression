# QQ Super Compression 0.1.10 — Plan A Verification

- Date: 2026-08-27
- Status: Candidate / Test
- Source archive: `QQ_Super_Compression_0.1.10_Source.zip`
- Source archive SHA-256: `00B2B3F91CC32357EDE43D731AE2E4457267F2733DA454FBE56DDB5E7A99FEE9`
- Build: Windows x64 Release VST3
- Toolchain: Visual Studio 2022 Build Tools / MSVC 19.44.35227.0
- Dependency: JUCE 8.0.15, tag commit `91ad83ae34a81e0833b1a2b0866f54846370ae53`
- Module size: 6,556,672 bytes
- Module SHA-256: `B4B9DA1A215F52D145469F12EE7C08F060E1D119EC2BA39BDDD7063685621009`

## Automated checks

- Source manifest: 25/25 entries matched SHA-256.
- CMake configure and `QQSuperCompression_VST3` Release build: PASS.
- PE machine: `0x8664` (x64).
- Module metadata: version `0.1.10`, vendor `Qing Audio`: PASS.
- DLL/VST3 entry points `InitDll`, `GetPluginFactory`, and `ExitDll`: PASS.
- Steinberg `vst3effectsvalidator.exe`: exit code `0`.
- BS.1770 self-test: PASS.
  - 1 kHz 0 dBFS mono: -3.0036 LUFS.
  - Fixed 6 dB Dry/Wet difference: 6 dB Match.
- Cubase was not running before the system VST3 was overwritten.
- Built, delivered, and installed module hashes matched.

## Compiler warning

MSVC emitted non-fatal warning C4459 in `Source/Parameters.h`: the local `lookaheadMs` declaration hides `qqsc::params::lookaheadMs`. The build and all automated checks completed successfully. This is a name-resolution/readability warning, not a runtime failure; it can be cleaned in a later focused source revision.

## Manual checks still required

- Cubase GUI and listening test.
- 0 ms 1x/8x/16x aliasing and CPU comparison.
- 0 ms 8x/16x PDC, 50% Mix, and Bypass alignment.
- Non-zero Lookahead hidden-control/effective-1x behaviour.
- A/B, Undo/Redo, automation, project restore, and 0.1.9 state migration.
- Explicit user acceptance before any Stable designation.
