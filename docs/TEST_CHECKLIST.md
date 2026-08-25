# QQ Super Compression 0.1.8 Test Checklist

## Build / load

- [ ] Release VST3 compiles.
- [ ] VST3 scans and loads in DAW.
- [ ] Host-reported latency equals the selected Lookahead preset in samples.
- [ ] Bypass reports and uses the identical Lookahead latency/PDC.

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
