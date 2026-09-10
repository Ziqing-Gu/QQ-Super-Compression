# QQ Super Compression 1.1.6 Revision 3 — Plan A

2026-09-07. Windows candidate built, delivered and installed for user validation. Stable remains v1.1.5.

## Scope

LIGHT only: slightly deeper warm-grey backplates, vertical Meter gradients and level-aligned tonal Display strokes. Gray Input / orange Output / cyan-blue GR hues are retained. Accepted knob assets/compositor, positions, interactions, theme memory, audio, parameters and historical projection algorithms are unchanged. The Display still uses retained paths and sparse GR shading; no new depth-dependent area fill or worker was added.

## Paths

- Source: `D:\Codex\Workspaces\QQSuperCompression-1.1.6-WarmAsset`
- Build: `D:\Codex\Temp\QQSuperCompression-1.1.6-WarmAsset`
- Delivery: `D:\Codex\Outputs\QQ Super Compression 1.1.6 Rev3 Warm Depth Plan A`
- Installed: `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`
- Previous installed Rev2: delivery subfolder `Rollback - installed 1.1.6 Rev2\QQ Super Compression.vst3`; copied and hash-verified before overwrite. Earlier Rev2 outputs and Stable rollback were not changed.

## Checks

- MSVC / JUCE 8.0.15 Windows x64 Release build: PASS, metadata 1.1.6.
- Twelve existing Python/source/math checks plus the updated warm-skin contract: 13 PASS. Processor, DSP, parameters, meter state and Match files match Stable byte-for-byte. All DynamicDisplay code outside drawDomainPanel matches Stable; the change inside that function is paint-only. Complete resized() layout is identical.
- Actual editor PNGs: ST, sidechain, LR/MS, minimum/maximum sizes, 2x and numeric entry generated. Updated ST and actual Meter depth render inspected. Images use synthetic test audio, not a user mix.
- Classic ST and Classic sidechain PNGs match Rev2 byte-for-byte. Warm/Classic ST and warm LR/MS bounds manifests also match Rev2 exactly.
- Actual rendered gray/orange/cyan bars: leading-edge versus origin luminance separation exceeds 0.12, including mirrored GR direction. Meter values and hold/dB mapping were not changed.
- Production knob zero-emission/fixed-material tests and 200-paint cache check: PASS. 160px composition median 0.5632 ms / p95 0.8543 ms; this is not whole-plugin DAW CPU measurement.
- Steinberg vst3effectsvalidator: exit 0, Fx and Dynamics classes loaded. Module-loading check only, not full audio/host certification.
- No matching DAW/host process found immediately before overwrite. Build/delivery/installed hashes match for both bundle files (2 files / 10,292,315 bytes).

## SHA-256

- Main VST3: `D49F05EDF0390944BABD5FD1B8D3672DB76EB5CC5B3BD86CFA1FA3B650036CBD`
- moduleinfo.json: `645B2772A7D093C3DBC21BFDCD55F7406BB523BA120DAE8BE30D6E9607D87BC6`
- Windows ZIP: `A943B6215204328C8067F71702E02C4269FAB9BF2C525086639954855ECBA76D`
- Previous installed Rev2 main binary: `9168B9A48522A082C047D4306DD4DA484FBAEDACF2A648624A09E9958D50AF25`

No Stable promotion, Plan B/C/D, GitHub, Actions, Figma, image generation or macOS build. Real-host appearance/interaction remains user validation. The user's README introduction and license files were not modified.
