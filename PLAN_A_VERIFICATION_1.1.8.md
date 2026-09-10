# QQ Super Compression 1.1.8 — material refinement candidate

2026-09-07. Local Windows Plan A candidate; Stable remains 1.1.7. No Plan B/C/D, GitHub, Actions, Figma, image generation or Multiband changes.

## Changes

- Dark: integrate the exact approved JUCE refined knob renderer (SHA-256 `F60F55665A1C37B3EC9C916A56ED7B2F70FEFADB81C8D5493F2035A26487415C`), including bevel, raised wall, contact shadow and local blue-light reflection.
- Dark bottom panel only: stationary, fine, low-contrast graphite grain, generated once at 2x resolution and reused. No cloudy or mottled texture; the main Display stays clean. No audio-thread rendering work.
- Light: 13 fixed, non-emissive exterior ticks aligned to its existing 260-degree normalized travel. Lengthen the pointer from 44 to 92 material units, slightly widen/brighten its warm-white core. The approved texture/arc remains intact, and zero still has no emitted light. No invented numeric scales.
- Retain all three themes, existing knob/layout positions and interactions, theme memory, parameters, DSP, Match, Mix-aware GR and Display history behavior.

## Checks

- MSVC x64 Release VST3 and actual-editor renderer built using local JUCE 8.0.15. No C++ compiler warnings/errors. CMake emitted a non-fatal environment warning that CMAKE_GENERATOR_PLATFORM is ignored when CMAKE_GENERATOR is unset; the configured VS 2022 x64 generator was retained.
- Fourteen source/math checks passed. DSP/processor/state/parameters/history/meters and Light bitmap assets equal active Stable 1.1.7. Complete resized() unchanged. Light compositor/cache unchanged except paintPointer and insignificant EOF whitespace.
- Exact approved Dark renderer source hash matches its accepted study. Dark 0/10/50/100% progressive blue light, fixed body/shadow and 200 cached paints passed. Light zero emission/fixed body/cache checks passed.
- Bottom finish: one generation across 200 repaints, and identical pixels from an independent renderer when each frame starts on the same background. The initial test erroneously accumulated antialiased panel edges across uncleared frames; corrected the test to match the editor's background clear, without changing production code.
- Actual editor renders: three themes in ST/LR/MS and sidechain; Light/Dark numeric entry, minimum/maximum sizes, 2x and bottom details. All Light changes are confined to actual rotary draw areas; its Display/meters/background are unchanged. Classic ST/LR/MS/2x match outside the version label; one legacy sidechain dial-edge pixel differs by <=3/255. Dark backgrounds/meters match; LR/MS have 1/2 trace-edge raster pixels differing by <=8/255 with identical Display source. Narrow edge tolerance, not a broad screenshot mask.
- Three-theme cycle, old preference migration, disk round-trip and unchanged APVTS state passed. Test files stayed in D:\Codex\Temp; user preferences were not written.
- Final Dark 160px composition timing: median 0.936 ms, p95 1.6356 ms. Cached when unchanged; this is a component timing, not whole-DAW CPU certification.
- Steinberg vst3effectsvalidator exit 0, Fx/Dynamics module loaded. This is a module-loading check, not full DAW/audio certification.

## Paths and hashes

- Source: `D:\Codex\Workspaces\QQSuperCompression-1.1.8-DarkMaterial`
- Build/test evidence: `D:\Codex\Temp\QQSuperCompression-1.1.8-DarkMaterial`
- Final output: `D:\Codex\Outputs\QQ Super Compression 1.1.8 Material Plan A`
- Preview: actual production editor offscreen screenshots with synthetic audio, not a generated design mockup.
- VST3 bundle: 2 files / 10,316,379 bytes.
- Main VST3 SHA-256: `7301F20BD7CDD68CB33444E71A9FB9CB5D4ED43784EC7B311A93F2E9756E1731`
- moduleinfo.json SHA-256: `29BA287C0B1E207146D147FA24F1958A3451D52B447A32310E98A9DAB0605D2A`
- Prior installed Stable 1.1.7 SHA-256: `7F599D01EFBD2F0CE62CC2D74F546BD5254DDEAB9B88520848A7301216C05723`

- Windows ZIP SHA-256: `4E64FF540A34D8C81702D4730F8B12B39FD3055B9D7D5CBD242AE530F5DB06A7`

Delivery and installation completed. Build/output/install hashes match for both bundle files. PluginDoctor was initially running; no overwrite occurred while it was open. Both host checks during delivery found no matching host/DAW. Prior installed 1.1.7 was copied and both files hash-verified in `Rollback - 1.1.7 Stable\QQ Super Compression.vst3` before overwrite. Installed path: `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`. Frozen Plan B backups were not accessed. Actual Cubase visual/audio acceptance remains the user's check; 1.1.8 is not Stable.
