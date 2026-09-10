# QQ Super Compression 1.1.6 Revision 2 — Plan A

Date: 2026-09-07. Status: Windows candidate built, delivered and installed for user verification. Stable/rollback remains v1.1.5; the user approved the interactive knob study, not a Stable promotion of the complete plug-in.

## Delivered

- Source: `D:\Codex\Workspaces\QQSuperCompression-1.1.6-WarmAsset`.
- Build: `D:\Codex\Temp\QQSuperCompression-1.1.6-WarmAsset`.
- VST3, Windows ZIP and actual editor previews: `D:\Codex\Outputs\QQ Super Compression 1.1.6 WarmAsset Plan A`.
- Installed: `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`.
- Recoverable copy of the previously installed v1.1.5: `D:\Codex\Outputs\QQ Super Compression 1.1.6 WarmAsset Plan A\Rollback - installed 1.1.5\QQ Super Compression.vst3`. This is an installation rollback copy, not Plan B.

## Changes

- The accepted interactive material compositor now supplies the real LIGHT rotary controls, including compact LR/MS and sidechain controls. Embedded lit/unlit images replace the rejected shallow-disc renderer; no external runtime image folder is needed.
- Continuous normalized illumination: 0% emits no light, 10% lights the first tenth, 50% the left half, and 100% the entire effective arc. Metal and cast shadow stay stationary. Gain controls use their normalized range, so 0 dB in a bipolar trim is its midpoint, not the minimum.
- LIGHT Input gray, Output orange and GR cyan-blue are shared by Meter, Display traces, legends and corresponding colored readouts. Fine segmented meters and pale wells follow the user's reference.
- Existing positions, control meanings, FineKnob gestures, attachments, parameter IDs, state/A-B, theme memory, audio and Display algorithms are retained. Classic is untouched.

## Verified

- MSVC / JUCE 8.0.15 Windows x64 Release VST3 and metadata: 1.1.6.
- Twelve existing source/math tests: PASS.
- Warm skin source contract: PASS. Processor, parameters, DSP, meter state, Match and DynamicDisplay.cpp are byte-identical to Stable v1.1.5. The full resized() function is identical. DynamicDisplay.h has only the pre-existing visual-test friend addition.
- Original README author introduction and license notice: identical to the untouched active stable repository. No remote README or repository was edited.
- Actual JUCE editor snapshots: ST, LR, MS, sidechain, minimum/maximum size, 2x rendering and numeric editing inspected/generated. They contain synthetic test audio, not the user's mix.
- Classic ST and Classic sidechain PNGs: byte-identical to the preceding candidate's Classic previews. ST/LR/MS bounds manifests also match exactly; baseline source layout equality is separately verified.
- Production material: zero emission at all 1001 probes, fixed metal and shadow checks PASS. 200 unchanged knob paints reuse cached frames; changing the value refreshes once. Approximate 160px frame composition: median 0.55 ms, p95 0.82 ms on this machine; this is not a complete DAW CPU measurement.
- Steinberg SpectraLayers vst3effectsvalidator: exit 0, Fx and Dynamics classes loaded. This is a module-loading check, not comprehensive host/audio certification.
- Relevant DAW/host process check before overwrite: none found. No DAW was launched or operated.
- Both bundle files have matching build/output/installed SHA-256. Previous installed files were copied and hash-verified before overwrite.

## SHA-256

- Main VST3 binary: `9168B9A48522A082C047D4306DD4DA484FBAEDACF2A648624A09E9958D50AF25`.
- moduleinfo.json: `645B2772A7D093C3DBC21BFDCD55F7406BB523BA120DAE8BE30D6E9607D87BC6`.
- Windows ZIP: `08715A897BDA06C8E13147C1FCF95C03A741E3D0D0985D76DF20521284C6D95F`.
- Previous installed v1.1.5 binary: `ABB9CFD1CF7929C6D4C6A7D9F72226535F36169854A1697B44550047F54B64E1`.
- Current bundle: 2 files, 10,290,267 bytes.

## Boundaries

Plan A is complete by the user's simplified artifact-delivery rule. Cubase listening, real-host interaction/HiDPI and complete-plugin visual acceptance remain user verification. No Plan B/C/D, Stable promotion, GitHub, Actions, Release, Figma or macOS build was performed. The earlier rejected Revision 1 and frozen source backups remain untouched.
