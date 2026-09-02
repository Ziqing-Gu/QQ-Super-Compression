# QQ Super Compression 1.1.5 - Plan A Verification

**Date:** 2026-09-02  
**Status:** Plan A complete; Windows x64 VST3 generated and installed for user verification  
**Classification:** Candidate; v1.1.2 remains the Stable rollback baseline

## Implemented scope

- Dynamic Display refresh and history sampling increase from 30 Hz / 240 points to 60 Hz / 480 points while retaining the same approximately eight-second visible window.
- Historical projection uses fixed preallocated arrays, and Dry/Input, Mix-aware GR, Output, and External Key paths are retained in per-domain render caches.
- `paint()` no longer allocates projected history containers or rebuilds every curve path.
- The depth-dependent full-area translucent GR polygon is replaced by one cached sparse shade path capped at 160 segments. The GR boundary and numerical meaning are unchanged.
- Dynamic Display is now an opaque child with a complete canvas fill so its timer repaint does not invalidate the parent editor behind it.
- HPF mouse-release replay, non-mouse debounce duration, latest-request retry, Key Gain real-time projection, audio DSP, parameters, automation IDs, A/B, migration, and state schema 10 are unchanged.

## Build and tests

- Visual Studio 17 2022 / MSVC 19.44 / JUCE 8.0.15 / x64 Release VST3: PASS.
- Twelve Python/source/math self-tests, including the new Display render-performance contract: PASS.
- Standalone BS.1770: PASS (-3.0036 LUFS reference; 6.00 dB Match).
- Steinberg `vst3effectsvalidator`: PASS; Fx and Dynamics classes exposed.
- Binary FileVersion/ProductVersion and module metadata: 1.1.5.
- Compilation completed without warnings or errors.

## Artifact identity and installation

- Bundle: 2 files, 6,745,691 bytes.
- Main binary SHA-256: `ABB9CFD1CF7929C6D4C6A7D9F72226535F36169854A1697B44550047F54B64E1`.
- `moduleinfo.json` SHA-256: `2D5824AFB009FC8DD769C06E883440957A41BAA306B53B4DE846EE759D5B73CE`.
- Windows VST3 ZIP SHA-256: `6A088914F5C4F4875A869F73D3EDEB7CEE78D8C7DF11E4F33AE5D85FBAD61EB0`.
- Build, formal output, and installed binary/module hashes match.
- Formal output: `D:\Codex\Outputs\QQ Super Compression 1.1.5 Plan A 20260902`.
- Installed to `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3` after host processes were confirmed closed.
- The previous installed v1.1.4 bundle is retained under the internal 1.1.5 build directory for rollback.

## User validation boundary

Cubase listening/UI checks remain manual. Compare small and large GR while watching continuous scrolling, then test INT/EXT, ST/LR/MS, LIGHT/CLASSIC, Key Gain drag, repeated HPF release refresh, Threshold, Mix, Match, A/B, state restore, automation, PDC, and true Bypass.

No Plan B/C/D, Stable promotion, GitHub, Actions, macOS build, or Release work was performed.
