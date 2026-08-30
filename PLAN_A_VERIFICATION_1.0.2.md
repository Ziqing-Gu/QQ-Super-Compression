# QQ Super Compression 1.0.2 Plan A verification

Date: 2026-08-30  
Status: Stable baseline, explicitly confirmed by the user

- Source ZIP SHA-256: `FC6C4D7E12C42C0D552E7494021C2DFC23C355186ADCD39482497E40BB5967CD`
- Supplied source manifest: 50/50 entries matched, 0 missing, 0 mismatched.
- Six Python project self-tests: PASS.
- Standalone BS.1770 C++ test: PASS (`-3.0036 LUFS` reference; `6 dB` match).
- Supplied-source Plan A build used the then-selected local JUCE 8.0.14 checkout with MSVC: PASS, 0 warnings, 0 errors.
- Its build, verified output, and installed VST3 binary SHA-256 matched: `7FA14AF306249E0B5D9F7614D520767CD9150A96E99D053969632C82A46122A7`.
- Steinberg `vst3effectsvalidator`: PASS for that build and installed copy, exit `0`; no nested VST3 bundle was created.
- Final Plan B/C/D repository source was rebuilt separately with the exact JUCE 8.0.15 cache and MSVC: PASS, 0 warnings, 0 errors.
- Final repository build PE architecture: `0x8664` (x86_64); `moduleinfo.json` version: `1.0.2`; bundle: 2 files / 6,667,355 bytes.
- Final repository build SHA-256: `00B2232E31BFEE740E03EA8B017A4F006533E14CC7B052CABAE73AB9CFB15FA0`.
- Steinberg `vst3effectsvalidator` on the final JUCE 8.0.15 repository build: PASS, exit `0`.
- The final JUCE 8.0.15 rebuild was not copied over the system installation during Plan B/C/D because DAW closure was not reconfirmed.

Cubase listening, UI, direct-entry, PDC, automation and legacy-project checks remain user-side validation. Plan D cross-platform artifacts require separate same-commit Actions and architecture/hash evidence.
