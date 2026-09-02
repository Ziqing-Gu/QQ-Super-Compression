# QQ Super Compression 1.1.3 - Plan A Verification

**Date:** 2026-09-02  
**Status:** Plan A complete; Windows x64 VST3 generated for user verification  
**Classification:** Candidate; v1.1.2 remains the Stable rollback baseline

## Implemented scope

- Key Gain now reprojects the complete visible detector, Mix-aware GR, and Output history in real time while the control moves.
- Side Chain HPF history is rebuilt once after the HPF gesture ends. Host automation, preset, A/B, and direct non-mouse changes use a short stable-value debounce.
- A display-only ten-second raw-key ring is enabled only while the editor is open, capped at a 48 kHz analysis rate, and replayed by a low-priority worker.
- Replay uses the same second-order Butterworth HPF, ST/LR/MS routing, and current Lookahead peak window as the detector display.
- Audio DSP, parameters, automation identities, A/B meaning, and state schema are unchanged; schema remains 10.

## Build and tests

- Generator: Visual Studio 17 2022, x64 Release.
- Compiler: MSVC 19.44.
- JUCE: 8.0.15.
- Target: QQSuperCompression_VST3.
- Build: PASS.
- Eleven Python/source/math self-tests: PASS.
- Standalone BS.1770: PASS (-3.0036 LUFS reference; 6.00 dB Match).
- Steinberg vst3effectsvalidator: PASS; the module exposed both Fx and Dynamics classes.
- Binary FileVersion/ProductVersion and module metadata: 1.1.3.

## Artifact identity

- Bundle: 2 files, 6,742,107 bytes.
- Main binary SHA-256: `9B37B5C756D33D1E32E7E4982695CB5387FD89284C875D9CE99530AD7004AB59`.
- moduleinfo.json SHA-256: `7677F2D75A9B30A20498F109F49D3B28ACDA97D91F0CEF0615DACF4545E7CD5C`.
- Windows VST3 ZIP SHA-256: `B56006F8DE9F0E70A180B0985B4A5128D37E058E841ED7B6648D74B948E9EC44`.
- Build/output main binary and moduleinfo hashes match.

## User validation boundary

After Cubase was confirmed closed, the verified bundle was installed to `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`; build/output/install binary and moduleinfo hashes match, and the previous 1.1.2 installation was retained as an internal rollback copy. Cubase listening and UI checks remain manual: verify continuous Key Gain history movement, one refresh after releasing HPF, host automation/preset HPF refresh, INT/EXT, ST/LR/MS, both themes, SC Listen, Match, PDC, A/B, state restore, and true Bypass.

No Plan B/C/D, Stable promotion, GitHub, Actions, macOS build, or Release work was performed.
