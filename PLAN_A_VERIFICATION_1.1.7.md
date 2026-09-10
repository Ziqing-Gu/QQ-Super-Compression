# QQ Super Compression 1.1.7 — Three Themes — Plan A

2026-09-07. Windows candidate built, delivered and installed for user validation. Stable/rollback remains 1.1.6 Revision 3. No Plan B/C/D or publishing performed.

## Final user scope

- Keep **Light, Dark and Classic**. The earlier replace-Classic plan was superseded before delivery.
- Dark follows the user's later near-black reference, keeping blue knob lights and avoiding mottled backgrounds. Large surfaces use solid colors, with restrained depth only on control bodies and edges.
- Dark silver Input / orange Output / cyan GR are shared by meters and Display. Input has a clearly darker inactive well. Actual meter direction, scale and signal values are unchanged.
- Continuous normalized illumination: 0% no emitted light, 10% first tenth, 50% left half, 100% full usable arc. Body/shadow stay fixed. Each control caches its last rendered frame; no image processing is added to the audio thread.
- Right-top button cycles Light -> Dark -> Classic -> Light. `uiTheme` remembers the last choice; missing keys preserve the previous Light/Classic boolean choice. Theme never changes audio parameters or A/B state.

## Paths

- Source: `D:\Codex\Workspaces\QQSuperCompression-1.1.7-Dark`
- Build/tests: `D:\Codex\Temp\QQSuperCompression-1.1.7-Dark`
- Output: `D:\Codex\Outputs\QQ Super Compression 1.1.7 Dark Plan A`
- Installed: `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`
- Rollback: output subfolder `Rollback - 1.1.6 Rev3\QQ Super Compression.vst3`. The installed previous bundle was copied and hash-verified before overwrite. Frozen Plan B backups were not accessed.
- Actual screenshots: output `Preview`. Synthetic audio, offscreen rendering of the production editor, not an image-generation mockup or a Cubase screenshot.

## Verified

- JUCE 8.0.15 / MSVC x64 Release VST3 and QQSCVisualCheck: success; no compiler warning/error lines in final build log.
- Fourteen Python source/math contracts passed; additional comparison against the active Rev3 source proves processor/DSP/parameters/state, Match, Display history/cache logic and LIGHT assets unchanged. Full `resized()` layout unchanged.
- Actual rendering covers Light/Dark/Classic ST, LR/MS, sidechain; Light and Dark minimum/maximum sizes, 2x images and numeric entry. All ten Light reference images and Classic ST show zero changed pixels outside the version readout. Classic sidechain has one legacy HPF edge raster pixel differing by at most 3/255 per color channel; checked with explicit narrow tolerance. This is not claimed to be bit-identical everywhere.
- Theme preference: legacy true -> Classic, false -> Light; three-theme cycle and test-file disk reopen restore the expected theme without changing APVTS state. Test files stayed in D:\Codex\Temp; user preference files were not written by validation.
- Dark 0/10/50/100% blue emission, inactive arc, stationary body/shadow, and 200 unchanged cache draws: passed. Input active/inactive average luminance difference exceeds 0.18 in the reference-level render.
- Dark 160px redraw timing on this machine: median 0.4257 ms, p95 0.5613 ms; unchanged frames use the cache. This is not a full DAW CPU measurement.
- Steinberg vst3effectsvalidator: exit 0; Fx and Dynamics classes loaded. Module-loading check only, not full host/audio certification.
- No matching DAW/host process immediately before installation. Both build/output/install files match, 2 files / 10,309,211 bytes. Prior bundle retained for rollback.

## SHA-256

- Main VST3: `7F599D01EFBD2F0CE62CC2D74F546BD5254DDEAB9B88520848A7301216C05723`
- moduleinfo.json: `F3442C145C69926E87C228F301C97980128D775AFB5AAD1AA5D91CFB89D2BBE7`
- Windows ZIP: `FD99CCE6ED32B2B0503A0C342BD9FD8329DD5A8FFC8A2CD7126B547F48ACC65E`
- Prior installed Rev3 main VST3: `D49F05EDF0390944BABD5FD1B8D3672DB76EB5CC5B3BD86CFA1FA3B650036CBD`

User follow-up: validate actual appearance, three-theme switching, edit/reopen and audio behavior in Cubase. No GitHub, Actions, Release, image generation, Figma, extra dependencies, formal source backup or Stable promotion in this implementation.
