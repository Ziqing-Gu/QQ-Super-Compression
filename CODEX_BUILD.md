# QQ Super Compression 0.1.8 — Codex build / validation brief

Read `AI_DEVELOPMENT_HANDOFF.md`, `README.md` and `CHANGELOG.md` before changing anything.

## Open-source release builds

- Windows builds the `QQSuperCompression_VST3` target.
- macOS builds `QQSuperCompression_VST3` for Apple Silicon and Intel separately.
- macOS also builds `QQSuperCompression_AU` as a Universal 2 Audio Unit.
- GitHub Actions definitions live in `.github/workflows/` and use pinned JUCE 8.0.15 through `QQSC_FETCH_JUCE=ON`.
- Do not change DSP, parameter IDs or state behaviour merely to package a release.

Build this source as VST3. **Do not redesign the future-window peak core while fixing compile/API issues.** 0.1.8 is a UI-only follow-up: preserve the 0.1.7 automatic GR Hold and user-confirmed 0.1.6 LUFS Match, remove only the Hold readout's `H` prefix, and verify true uniform 1:1 editor scaling.

## Build priority

1. Prefer an existing local JUCE checkout via `-DJUCE_PATH=...`.
2. If none is available and network access is allowed, CMake can FetchContent JUCE 8.0.15.
3. Build Release target `QQSuperCompression_VST3`.

## 0.1.5 Lookahead presets

The old editable TextEditor is removed. The UI must show exactly:

- 0 ms
- 10 ms
- 26 ms
- 40 ms
- 80 ms
- 100 ms

The existing APVTS parameter ID remains `lookaheadMs` for 0.1.4 candidate compatibility. DSP/UI snap to the approved presets. Legacy arbitrary 0.1.4 state values are migrated to the nearest preset on project restore (exact ties choose the longer preset).

### Per-user default

- Existing project state wins for an existing instance.
- A new instance uses the last Lookahead manually chosen by the user.
- The preference key is `lastLookaheadMs` in `QQSuperCompression.settings`.
- First-run fallback with no stored preference: 26 ms.

Verify that changing an existing project to another user default does **not** overwrite that project's saved Lookahead on reload.

## Host latency / PDC

Selected Lookahead controls both future-window length and exact audio-path delay. Report rounded samples through `setLatencySamples()`.

Bypass must remain on the identical delayed path and keep the same reported latency. Do not reproduce the historical 5035-style Active/Bypass PDC mismatch.

Changing presets during playback can trigger a one-time host PDC realignment.

## Future-window peak core — keep unchanged

The 0.1.4 monotonic sliding-maximum detector remains active. Ratio law remains:

```text
gain = 1 / (1 + (Ratio - 1) * level)
```

Do not restore 20 ms rolling RMS and do not restore the rejected 0.1.0 `abs(x)^(1/Ratio)` waveshaper.

### 0 ms

0 ms remains intentionally distortion-prone and is now an approved flavour option based on user listening. Do not secretly add smoothing/lookahead to it.

## User-confirmed 0.1.4 PluginDoctor observations

Record these as actual user tests, not model simulation:

- 5 ms: sharp reduction above ~99.6 Hz;
- 10 ms: sharp reduction above ~49.8 Hz;
- 20 ms: sharp reduction above ~24.9 Hz;
- 26 ms: boundary around the 20 Hz region;
- 40 ms: lower 20 Hz distortion than 26 ms;
- 80 ms: clearly cleaner again;
- longer windows continue reducing the effect.

0.1.8 should check the six Lookahead presets only for regression because the peak algorithm itself is intentionally unchanged.

## Existing workflow that must remain

- Slim Meter panel.
- Persistent editor size.
- A/B + A→B / B→A.
- Shift fine / Alt reset / Undo / Redo on rotary controls.
- Bypass TextButton.
- Mode cycle ST -> MS -> LR.
- ST common Makeup; LR independent L/R; MS independent M/S.
- Dynamic Display: Dry, Wet pre-Makeup, final post-Mix Output.
- Dual Input / Output / GR meters; MS shows M/S; GR grows downward.

## Match — strict LUFS must remain unchanged

Verify `Source/BS1770LoudnessMatch.h` remains active and is not replaced by RMS/power matching. Required structure:

- BS.1770-compatible K-weighting;
- 400 ms blocks, 100 ms hop / 75% overlap;
- -70 LUFS absolute gate;
- relative gate 10 LU below the absolute-gated result;
- ST stereo programme measurement; LR and MS independent mono component measurements;
- source = Dry vs compressed Wet pre-Makeup/pre-Mix.

Do not simplify Match back to plain RMS while resolving compiler errors.

### Standalone loudness self-test

Before/after the JUCE build, Codex can compile the JUCE-free test:

```text
c++ -std=c++17 -O2 tests/bs1770_match_selftest.cpp -I. -o bs1770_test
```

Expected: 48 kHz full-scale 1 kHz mono sine about -3.01 LUFS and fixed 6 dB Dry/Wet difference about +6 dB Match.


## 0.1.7 automatic Gain Reduction Hold

Verify the GR meter adds a **2 second automatic peak hold** without changing the audio path:

- current GR bars still move normally and grow downward from 0 dB at the top;
- a deeper GR peak moves the white Hold marker immediately and restarts the 2 s timer;
- with no deeper peak, the marker remains for ~2 s then refreshes to current GR automatically;
- ST linked L/R should naturally match; LR L/R are independent; MS M/S are independent;
- Mode and Lookahead changes clear stale Hold values;
- Hold is meter-only: no APVTS parameter, no A/B state, no project-state change, no audio gain change.

The user's previous 0.1.6 build also reported a **non-fatal `playHead` name-shadow warning**. 0.1.7 intentionally names the local pointer `hostPlayHead`; do not reintroduce a local variable named `playHead`.

The panel must show a small low-contrast version identifier (expected `v0.1.8`) derived from JUCE/CMake plug-in version metadata. It should not become a prominent subtitle.

## 0.1.8 Hold readout and true 1:1 UI scaling

This version changes UI presentation only. Verify:

- the second GR number no longer has an `H` prefix; it shows only the held dB number while the white horizontal marker remains the visual Hold cue;
- the Hold numeric font is slightly larger than 0.1.7 and remains visually secondary to the current GR value;
- the editor keeps the original design aspect ratio `1020 / 670` at every user-resized size;
- resizing larger or smaller scales the *entire* UI uniformly: title, version, Dynamic Display, meter panel, meter text/markers, knobs, buttons, ComboBox, fonts, spacing and stroke thicknesses must all change by the same factor;
- no region should merely gain empty/stretch space while other widgets remain at the old pixel size;
- an old saved non-proportional size should reopen at a proportional size rather than reproducing the stretched geometry;
- resize mouse hit-testing remains aligned with the visually scaled controls.

The implementation uses a fixed 1020x670 `contentRoot` with one uniform `AffineTransform`, plus the editor aspect-ratio constrainer. Do not replace this with separate X/Y layout scaling.

## UTF-8 / macOS

Keep source UTF-8, MSVC `/utf-8`, PingFang SC and Microsoft YaHei fallback behaviour.

## Validation status discipline

Append results to `AI_DEVELOPMENT_HANDOFF.md` and distinguish source/static check, compile success, VST3 load, PluginDoctor, Cubase, and user confirmation. Do not mark 0.1.8 Stable unless the user explicitly confirms it.
