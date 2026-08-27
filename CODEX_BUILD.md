# QQ Super Compression 0.9.3 — Codex build / validation brief

Before changing code, read these files in this order:

1. `AI_DEVELOPMENT_HANDOFF.md`
2. `PRODUCT_DESIGN_NOTES.md`
3. `OVERSAMPLING_DESIGN_NOTES.md`
4. `README.md`
5. `CHANGELOG.md`
6. `DEVELOPMENT_HISTORY.md`
7. `docs/TEST_CHECKLIST.md`

Codex does **not** have the design conversation that produced this plug-in. The two dedicated design-note files are therefore authoritative context for GitHub/Release/product writing as well as development decisions.

Build as VST3. Do not redesign the Ratio/future-window algorithm just to solve compiler/API issues.

## Build priority

1. Prefer an existing local JUCE checkout via `-DJUCE_PATH=...`.
2. If none is available and network access is allowed, CMake FetchContent is pinned to JUCE 8.0.15.
3. Build Release target `QQSuperCompression_VST3`.
4. Report compiler warnings separately from errors; do not call a static source check a successful plug-in build.

## Core product intent — do not lose this

QQ Super Compression exists for sources that need dynamic control but where conventional compressor Attack / Release behaviour would undesirably reshape the transient/onset. Typical examples are guitar pick attack, vocal word/consonant onsets, piano hammer attack and bass finger/pick articulation.

The plug-in is not claiming traditional compressors are wrong. It is for the specific use case where the engineer wants compression while preserving the original transient character as much as practical.

This is why the core uses delayed audio + future-window analysis rather than a conventional user Attack/Release envelope. See `PRODUCT_DESIGN_NOTES.md` before writing public GitHub copy.

## 0.1.10 Oversampling — primary change

### Final UI / behaviour

At **Lookahead = 0 ms only**, show `OVERSAMPLING` and one button. Each click cycles:

```text
1x -> 8x -> 16x -> 1x
```

The remembered default is **8x**.

At **Lookahead = 10 / 26 / 40 / 80 / 100 ms**:

- hide the Oversampling label/button;
- force the audio core to **1x** internally;
- keep the user's remembered 0 ms Oversampling choice untouched;
- when Lookahead returns to 0 ms, restore that remembered choice.

### Why 2x / 4x are missing

Do **not** treat this as an incomplete menu.

User PluginDoctor tests found:

- `2x`: aliasing still severe;
- `4x`: aliasing still severe;
- Oversampling added only a small amount of latency in practice.

Therefore the user intentionally rejected 2x/4x and chose the meaningful set `1x / 8x / 16x`. 8x/16x make more sense because their latency cost is small enough not to justify weaker intermediate modes.

Do not re-add 2x/4x unless the user explicitly requests new testing/design work.

### Why only 0 ms uses Oversampling

User PluginDoctor testing found no meaningful aliasing problem at 10 ms or longer Lookahead. Those settings therefore remain 1x. Oversampling at 10 ms+ would add CPU/complexity without solving an observed problem.

### 0 ms meaning

0 ms is intentionally nonlinear/coloured. Oversampling is meant to reduce **alias fold-back**, not remove the harmonic colour itself.

- `1x`: rawest / strongest aliasing;
- `8x`: default practical balance;
- `16x`: further alias reduction.

Do not add hidden smoothing or hidden Lookahead to make 0 ms "clean".

## Oversampling implementation / PDC requirements

- `1x` = JUCE dummy Oversampling path.
- `8x` = maximum-quality linear-phase half-band FIR, 3 stages, integer latency.
- `16x` = same FIR family, 4 stages, integer latency.
- At 0 ms, detector input + Ratio smoother + Ratio gain application run in the selected internal domain.
- At 10 ms+ effective factor must be 1x, so the established future-window detector remains host-rate.
- Makeup, Mix, Integrated LUFS Match accumulation and public meters/display remain host-rate after downsampling.
- Six pre-Makeup Wet streams are still downsampled in parallel (`ST L/R`, `LR L/R`, `MS M/S`) so Match can continue accumulating all processing domains exactly as before.

Reported latency must be:

```text
active Lookahead samples + active Oversampling FIR integer latency
```

Because Oversampling is effective only at 0 ms:

- 0 ms / 1x -> 0-sample Oversampling latency;
- 0 ms / 8x or 16x -> FIR latency only;
- 10 ms+ -> original Lookahead-only latency.

Bypass and the Dry side of Mix must use the identical total delay. Test 50% Mix for combing/time offset.

Changing Lookahead or active 0 ms Oversampling may trigger one host PDC realignment. A setting-change transient is acceptable for this Candidate; do not add an unrequested dual-path crossfade architecture.

## State / A-B / migration

Parameter ID remains `oversampling`, but v0.1.10 changes its choices to `1x/8x/16x` and treats it as the remembered 0 ms choice.

Required behaviour:

- New instance remembered 0 ms choice = `8x`.
- v0.1.8-or-earlier state has no Oversampling parameter -> migrate remembered choice to `8x`.
- v0.1.9 state migration -> old `1x => new 1x`; old `2x/4x/8x => new 8x`.
- A/B snapshots include the remembered 0 ms choice.
- Undo/Redo includes it.
- Moving from 0 ms to non-zero Lookahead must **not** overwrite it.
- Returning to 0 ms must restore it and update PDC.

## UI validation

- Panel version must show subdued `v0.1.10` from JUCE/CMake metadata.
- 0 ms: Oversampling label/button visible; one click cycles 1x/8x/16x.
- 10 ms+: label/button hidden, leaving no misleading quality control visible.
- Existing fixed 1020x670 design root, aspect ratio and uniform scaling must remain intact.
- Mouse hit-testing must stay aligned after resize.
- Confirm the prior non-fatal shadow warnings remain gone: local names should not regress to `playHead` or `constrainer`; current code uses `hostPlayHead` and `editorBoundsConstrainer`.

## PluginDoctor validation

1. `0 ms / Ratio > 1`: compare 1x vs 8x vs 16x aliasing.
2. Expect harmonic colour to remain even at 16x; judge **alias fold-back**, not the existence of harmonics.
3. Verify 2x/4x are not in the UI.
4. `10 / 26 / 40 / 80 / 100 ms`: confirm UI hides Oversampling and effective DSP/PDC is 1x.
5. Ratio `1:1` at 0 ms + 8x/16x: linear response should be essentially flat except expected near-Nyquist FIR roll-off; do not add compensating EQ merely to flatten nonlinear LinearAnalysis at Ratio > 1. See `OVERSAMPLING_DESIGN_NOTES.md`.

## Future-window core — keep unchanged

Approved Lookahead presets:

`0 / 10 / 26 / 40 / 80 / 100 ms`

Ratio law:

```text
gain = 1 / (1 + (Ratio - 1) * level)
```

Do not restore:

- v0.1.0 `abs(x)^(1/Ratio)` sample-domain waveshaper;
- v0.1.1-v0.1.3 fixed 20 ms rolling RMS detector.

User-verified historical PluginDoctor observations that must remain documented:

- 5 ms ~99.6 Hz boundary;
- 10 ms ~49.8 Hz;
- 20 ms ~24.9 Hz;
- 26 ms ~20 Hz region;
- 40 ms cleaner at 20 Hz than 26 ms;
- 80 ms cleaner again.

## Match — strict Integrated LUFS must remain unchanged

`Source/BS1770LoudnessMatch.h` remains the active Match implementation:

- BS.1770-compatible K-weighting;
- 400 ms blocks;
- 100 ms hop / 75% overlap;
- -70 LUFS absolute gate;
- -10 LU relative gate;
- source = delayed Dry vs compressed Wet pre-Makeup/pre-Mix;
- ST stereo programme; LR/MS independent mono-domain measurements.

Do not simplify back to RMS/power.

Standalone sanity test:

```text
c++ -std=c++17 -O2 tests/bs1770_match_selftest.cpp -I. -o bs1770_test
```

Expected: full-scale 1 kHz mono at 48 kHz about -3.01 LUFS; fixed 6 dB Dry/Wet difference about +6 dB Match.

## Existing workflow that must remain

- ST / MS / LR.
- ST common Makeup; LR independent L/R; MS independent M/S.
- A/B + A→B / B→A.
- Shift fine / Alt reset / Undo / Redo.
- Bypass button and correct PDC.
- Dynamic Display = Dry, Wet pre-Makeup, final post-Mix Output.
- Input / Output / GR meters.
- 2-second automatic GR Peak Hold.
- Uniform resize/aspect-ratio system.
- UTF-8 source, MSVC `/utf-8`, CJK font fallback.

## Validation status discipline

Append results to `AI_DEVELOPMENT_HANDOFF.md` / `DEVELOPMENT_HISTORY.md`. Separate:

- source/static check;
- standalone test;
- compiler success;
- VST3 scan/load;
- PluginDoctor result;
- Cubase/PDC result;
- user confirmation.

For this Plan D run, v0.9.3 is the requested stable baseline; retain Candidate labels for historical versions unless explicitly changed.

## v0.9.0 UI-specific verification

Before building or editing v0.9.0, also read `UI_DESIGN_NOTES.md`.

This version is intentionally a UI-only pre-release pass over the v0.1.10 DSP core. Please verify:

- the plug-in reports/displays `v0.9.0`;
- warm ivory/sand background and panel borders render correctly on Windows/macOS;
- rotary controls show a subtle orange/cyan vector glow without clipping;
- text boxes remain readable and editable;
- Lookahead ComboBox and popup menu are readable on the light theme;
- Mode is one click-cycle button (`ST -> MS -> LR -> ST`), not a dropdown;
- A/B active state and Bypass ON state remain visually distinct;
- disabled Match is visibly disabled/readable;
- 0 ms Oversampling button still appears only at 0 ms and cycles 1x/8x/16x;
- resize remains uniform 1020:670 with correct mouse hit targets;
- Dynamic Display traces retain Dry/Wet/Output semantics;
- Meter ranges/orientation and 2 s GR Hold remain unchanged;
- no DSP regression relative to v0.1.10.


## v0.9.1 lighting-refinement verification

Build v0.9.1 exactly as the previous candidate, then verify the visual change in a real host rather than judging only source constants.

- panel version must read `v0.9.1`;
- the v0.9.0 layout must not move;
- Ratio / Makeup / Mix arcs should now read as **light**, with a visible but soft warm pool below the knob;
- active A/B, Mode, Bypass and 0 ms Oversampling states should read as softly back-lit, not merely outlined;
- no glow may clip at minimum/max editor size;
- ivory background must remain clean and light (do not darken it to make glow easier to see);
- confirm MSVC C4459 for `lookaheadMs` is gone;
- confirm no new warnings are introduced by `UTF8LookAndFeel.h`;
- verify Mode still cycles `ST -> MS -> LR -> ST`;
- verify Lookahead, 0 ms Oversampling visibility, Match, A/B, PDC, Mix and Bypass behave exactly as v0.9.0 / 0.1.10.

If the glow is still too weak/strong, report a screenshot and tune only the alpha/width/material constants before changing layout.


## v0.9.2 asset-knob / I-O Gain verification

Build v0.9.2 as a new Candidate. This is the first version with an embedded BinaryData PNG filmstrip and new audio parameters, so verify both build-system and runtime behaviour.

Required checks:

- CMake/JUCE version and panel must report `0.9.2`.
- `QQSCAssets` BinaryData target must compile and link; no external PNG file is required at runtime.
- All rotary controls display the 128-frame asset and remain draggable/editable.
- Direct text entry must accept decimal values such as `7.95 dB`; text must remain crisp and independent of the bitmap.
- Visual frame mapping must not quantise DAW automation or parameter values.
- Frame 0 and 127 pointer visibility; 20% approximate partial arc; 100% full usable arc.
- Input Gain default 0 dB; current Candidate range -24..+24 dB.
- Input Gain changes detector/compression behaviour and Input meters, but the Dynamic Display Dry/Input trace must not move merely because Input Gain changes.
- Output Gain must change final audio, Output meters and Dynamic Display Output trace.
- LUFS Match must still write Makeup only; Output Gain must not contaminate Match calculation.
- Bypass must remain sample-aligned/PDC-correct and bypass both new trims.
- A/B copy/select, project save/restore and Undo/Redo must include Input/Output Gain.
- Load a pre-0.9.2 state and confirm both new trims migrate to 0 dB.
- Confirm the old C4459 lookahead shadow warning remains absent.

Do not call v0.9.2 Stable/Release until the user explicitly confirms it.


## v0.9.3 UI rollback verification

This Candidate intentionally keeps v0.9.2 DSP/state features but restores v0.9.1 vector rotary rendering. Verify:

- build has **no `QQSCAssets` BinaryData dependency**;
- panel version is `v0.9.3`;
- rotary controls render with the v0.9.1 code-drawn warm-light LookAndFeel, not bitmap filmstrip frames;
- Input Gain / Output Gain are still present and editable as smaller secondary trims;
- Input Gain changes compression/Input meters but not Dynamic Display Dry/Input reference;
- Output Gain changes final audio/Output meters/Dynamic Display Output;
- A/B, save/restore and Undo/Redo still include both trims;
- Ratio / Lookahead / Oversampling / PDC / Bypass / LUFS Match / GR Hold regressions remain unchanged.

Do not remove the archived v0.9.2 asset-history documents merely because they are no longer active.
