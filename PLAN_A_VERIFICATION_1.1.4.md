# QQ Super Compression 1.1.4 - Plan A Verification

**Date:** 2026-09-02
**Status:** Plan A complete; Windows x64 VST3 generated and installed for user verification
**Classification:** Candidate; v1.1.2 remains the Stable rollback baseline

## Implemented scope

- The newest HPF history request is retained; stale work is cancelled and transient snapshot failures retry up to three times.
- Mouse-wheel, text-entry, automation, preset, and A/B stability debounce is reduced from four Display ticks to two.
- Replay copies only the visible history plus a one-second maximum pre-roll instead of always copying the complete ten-second ring.
- Replay runs only the two peak-window engines required by the current ST/LR/MS domain instead of all four.
- A subtle `HPF UPDATING` header status remains visible until the latest successful result is applied or bounded retries are exhausted.
- Key Gain remains real-time. Audible HPF/DSP, parameters, automation IDs, A/B, migration, and state schema 10 are unchanged.

## Build and tests

- Visual Studio 17 2022 / MSVC 19.44 / JUCE 8.0.15 / x64 Release VST3: PASS.
- Eleven Python/source/math self-tests: PASS.
- Standalone BS.1770: PASS (-3.0036 LUFS reference; 6.00 dB Match).
- Steinberg vst3effectsvalidator: PASS; Fx and Dynamics classes exposed.
- Binary FileVersion/ProductVersion and module metadata: 1.1.4.
- Same-machine replay microbenchmark: old four-engine/full-ring core about 19-26 ms; optimized two-engine/trimmed-range core about 8-13 ms, typically about 55% lower.

## Artifact identity and installation

- Bundle: 2 files, 6,744,155 bytes.
- Main binary SHA-256: `05A41D64AC1CA45A7EF89F34E6BE946A18E83323308A24A496C89EF381504731`.
- moduleinfo.json SHA-256: `9E0B76643D69A39E0E0004A83CF7F7F584A0F81ADE8D82266187DE508E8FFD31`.
- Windows VST3 ZIP SHA-256: `2497BE2B66BC645EEEF4077338031F139CCC9259CA4DD9EC8F697B1A65EC019D`.
- Build/output/install binary and moduleinfo hashes match.
- Installed to `C:\Program Files\Common Files\VST3\QQ Super Compression.vst3` after host processes were confirmed closed.
- The previous installed v1.1.3 bundle is retained as an internal rollback copy.

## User validation boundary

Cubase listening/UI checks remain manual. Repeatedly change HPF and confirm the latest release always updates history, `HPF UPDATING` clears, Key Gain remains continuous, Display stays responsive, and INT/EXT, ST/LR/MS, both themes, SC Listen, Match, PDC, A/B, state restore, automation, and true Bypass remain correct.

No Plan B/C/D, Stable promotion, GitHub, Actions, macOS build, or Release work was performed.
