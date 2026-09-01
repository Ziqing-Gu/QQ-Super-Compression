# QQ Super Compression 1.1.0 — Plan A Verification

**Date:** 2026-09-01
**Status:** Plan A local implementation/build/install complete; Candidate, not Stable
**Stable rollback baseline:** v1.0.4 Light / Classic UI switch
**Working branch:** `candidate/external-key-1.1.0`
**Base commit:** `34d163391881262395b35b9358c7c63cb54ebbb2`

## Implemented scope

- Optional mono/stereo VST3 input bus named `Sidechain`.
- `SC: INT / SC: EXT` top control and theme-aware 230x146 floating panel.
- INT is the v1.0.4 post-Input-Gain detector; EXT is sidechain audio after dedicated Key Gain.
- External key uses the same future-window detector, Lookahead, Ratio, Threshold, domain, and Oversampling rules; no Attack/Release envelope.
- Disconnected or silent EXT produces zero gain reduction while preserving the audible carrier.
- ST common key; LR independent key L/R; stereo external MS matrix; mono external key drives both M/S detectors in common.
- Key Source and Key Gain are appended sound parameters included in automation, project state, undo, migration, and A/B.
- SC Listen is latency-aligned final-audible-only, ignored by true Bypass, excluded from automation/state/A-B, and reset OFF on panel/editor close and state restore.
- LIGHT and CLASSIC use identical panel geometry/features.

## Build

- Generator: Visual Studio 17 2022, x64 Release.
- Compiler: MSVC 19.44.35227.0.
- JUCE: pinned 8.0.15 source; machine-specific checkout path omitted from public source.
- Build directory: verified local Release build directory; machine-specific path omitted from public source.
- Target: `QQSuperCompression_VST3`.
- Result: PASS, no warnings in the successful rebuild.
- Binary FileVersion/ProductVersion: `1.1.0`; VST3 module metadata: `1.1.0`.

## Automated regression

Eight Python/source/math self-tests: PASS

1. `display_scale_selftest.py`
2. `domain_link_selftest.py`
3. `external_key_selftest.py`
4. `independent_mix_selftest.py`
5. `link_ui_source_selftest.py`
6. `monitor_audition_selftest.py`
7. `threshold_rebuild_selftest.py`
8. `transparent_core_selftest.py`

Standalone BS.1770 test: PASS

- 1 kHz 0 dBFS mono reference: `-3.0036 LUFS`.
- Fixed Match difference: `6.00 dB`.

Steinberg validator: **not run**. No `vst3validator`, `vst3effectsvalidator`, or `pluginval` executable was available in the searched local environment. This is an explicit validation gap, not a pass.

## Artifact and install parity

Build, formal output, and system install each contain 2 files / 6,705,755 bytes and have identical tree SHA-256:

`F0F45D9C82EB80611025BA7E1799C7218A432A1BF099A02168041AD272B8EEAE`

Main VST3 binary:

- Bytes: `6,704,640`
- SHA-256: `67C8FD2A2E03FA6C2BA7C325F91A68F4FE3D0287BA135A850983EBA5F5415B85`

`moduleinfo.json` SHA-256:

`15DC49B150C9CD6CA64889A1E7713240EEE78E1AC0398CC96FDF8A1D55963608`

Locations:

- Build: verified local Release VST3 bundle; machine-specific path omitted from public source.
- Output: verified local Plan A formal output; machine-specific path omitted from public source.
- Installed: `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3`
- Pre-install system bundle backup: verified internal rollback copy; machine-specific path omitted from public source.

Cubase and other common DAW/plug-in-host processes were checked and absent immediately before overwrite. No DAW was launched by Codex.

## User validation still required before any Stable promotion

- Cubase scan/load and optional sidechain bus visibility.
- Kick/drum -> carrier EXT routing, Key Gain response, disconnected/silent key behaviour.
- INT sonic/null regression against v1.0.4.
- SC Listen routing and total-latency alignment; true Bypass safety.
- ST/LR/MS key mapping, every Lookahead, 0 ms Oversampling, PDC, Dry/Wet alignment.
- A/B, automation, undo/redo, project restore, and legacy v1.0.4 state migration.
- LIGHT/CLASSIC UI geometry and popup usability.

Plan B, Plan C, Plan D, GitHub sync, Actions, and macOS artifacts were not run.
