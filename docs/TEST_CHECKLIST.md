# QQ Super Compression 1.0.4 — Stable Baseline Checklist

**Stable baseline:** v1.0.4 Light / Classic UI switch (user-confirmed Stable on 2026-09-01).

## Build / identity

- [x] Windows Release VST3 compiles without new warnings.
- [x] Cubase scans and loads it.
- [x] Panel and binary metadata show `v1.0.4`.

## v1.0.4 visual themes

- [x] The upper-right `LIGHT` / `CLASSIC` control switches only the visual theme.
- [x] LIGHT preserves the current warm ivory interface and CLASSIC uses the restrained dark charcoal/cyan control style.
- [x] Both themes keep the same 1020x820 layout, controls, hit targets, parameters and audio behaviour.
- [x] The last selected theme is stored locally and restored when the editor is reopened.
- [x] Theme selection remains outside DSP, APVTS, host automation, A/B snapshots and project state.

## Centered Domain Monitor

- [ ] ST hides Monitor.
- [ ] LR shows `ALL / L / R`; MS shows `ALL / M / S`.
- [ ] LR ALL and MS ALL reproduce the normal v1.0.2 stereo result.
- [ ] L is centered same-polarity to both outputs and uses -3.0103 dB (`1/sqrt(2)`) listening compensation.
- [ ] R is centered same-polarity to both outputs and uses the same compensation.
- [ ] M is centered same-polarity at **unity**, with no extra -3.01 dB.
- [ ] S uses `(L-R)/2`, is centered same-polarity, and uses -3.0103 dB compensation.
- [ ] Display/Meter levels do not drop merely because L/R/S centered monitor compensation is active.
- [ ] Match result is unchanged by Monitor selection.
- [ ] True Bypass ignores Monitor and stays the normal latency-aligned bypass.
- [ ] LR and MS remember different Monitor selections when switching modes.
- [ ] Save/reopen project restores both selections.
- [ ] A/B switch and A<->B copy do not change Monitor selection.
- [ ] DAW automation/parameter list has no new Monitor parameter.
- [ ] Mode and Lookahead remain 108x23 and aligned; LINK remains 34x23; Display height is unchanged.
- [ ] At 0 ms, Monitor row and Oversampling control both fit without overlap.

## v1.0.2 regression

- [ ] LINK covers Ratio / Threshold / Makeup / Mix in LR/MS.
- [ ] LINK drag, Shift fine drag and direct numeric entry preserve relative differences and shared boundaries.
- [ ] Threshold OFF migration/semantics unchanged.
- [ ] Independent LR/MS Mix unchanged.
- [ ] Transparent future-window DSP / Lookahead unchanged.
- [ ] 0 ms 1x/8x/16x Oversampling and PDC unchanged.
- [ ] Display 0…-90 dB scale unchanged.

--- HISTORICAL CHECKLIST BELOW ---

# QQ Super Compression 0.1.9 Test Checklist

## 0.1.9 Oversampling / PDC / warning cleanup

- [ ] Panel version reads small/low-contrast `v0.1.9`.
- [ ] `OVERSAMPLING` menu shows exactly `1x / 2x / 4x / 8x`; first/default is `1x`.
- [ ] A fresh instance does **not** inherit the previous user's Oversampling choice; it starts at `1x`.
- [ ] A 0.1.8 project/state with no Oversampling field opens at `1x` while retaining its old Ratio/Makeup/Mix/Lookahead/Mode values.
- [ ] A 0.1.9 project restores its saved Oversampling choice.
- [ ] A/B and A→B/B→A include Oversampling.
- [ ] Ctrl/Cmd+Z and Ctrl/Cmd+Shift+Z restore Oversampling and host PDC correctly.
- [ ] `1x` with identical settings matches 0.1.8 behaviour and Lookahead-only latency.
- [ ] `2x / 4x / 8x` report Lookahead + FIR latency to the host.
- [ ] Bypass and Mix Dry use the same total delay as Wet; no Active/Bypass PDC offset.
- [ ] Ratio 1:1, Makeup 0 dB, Mix 0% gives pure delayed Dry at each factor.
- [ ] Mix around 50% does not show a timing-offset comb caused by uncompensated Oversampling latency.
- [ ] At Lookahead 0 ms, PluginDoctor comparison across 1x/2x/4x/8x shows progressively reduced alias fold-back without requiring the intentional harmonic flavour itself to disappear.
- [ ] Repeat the oversampling spectrum comparison at 10 ms.
- [ ] 26/40/80/100 ms remain selectable and functional at every factor.
- [ ] ST/LR/MS output and strict LUFS Match still work after factor changes; Match remains Dry vs compressed Wet pre-Makeup/pre-Mix.
- [ ] GR current meter and 2 s Hold still work; changing Oversampling clears an old Hold marker.
- [ ] Factor changes during playback may cause one setting-change/PDC transition but do not create a persistent alignment error.
- [ ] No non-fatal local-name shadow warning remains for `playHead` or `constrainer`.
- [ ] CPU scaling is observed/documented, especially 8x; no expectation that 8x is cheap.


## Build / load

- [ ] Release VST3 compiles.
- [ ] VST3 scans and loads in DAW.
- [ ] Host-reported latency equals selected Lookahead samples + selected Oversampling FIR latency (1x adds 0).
- [ ] Bypass reports and uses the identical combined latency/PDC.

## UI layout

- [ ] The removed subtitle is not visible anywhere below the title.
- [ ] Meter panel is noticeably narrower than 0.1.2 while keeping the same height.
- [ ] Dynamic Display receives the freed horizontal space.
- [ ] No labels/buttons overlap at minimum editor size.
- [ ] Bypass is a normal button, not a checkbox control.
- [ ] Mode is a single button; each click cycles ST -> MS -> LR -> ST.

## Editor size memory

- [ ] Resize the plugin, close the editor, reopen it: width/height are restored.
- [ ] Open another new instance: the last saved user size is used.
- [ ] Saved size is clamped to the supported resize limits.
- [ ] Verify settings path works on Windows.
- [ ] Verify settings path works on macOS without PropertiesFile assertion.

## Knob interaction / Undo

For Ratio, all active/inactive mode Makeup knobs, and Mix:

- [ ] Normal drag works.
- [ ] Shift + drag is clearly finer than normal drag.
- [ ] Alt + left click returns the parameter to its default.
- [ ] Ctrl+Z (Windows) / Command+Z (macOS) undoes the latest parameter edit.
- [ ] Ctrl+Shift+Z / Command+Shift+Z redoes it.
- [ ] Cubase does not swallow these shortcuts when the plugin has keyboard focus, or any conflict is documented.

## A/B

- [ ] A and B initially behave consistently.
- [ ] Edit A, switch to B, edit B, then switch A/B: each state returns correctly.
- [ ] A→B copies all sound parameters from A into B.
- [ ] B→A copies all sound parameters from B into A.
- [ ] Ratio, all Makeup values, Mix and Mode are part of the snapshot.
- [ ] Bypass remains global and is not changed by A/B.
- [ ] Save/reload project: A, B and the active slot persist.

## Ratio / distortion regression

- [ ] Ratio 1:1, Makeup 0 dB, Mix 100% is unity before intended detector behaviour.
- [ ] Increasing Ratio lowers Wet and increases GR.
- [ ] 1 kHz sine does not turn into the rejected 0.1.0 waveshaper distortion.
- [ ] 0.1.3 sounds the same as 0.1.2 when using equivalent ST settings and Makeup.

## ST mode

- [ ] Meter labels show L/R.
- [ ] ST uses one common Makeup knob.
- [ ] GR L and GR R display the same linked reduction.
- [ ] Stereo image remains stable under asymmetric input.

## LR mode

- [ ] Meter labels show L/R.
- [ ] L and R compression can differ.
- [ ] Two separate Makeup controls labelled L/R appear.
- [ ] Changing L Makeup only changes L processed path.
- [ ] Changing R Makeup only changes R processed path.

## MS mode

- [ ] Meter labels show M/S.
- [ ] Two separate Makeup controls labelled M/S appear.
- [ ] M Makeup is applied in M/S domain before decode.
- [ ] S Makeup is applied in M/S domain before decode.
- [ ] Pure centred mono signal produces Side near -inf.
- [ ] Pure anti-phase signal produces Mid near -inf.

## Match

- [ ] Start playback and let a useful section/song play; Match becomes available after data is accumulated.
- [ ] ST Match sets the one ST Makeup value.
- [ ] LR Match writes different L/R Makeup values when channel energy/reduction differs.
- [ ] MS Match writes different M/S Makeup values when Mid/Side energy/reduction differs.
- [ ] Match uses Dry vs Wet pre-Makeup and is not changed by the current Mix value.
- [ ] Match does not include current Makeup in its measurement result.
- [ ] Repeated Stop -> Play passes reset/restart accumulation as intended in Cubase.
- [ ] Seek/loop/discontinuous transport does not accidentally mix unrelated passes; document any issue.
- [ ] Match is gated Integrated LUFS (K-weighted BS.1770/EBU R128 structure), not plain RMS/power.

## Gain Reduction orientation

- [ ] 0 dB GR rests at the top.
- [ ] Increasing GR extends downward, not upward.

## Dynamic Display

- [ ] Dry/Input, Wet pre-Makeup and Output traces are visible.
- [ ] Yellow Output is the final signal after Makeup + Mix.
- [ ] Changing Mix changes yellow Output while cyan Wet pre-Makeup stays the full compressed result.

## UTF-8 / Windows / macOS

- [ ] Source remains UTF-8.
- [ ] Windows build uses `/utf-8`.
- [ ] UI fonts render normally on Windows.
- [ ] UI fonts render normally on macOS.


---

## 0.1.4 variable Lookahead experiment

- [ ] LOOKAHEAD TextEditor accepts 0.0–100.0 ms and clamps invalid values.
- [ ] Default new-instance Lookahead = 5.0 ms.
- [ ] Project save/reload restores Lookahead.
- [ ] A/B snapshots recall/copy Lookahead.
- [ ] Host-reported latency equals rounded Lookahead samples.
- [ ] Bypass remains delayed by exactly the same amount and stays PDC-aligned.
- [ ] PluginDoctor Dynamics no longer shows the old 20 ms causal rolling-RMS attack/release shape.
- [ ] PluginDoctor Harmonic Analysis: compare 0/1/2/5/10/20/50 ms at 50, 100, ~521.5, 1k and 5k Hz.
- [ ] Verify stable sine distortion reduces as the window becomes long enough to contain representative waveform peaks.
- [ ] Changing Lookahead while playing may trigger one host/PDC realignment but must settle to correct steady-state delay.
- [ ] Existing ST/MS/LR, independent Makeup, meters, Dynamic Display, A/B, Undo/Redo and window-size memory still work.
- [ ] Match is still RMS/power in this candidate; do not mark strict LUFS as complete.


---

## 0.1.5 fixed Lookahead presets / persistence

- [ ] Lookahead is a ComboBox, not an editable TextEditor.
- [ ] Choices are exactly 0 / 10 / 26 / 40 / 80 / 100 ms.
- [ ] First run with no saved user preference starts at 26 ms.
- [ ] Manually choose 40 ms, insert a new instance: new instance starts at 40 ms.
- [ ] Save a project instance at 10 ms, later change the user default to 80 ms in another/new instance, reload the project: the saved instance still restores 10 ms.
- [ ] 0.1.4 arbitrary saved value migrates to the nearest fixed preset; 5 ms specifically migrates upward to 10 ms, not down to the 0 ms flavour mode.
- [ ] A/B snapshots recall/copy only legal Lookahead presets.
- [ ] Host latency equals rounded samples for each selected preset.
- [ ] Bypass uses the identical delay/PDC for each preset.
- [ ] PluginDoctor regression confirms six presets map to the same future-window peak behaviour as 0.1.4.
- [ ] 0 ms remains measurably distortion-prone by design and is not secretly smoothed.
- [ ] Existing ST/MS/LR, Makeup, Mix, Dynamic Display, meters, A/B, Undo/Redo and window-size memory remain unchanged.
- [ ] Match is still RMS/power in this candidate; strict LUFS remains pending.


## 0.1.6 strict LUFS Match

- [ ] Confirm MATCH does not use plain RMS/power matching.
- [ ] Compare ST Integrated LUFS against a trusted BS.1770/EBU R128 meter using the same playback range.
- [ ] Use a known fixed gain difference: Dry vs Wet differing by about 6 dB should produce about +6 dB Makeup when both remain well above the gates.
- [ ] Verify LR calculates/writes L and R independently.
- [ ] Verify MS calculates/writes M and S independently.
- [ ] Verify a silent/invalid channel (for example silent Side) is left unchanged rather than reset to 0 dB.
- [ ] Verify Match source is Wet pre-Makeup/pre-Mix, so changing existing Makeup/Mix does not change the measured Match target.
- [ ] Verify Stop -> Play starts a new Integrated measurement and incomplete final 400 ms blocks are not counted.


## 0.1.7 automatic GR Hold / version / warning cleanup

- [ ] Panel shows a small, low-contrast `v0.1.7` and it does not visually behave like the removed subtitle.
- [ ] Build no longer reports the prior local `playHead` name-shadow warning.
- [ ] Current GR bars continue moving normally; Hold does not freeze the bar.
- [ ] A new deeper GR peak immediately updates the Hold marker/value.
- [ ] A new deeper peak restarts the ~2 s hold timer.
- [ ] With no deeper peak for ~2 s, Hold automatically refreshes to current GR.
- [ ] ST linked mode shows matching L/R Hold values.
- [ ] LR mode holds L/R independently.
- [ ] MS mode holds M/S independently.
- [ ] Changing Mode clears stale Hold from the previous L/R or M/S domain.
- [ ] Changing Lookahead clears stale Hold from the previous detector window.
- [ ] GR Hold has no audible effect and does not change Match, Makeup, Mix, A/B, project parameters or PDC.
- [ ] 0.1.6 strict LUFS Match result is unchanged.


## 0.1.8 GR Hold readability / uniform 1:1 resize

- [ ] Panel version reads small/low-contrast `v0.1.8`.
- [ ] GR Hold second numeric readout has no `H` prefix and remains clearly associated with the white Hold marker.
- [ ] Hold number is easier to read than 0.1.7 and current GR remains the primary numeric readout.
- [ ] 2-second Hold timing/refresh behaviour is byte-for-behaviour unchanged from 0.1.7.
- [ ] Dragging the editor corner preserves the 1020:670 aspect ratio.
- [ ] At several sizes, title/fonts/buttons/knobs/meters/Dynamic Display/spacing/strokes all scale by the same factor rather than stretching independently.
- [ ] Mouse hit targets remain aligned after scaling, including A/B, Match, Bypass, Mode, Lookahead and rotary controls.
- [ ] A previously saved non-proportional 0.1.7 size migrates to a proportional 0.1.8 size and remains proportional after reopen.
- [ ] Audio DSP, Lookahead/PDC, LUFS Match, A/B, Makeup/Mix and ST/MS/LR behaviour are unchanged.


## 0.1.10 0 ms-only 1x/8x/16x Oversampling

### UI / product logic

- [ ] Panel version reads subdued `v0.1.10`.
- [ ] Lookahead `0 ms`: `OVERSAMPLING` label and one button are visible.
- [ ] One click cycles exactly `1x -> 8x -> 16x -> 1x`.
- [ ] A fresh/new/legacy-no-OS state remembers `8x` as the default 0 ms choice.
- [ ] Lookahead `10 / 26 / 40 / 80 / 100 ms`: Oversampling label/button are hidden.
- [ ] Non-zero Lookahead actually runs 1x internally; no hidden 8x/16x CPU or FIR latency remains.
- [ ] Switch `0 ms / 16x -> 10 ms -> 0 ms`: the 0 ms button returns to `16x` rather than resetting.

### Deliberately omitted factors

- [ ] UI/host choice list contains no user-facing `2x` or `4x`.
- [ ] Do not mark their absence as a bug: user PluginDoctor tests already found both insufficient against 0 ms aliasing.

### PluginDoctor

- [ ] At 0 ms and a clearly nonlinear Ratio, compare `1x / 8x / 16x`: alias fold-back should progressively reduce.
- [ ] Do **not** require 8x/16x to remove the intended harmonic colour itself.
- [ ] At 10 ms and longer, verify there is no meaningful aliasing regression relative to the approved 1x behaviour.
- [ ] Ratio `1:1` + 8x/16x remains essentially flat except expected near-Nyquist FIR roll-off.
- [ ] Do not add compensating EQ only to make nonlinear Ratio>1 LinearAnalysis visually flat.

### PDC / Dry / Mix / Bypass

- [ ] 0 ms / 1x reports no Oversampling FIR latency.
- [ ] 0 ms / 8x reports the JUCE integer FIR latency and Bypass uses the same delay.
- [ ] 0 ms / 16x reports its JUCE integer FIR latency and Bypass uses the same delay.
- [ ] 10 ms+ reports Lookahead-only latency regardless of the hidden remembered 0 ms choice.
- [ ] Mix around 50% has no steady-state combing from Dry/Wet time misalignment at 8x or 16x.
- [ ] Switching factor/Lookahead may cause one PDC realignment but settles to the exact reported delay.

### State / A-B / Undo

- [ ] Project save/reload restores the remembered 0 ms OS choice.
- [ ] A/B and A→B/B→A restore Lookahead + remembered OS choice together.
- [ ] Undo/Redo on the OS button restores factor and PDC.
- [ ] v0.1.8-or-earlier state with no OS migrates to remembered 8x.
- [ ] v0.1.9 state migration: old 1x -> new 1x; old 2x/4x/8x -> new 8x.

### CPU / warnings / regressions

- [ ] Profile 8x and 16x CPU at common host block sizes.
- [ ] Build has no local-name shadow warning for `playHead` or `constrainer`.
- [ ] strict LUFS Match result is unchanged.
- [ ] Ratio law / ST-MS-LR / Makeup / Mix / 2 s GR Hold / Dynamic Display remain unchanged.
- [ ] Existing 1020x670 uniform scaling and mouse hit-testing remain correct.

## 0.9.0 Warm Transparent UI Candidate

### Version / theme

- [ ] Panel version reads subdued `v0.9.0`.
- [ ] Main canvas is warm ivory/sand, not the old black prototype theme.
- [ ] Text remains high-contrast/readable at minimum and maximum editor size.
- [ ] Panel shadows/glow remain subtle; no clipped neon halos.

### Knobs / buttons / glow

- [ ] Ratio / Makeup / Mix use warm orange illuminated arcs.
- [ ] Mode reads as a compact active state button and clicks `ST -> MS -> LR -> ST`.
- [ ] A/B selected slot is visually obvious.
- [ ] Bypass ON is visually obvious but not visually overwhelming.
- [ ] Oversampling button uses the technical cyan accent and remains visible only at 0 ms.
- [ ] Match disabled state is still clearly disabled/readable.

### Display / meters

- [ ] Dynamic Display geometry/behaviour is unchanged from 0.1.10.
- [ ] Dry trace = warm neutral grey; Wet = cyan; Output = coral/orange.
- [ ] Meter panel is light/warm and Input/Output/GR remain distinguishable.
- [ ] GR grows downward from 0 dB and the 2 s Hold marker/value still work.

### Resize / interaction

- [ ] 1020:670 aspect and uniform scaling remain correct.
- [ ] Knob/button/ComboBox mouse hit targets stay aligned after resize.
- [ ] Lookahead popup menu remains readable on the light theme.
- [ ] Shift fine drag / Alt reset / Undo / Redo still work.

### DSP regression

- [ ] Ratio / Lookahead / Oversampling / LUFS Match / PDC / A-B / Makeup / Mix are audibly/behaviourally unchanged from 0.1.10.
- [ ] 0 ms Oversampling still cycles 1x/8x/16x; 10 ms+ remains fixed 1x internally.


## 0.9.1 Lighting & Material Refinement Candidate

### Visual lighting

- [ ] Panel version reads subdued `v0.9.1`.
- [ ] v0.9.0 layout is unchanged.
- [ ] Ratio / Makeup / Mix have a visible soft halo rather than only an orange outline.
- [ ] A broad but subtle warm spill is visible below each warm-accent knob.
- [ ] The lit arc endpoint has a small bright core without looking like harsh neon.
- [ ] Knob body has a light top rim, soft lower shade and subtle reflected warm light.
- [ ] Mode / selected A-B / Bypass ON / 0 ms Oversampling use the same back-lit visual language.
- [ ] Glow is not clipped at min/max editor sizes.
- [ ] Background remains warm ivory and does not become dark/heavy.

### Compile / regression

- [ ] MSVC C4459 `lookaheadMs` warning is gone.
- [ ] No new compiler warnings from the lighting code.
- [ ] Mode still cycles `ST -> MS -> LR -> ST`.
- [ ] Dynamic Display and meter geometry/data semantics are unchanged.
- [ ] Ratio / Lookahead / Oversampling / PDC / LUFS Match / A-B / Makeup / Mix / GR Hold are unchanged from v0.9.0.

## v0.9.2 — Asset knobs / Input & Output Gain

- [ ] VST3 compiles with `QQSCAssets` BinaryData target.
- [ ] No external PNG is required beside installed VST3.
- [ ] Ratio / Makeup / Mix / Input Gain / Output Gain show the intended bitmap knob.
- [ ] visual frame 0 pointer visible.
- [ ] visual frame 127 pointer visible.
- [ ] ~20% shows only the first ~20% of the lit arc.
- [ ] 100% shows the complete usable arc lit.
- [ ] direct numeric input remains editable and supports decimals.
- [ ] DAW automation remains continuous; values are not quantised to 128 steps.
- [ ] Input Gain 0 dB null/behaviour regression against 0.9.1 where applicable.
- [ ] Input Gain changes compression response.
- [ ] Input Gain changes Input meter but does not move Dynamic Display Dry/Input reference by itself.
- [ ] Output Gain changes final audio, Output meter and Dynamic Display Output.
- [ ] LUFS Match still writes Makeup only.
- [ ] Mix 0 path uses Input-Gain-adjusted Dry when active.
- [ ] Bypass removes both trims but keeps exact PDC alignment.
- [ ] A/B copy/recall includes both trims.
- [ ] Undo/Redo includes both trims.
- [ ] pre-0.9.2 project restores Input/Output Gain at 0 dB.
- [ ] 0ms 1x/8x/16x and 10/26/40/80/100ms latency regressions pass.
- [ ] MSVC C4459 warning from old lookahead naming does not return.


## v0.9.3 — UI rollback / v0.9.2 feature retention

- [ ] Panel version reads `v0.9.3`.
- [ ] No bitmap/filmstrip knob is visible or required at runtime.
- [ ] v0.9.1 warm vector knob/button appearance is restored.
- [ ] Input Gain and Output Gain remain visible, editable and automatable.
- [ ] Input Gain affects compression/Input meter but not Dynamic Display Dry/Input reference.
- [ ] Output Gain affects final Output meter and Dynamic Display Output.
- [ ] A/B, save/reload and Undo/Redo retain both trims.
- [ ] Pre-0.9.2 state still migrates both trims to 0 dB.
- [ ] 0 ms 1x/8x/16x and 10/26/40/80/100 ms PDC behaviour unchanged.
- [ ] LUFS Match, ST/MS/LR, Mix, Bypass and 2 s GR Hold unchanged.


## v0.9.4 — Editable numeric text contrast

- [ ] Panel version reads `v0.9.4`.
- [ ] Double-click Ratio numeric value: editable text is dark/readable on ivory.
- [ ] Double-click Makeup / Mix numeric values: editable text is dark/readable.
- [ ] Double-click Input Gain / Output Gain numeric values: editable text is dark/readable.
- [ ] Decimal point, minus sign, plus sign/unit suffix where applicable remain visible.
- [ ] Text selection remains readable and caret is visible.
- [ ] Enter commits direct input normally.
- [ ] Normal non-editing text colours are unchanged.
- [ ] Audio DSP/state/PDC/LUFS Match/GR Hold regression checks remain unchanged from v0.9.3.

## v0.9.7 — Threshold Rebuild / UI proportion

### Core regression against v0.9.4

- [ ] Threshold OFF nulls / behaves identically to v0.9.4 at the same settings.
- [ ] PluginDoctor Dynamics with Threshold OFF matches v0.9.4; especially compare Ratio 3:1 / 8:1 / 16:1 / 32:1.
- [ ] No v0.9.6 half-wave/segment detector behaviour remains.
- [ ] Lookahead 0 / 10 / 26 / 40 / 80 / 100 ms retains the v0.9.4 detector/window behaviour.
- [ ] 0 ms 1x/8x/16x behaviour and PDC remain unchanged.

### Threshold

- [ ] OFF is default and is displayed as OFF.
- [ ] Below finite Threshold: no compression.
- [ ] At Threshold boundary: no discontinuous gain jump in the static curve.
- [ ] Above Threshold: same QQ Super Compression Ratio character, not classic above-threshold compressor timing.
- [ ] Shift-drag on Threshold is visibly finer than normal drag and can set 0.01 dB values.
- [ ] Alt+left-click returns Threshold to OFF.
- [ ] Double-click accepts exact values and `OFF`.
- [ ] Undo/Redo includes Threshold.
- [ ] A/B + A→B/B→A includes Threshold.
- [ ] Save/reload restores Threshold; old v0.9.4 project loads Threshold OFF.

### Layout

- [ ] Display and meters are noticeably taller than v0.9.4/v0.9.5.
- [ ] Lower knobs/buttons are smaller but remain readable and easy to hit.
- [ ] Threshold strip does not overlap Display or meters.
- [ ] Fixed aspect-ratio resize keeps all hit targets aligned.
