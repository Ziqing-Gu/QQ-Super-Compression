# CHANGELOG

## 1.0.1 — Display 0…-90 dB Scale Polish — Stable baseline

- Returned to the clean future-window peak / Lookahead core after rejecting the 1.0.0 Direct/Analytic/Hilbert experiment for harmonic colour and high CPU/ASIO Guard cost.
- Added optional Threshold as a continuous lower boundary; Threshold OFF preserves the legacy QQ gain law exactly and does not change the detector.
- Added independent LR/MS Ratio, Threshold, Makeup, and Mix while ST remains a common linked domain.
- Added Relative LINK for Ratio, Threshold, and Makeup with offset preservation; Mix remains independent.
- Enlarged the editor to a 1020×820 display-first layout with stacked LR/MS histories.
- Fixed the Dynamic Display to a drawing-only 0…-90 dB range with 15 dB grid spacing; DSP, meters, loudness, parameter ranges, and the -120 dB OFF sentinel are unchanged.
- Windows JUCE/MSVC build, source manifest, project self-tests, installation, and hash parity passed before Plan C.
- Promoted to Stable by the user's standing Plan D rule; final Cubase listening/UI/automation/state-migration checks remain user-side verification.

## 0.9.4 — Editable Numeric Text Contrast — Stable baseline

- Fixed unreadable white text when double-clicking a slider value for direct numeric entry on the light ivory UI.
- Explicitly styled JUCE `Label` edit-state and internal `TextEditor` colours: dark warm text, ivory background, warm focus/caret, and a readable warm selection highlight.
- No layout, parameter, DSP, Input/Output Gain, Display, meter, state, A/B, Undo/Redo, Lookahead, Oversampling, PDC, LUFS Match or GR Hold behaviour changed.
- Plan D stable baseline under the user release policy; Windows build/install checks passed, and cross-platform Actions validation is recorded by the release commit.

## 0.9.3 — UI Rollback / Features Retained — Candidate

- Rejected the v0.9.2 runtime bitmap-filmstrip knob appearance after real user/DAW visual testing.
- Restored the v0.9.1 `UTF8LookAndFeel` vector rotary rendering and warm-light/material style.
- Removed the active `QQSCAssets` BinaryData target/link from CMake; rejected PNG assets remain preserved as development history only.
- Kept v0.9.2 Input Gain / Output Gain parameters, signal flow, Display semantics, meters, A/B, project state and Undo/Redo unchanged.
- Kept Input/Output as smaller secondary trims in the bottom control row; Ratio / Makeup / Mix remain the visual focus.
- No Ratio / Lookahead / Oversampling / PDC / LUFS Match / GR Hold DSP redesign.
- Candidate only; requires Codex build and user visual/functional verification.

## 0.1.10 — 0 ms-Only 1x/8x/16x Oversampling / Design Documentation — Candidate

- Finalised Oversampling around the user-verified need instead of exposing a generic quality menu at every Lookahead.
- Lookahead `0 ms` now alone exposes Oversampling; the UI is a single button cycling `1x -> 8x -> 16x -> 1x`. The remembered default is `8x`.
- Lookahead `10 / 26 / 40 / 80 / 100 ms` forces the DSP to `1x` and hides the Oversampling control while preserving the user's last 0 ms choice.
- Removed user-facing `2x` and `4x` choices intentionally: user PluginDoctor tests found aliasing remained severe at both factors, while the added latency of `8x`/`16x` was small enough that the intermediate factors had little practical value.
- Added a `16x` maximum-quality linear-phase FIR Oversampling path with integer latency compensation.
- Host PDC / compensated Dry / Bypass continue to use `Lookahead + active FIR latency`; non-zero Lookahead returns to Lookahead-only latency because Oversampling is effectively 1x.
- New/legacy state default for the remembered 0 ms choice is `8x`. v0.1.9 migration maps old `1x -> 1x` and old `2x/4x/8x -> 8x`. A/B snapshots and Undo/Redo keep the remembered 0 ms choice.
- Replaced the v0.1.9 Oversampling ComboBox with a compact click-to-cycle button that is hidden outside 0 ms.
- Added `PRODUCT_DESIGN_NOTES.md` explaining the core product motivation: compress dynamics while avoiding unwanted conventional Attack/Release reshaping of transients/onsets on sources such as guitar, vocals, piano and bass.
- Added `OVERSAMPLING_DESIGN_NOTES.md` documenting the measurement history and why 2x/4x are deliberately absent.
- Ratio law, strict Integrated LUFS Match, six Lookahead presets, ST/MS/LR topology, Makeup/Mix, two-second GR Hold, uniform UI scaling and internal product identity remain unchanged.

## 0.1.9 — FIR Oversampling / PDC Alignment / Warning Cleanup — Candidate

- Added user-selectable `OVERSAMPLING`: `1x / 2x / 4x / 8x`, default `1x`.
- `2x / 4x / 8x` use maximum-quality linear-phase half-band FIR oversampling with integer latency compensation; `1x` uses the dummy path.
- Oversampling encloses the future-window detector, Ratio smoothing and Ratio gain application. Lookahead keeps the same six millisecond meanings; internal Lookahead samples are the rounded base Lookahead samples multiplied by the selected factor.
- Downsampling carries simultaneous ST linked L/R, LR independent L/R and MS M/S pre-Makeup Wet streams so strict Integrated LUFS Match can continue accumulating every mode domain in parallel.
- Host latency/PDC now reports `Lookahead + Oversampling FIR latency`; the host-rate Dry/Mix/Bypass path uses the same combined delay.
- Oversampling is stored in project state and A/B snapshots and participates in normal APVTS Undo/Redo. New instances and 0.1.8-or-earlier states with no Oversampling field explicitly migrate to `1x`; no last-used Oversampling preference is introduced.
- Added `OVERSAMPLING` ComboBox beneath Lookahead without redesigning the rest of the 1020x670 uniform UI. Panel metadata now reports `v0.1.9`.
- Renamed the local editor constrainer pointer to `editorBoundsConstrainer`, addressing the non-fatal `constrainer` name-shadow warning reported from the 0.1.8 build.
- Ratio law, six Lookahead presets, 0 ms flavour semantics, strict LUFS Match definition, Makeup/Mix topology, two-second GR Hold and product/internal identity remain otherwise unchanged.

## 0.1.8 — GR Hold Readability / Uniform 1:1 UI Scaling — Candidate

- Removed the redundant `H` prefix from the Gain Reduction Hold numeric readout and increased the Hold value font from 7.5 px to 8.5 px; the white Hold marker and 2-second Hold algorithm are unchanged.
- Moved all plug-in widgets into one fixed 1020x670 design-space root and uniformly scale that root for user resizing, so fonts, meters, knobs, strokes and spacing all grow/shrink by the same X/Y factor.
- Added a fixed 1020:670 editor aspect-ratio constrainer. Width and height can no longer be stretched independently.
- Existing saved non-proportional editor sizes are migrated to the largest uniform scale that fits within the previously saved rectangle; subsequent saved sizes remain proportional.
- Kept 0.1.7 GR Hold timing, 0.1.6 strict LUFS Match, future-window peak compression, Ratio, Lookahead/PDC, ST/MS/LR, A/B, Makeup, Mix and all audio DSP unchanged.
- CMake/JUCE version bumped to `0.1.8`; Project/parameter/state schema is unchanged.

## 0.1.7 — Auto GR Peak Hold / Version Tag / Warning Cleanup — Candidate

- Added a meter-only 2 second automatic Gain Reduction Peak Hold for both displayed GR channels/components.
- A deeper GR peak updates the Hold immediately and restarts the 2 s timer; after 2 s without a deeper peak the Hold automatically refreshes to current GR.
- ST linked mode holds the linked GR; LR holds L/R independently; MS holds M/S independently.
- Added a small horizontal Hold marker plus a subdued `H` value without changing current GR meter movement/orientation.
- Mode or Lookahead changes clear stale GR Hold values.
- Renamed the local host playhead pointer from `playHead` to `hostPlayHead` to remove the non-fatal name-shadow warning reported by the user's 0.1.6 build.
- Added a small, low-contrast panel version label sourced from `JucePlugin_VersionString`.
- Strict LUFS Match, future-window peak Ratio DSP, Lookahead presets/PDC, A/B, Makeup, Mix and processing modes are unchanged.
- Candidate only; requires Codex build and Cubase visual/timing verification.

## 0.1.6 — Strict Integrated LUFS Match — Candidate

- Replaced the old integrated power/RMS-equivalent Match with ITU-R BS.1770 / EBU R128 style Integrated LUFS measurement.
- Added K-weighting, 400 ms gating blocks, 75% overlap (100 ms hop), -70 LUFS absolute gate and -10 LU relative gate.
- ST Match measures stereo Dry vs linked Wet and writes one common Makeup.
- LR and MS Match use independent mono gated-LUFS calculations and write only channels/components with valid gated data.
- Measurement remains Dry vs compressed Wet pre-Makeup/pre-Mix.
- Future-window peak Ratio core, Lookahead presets, A/B, Mix, UI and processing modes are unchanged.
- Candidate only; requires Codex build and Cubase/independent loudness-meter validation.


## 0.1.5 — Fixed Lookahead Presets / Last-Choice Memory — Candidate

- Replaced the 0.1.4 arbitrary Lookahead TextEditor with a ComboBox containing exactly 0 / 10 / 26 / 40 / 80 / 100 ms.
- Kept the 0.1.4 future-window peak detector and Ratio law unchanged.
- Preserved the existing `lookaheadMs` parameter ID; old arbitrary 0.1.4 candidate values are snapped to the nearest approved preset on state restore.
- Existing instances restore their project-saved preset; new instances default to the user's last manually selected preset.
- Added per-user `lastLookaheadMs` persistence in the existing QQSuperCompression settings file.
- First-run fallback is 26 ms.
- Recorded actual user PluginDoctor findings that motivated the six presets, including 0 ms as a retained distortion/flavour option.
- Bypass/PDC still use the exact selected Lookahead delay.
- Strict LUFS Match remains pending and is intentionally not mixed into this focused change.

## 0.1.4 — Variable Lookahead Peak Experiment — Candidate

- Replaced the active fixed 20 ms rolling RMS detector after user PluginDoctor tests showed Attack/Release-like timing and residual harmonic distortion.
- Added editable `LOOKAHEAD (ms)` text field, 0.0–100.0 ms, default 5.0 ms.
- Lookahead now controls both the future analysis window and the real audio-path latency/PDC.
- Added an allocation-free sliding future-window peak detector for L/R/M/S domains.
- Bypass remains on the same delayed path and reports the same latency as active processing.
- Lookahead is stored in project state and included in A/B snapshots.
- Retained Ratio law, Makeup topology, Mix, ST/MS/LR modes, meters, Dynamic Display, window-size memory and workflow features.
- Match intentionally remains the old RMS/power prototype; strict LUFS Match is a separately confirmed pending requirement and was not mixed into this experiment.
- 0 ms is intentionally retained as a distortion-prone instantaneous comparison point.

## 0.1.3 — Workflow / A-B / Match / Independent Makeup — Candidate

- Removed the subtitle text under the plug-in title.
- Slimmed the dual-meter panel while keeping its height unchanged.
- Added persistent per-user editor size memory.
- Added A/B comparison plus A→B and B→A copy actions.
- Added JUCE UndoManager integration and Ctrl/Cmd+Z / Ctrl/Cmd+Shift+Z handling.
- Added Shift-drag fine adjustment and Alt+left-click default reset to all rotary controls.
- Replaced checkbox-style Bypass appearance with a TextButton.
- Replaced Mode ComboBox with one cycle button: ST -> MS -> LR -> ST.
- Kept ST shared Makeup; added independent L/R Makeup in LR and independent M/S Makeup in MS.
- Added integrated energy Match based on Dry vs Wet pre-Makeup over host playback; LR/MS are calculated independently per domain channel.
- Kept 0.1.2 Ratio engine, 20 ms detector, zero latency, Dynamic Display trace meanings and dual meters unchanged.

## 0.1.2 — ST/MS/LR Modes / Zero Latency / Dual Meters — Candidate

- Removed 10 ms Stable latency mode; plugin is fixed 0-sample latency.
- Removed Makeup Gate and all related UI/processing.
- Added ST / MS / LR processing modes.
- Added dual-channel Input, Output and Gain Reduction meters.
- MS mode meters show M/S instead of L/R.
- Gain Reduction meter orientation changed to 0 dB at top, growing downward.
- Dynamic Display retains Dry, Wet pre-Makeup and final post-Mix Output; Gate line removed.
- Kept 0.1.1 threshold-free level-domain Ratio engine.

## 0.1.1 — Ratio Engine Fix / Meter & UI Pass — Candidate

- Replaced rejected sample-domain waveshaper Ratio with level-domain gain control.
- Added three meter groups.
- Improved Dynamic Display layout.
- Added stronger UTF-8/CJK handling.

## 0.1.0 — Prototype — Test

- Initial Ratio / Makeup / Makeup Gate / Mix / 0-or-10-ms prototype.
- Ratio implementation was rejected because it behaved as a waveshaper, increased level with Ratio and produced severe distortion.

## 0.9.1 — Lighting & Material Refinement Candidate

- Keeps the accepted v0.9.0 layout and all v0.1.10 DSP behaviour unchanged.
- Strengthens the knob "lamp under glass" system so it is visible in a real DAW: wider soft halo, medium bloom, crisp lit arc, bright endpoint lamp, warm panel spill and reflected light inside the knob body.
- Strengthens active-button back-lighting while keeping the same warm/cyan semantic colour roles.
- Adds low-contrast panel material depth using a slight vertical ivory gradient, fine highlight rim and softer border/shadow; geometry is unchanged.
- Fixes the user-reported non-fatal MSVC C4459 naming warning by renaming conflicting `lookaheadMs` argument/local identifiers only.
- Version metadata -> `0.9.1`. Still Candidate/Test; v1.0.0 remains reserved for explicit final user confirmation.

## 0.9.0 — Warm Transparent UI Candidate

- Version line intentionally jumps from the 0.1.x development series to **0.9.0** as the pre-release UI/polish stage. v1.0.0 is reserved for the user's final release confirmation.
- Replaces the dark prototype visual language with a warm ivory / sand chassis, dark warm-grey text, apricot/orange primary accents and cyan technical/Wet accents.
- Adds a custom scalable JUCE vector LookAndFeel for soft "lamp under glass" knob/button illumination using layered low-alpha strokes rather than bitmap blur assets.
- Keeps Dynamic Display behaviour/geometry unchanged; only its background, grid, text and Dry/Wet/Output palette are updated.
- Updates the meter panel to the same light/warm material language while preserving all meter and 2-second GR Hold semantics.
- Mode presentation is the user-approved compact single cycle button (`ST -> MS -> LR -> ST`), not a dropdown.
- Adds `UI_DESIGN_NOTES.md` so future Codex/AI can understand why the UI is intentionally clean, transparent and warm, and why the earlier dark cyberpunk concept was rejected.
- **DSP unchanged from 0.1.10:** Ratio, Lookahead, 0 ms Oversampling, PDC, LUFS Match, A/B, Makeup/Mix, ST/MS/LR and GR Hold are not redesigned in this version.

## v0.9.2 — Asset Knobs / Input & Output Gain (Candidate) — 2026-08-27

- Rotary rendering architecture changed from generated vector glow to an embedded 128-frame transparent PNG filmstrip.
- 128 frames map to normalized 0..127 visual states while APVTS parameters remain continuous/high precision.
- Numeric values/units remain editable JUCE text and are not baked into knob images.
- Progressive lamp rule: lit arc accumulates from minimum toward the current pointer; frame 127 lights the full usable arc; pointer remains visible at both endpoints.
- Added Input Gain and Output Gain trims (candidate range -24..+24 dB, default 0 dB), appended after existing parameters for legacy parameter-order safety.
- Input Gain is pre-detector/pre-compression and changes compression behaviour, but Dynamic Display Dry/Input remains the original pre-Input-Gain reference.
- Output Gain is final post-Mix trim and is included in Dynamic Display Output and Output meters.
- True Bypass keeps combined PDC but bypasses Input/Output Gain and the processing path.
- Input/Output Gain are included in project state, A/B snapshots and Undo/Redo; pre-0.9.2 states migrate both trims to 0 dB.
- Existing DSP Ratio/Lookahead/Oversampling/LUFS Match/GR Hold design is otherwise unchanged.
- Button visuals remain vector/JUCE in this candidate pending separate asset approval.
