# QQ Super Compression — UI Design Notes

## v1.0.2 — Complete Relative LINK interaction

- 1020×820 Display-first geometry and visual language remain unchanged.
- In LR/MS, one LINK state covers Ratio, Threshold, Makeup, and Mix.
- Drag, Shift-fine, and direct numeric entry preserve the existing pair offset.
- Both values stop together at boundaries; LINK never silently collapses them to equality.
- LINK remains outside A/B sound snapshots. No DSP, parameter-ID, or layout redesign is part of this version.


## Why this document exists

Codex / future AI may work from the GitHub repository without access to the original design conversation. This file records the **reasoning behind the v0.9.0 visual direction**, so future UI work does not accidentally move the product toward a style that contradicts its sound and product identity.

## Product context first

QQ Super Compression was created for situations where dynamic compression is useful but conventional Attack / Release behaviour can undesirably reshape note onsets and transients. Guitar pick attack, vocal consonants, piano hammer attack and bass finger/pick articulation are representative examples.

The UI should therefore visually communicate:

- clean;
- transparent;
- light;
- precise;
- modern but not cold;
- musical rather than laboratory-only.

The UI must **not** visually imply heavy saturation, distortion or aggressive colour unless the user is specifically in the 0 ms colour mode.

## Rejected first visual direction

An early concept used a very dark cyberpunk / neon-black appearance. The user rejected it because the dark palette visually suggested heavy distortion / saturation, while the actual product is intended to feel clean and transparent.

Do not return to a predominantly black / purple / heavy-neon UI unless the user explicitly changes direction.

## Approved v0.9.0 direction

The approved direction is a **clean, warm, transparent, retro-future instrument**:

- warm ivory / sand background rather than black;
- dark warm-grey typography;
- soft orange / apricot as the primary musical accent;
- cyan as a secondary technical / wet-signal accent;
- thin outlines and generous negative space;
- subtle glass/instrument-panel feeling;
- gentle light bloom rather than neon.

The user specifically likes the feeling of modern Baby Audio interfaces: simple, clean, warm and immediately readable. This is a **directional reference**, not a request to clone any exact product layout or assets.

## Dynamic Display

The user is already satisfied with the existing Dynamic Display structure and does **not** want it redesigned into a FabFilter/Cenozoix-style display.

For v0.9.0:

- keep the existing history behaviour and geometry;
- keep the three semantic traces;
- change background / grid / typography / trace palette only;
- Dry remains neutral;
- Wet remains cyan;
- Output becomes warm coral/orange.

The Display should remain the central visual evidence of what the processor is doing, but its operation is unchanged.

## "Lamp under glass" glow

The user especially liked the soft illuminated controls in the approved concept.

Implementation principle:

- do not use a large dark neon halo;
- use several low-alpha vector strokes / fills around the active arc or button;
- add one small bright endpoint / lamp point;
- use a soft warm shadow / reflection so the control feels illuminated from within;
- keep glow subtle enough that the UI still reads as clean and transparent.

v0.9.0 implements this using layered JUCE vector drawing in `UTF8LookAndFeel.h`, not bitmap glow assets or an expensive realtime blur pass.

This choice keeps scaling clean at every editor size and makes colour / intensity easy to tune later.

## Colour roles

Primary roles in v0.9.0:

- warm ivory / sand: chassis and panels;
- warm dark grey/brown: text;
- apricot/orange: Ratio, Makeup, Mix, selected musical states;
- cyan: Wet / Lookahead / technical state accent;
- coral/orange: final Output and Gain Reduction emphasis;
- warm neutral grey: Dry/Input reference.

These roles should remain semantically consistent. Avoid arbitrary per-control colours.

## Mode interaction — user-approved Scheme A

Do **not** use a dropdown for ST / MS / LR.

The approved interaction is one compact button showing the current mode. A click cycles the three modes using the existing processing-mode parameter semantics:

`ST -> MS -> LR -> ST`

This preserves the established internal mode ordering and keeps the interface direct and compact.

The button should visually read as an active mode/status control, with a subtle warm illuminated treatment. It should not resemble a hidden menu.

## What v0.9.0 is allowed to change

- LookAndFeel drawing;
- colours;
- backgrounds / panel treatment;
- borders / light/shadow treatment;
- control visual hierarchy;
- Mode visual presentation as the approved single cycle button;
- user-facing version metadata to v0.9.0.

## What v0.9.0 must NOT change

- Ratio DSP law;
- Lookahead values or detector meaning;
- 0 ms 1x/8x/16x Oversampling semantics;
- PDC / Dry-Wet alignment;
- LUFS Match;
- A/B snapshot behaviour;
- 2 s GR Hold;
- ST / MS / LR DSP definitions;
- Makeup / Mix parameter meanings;
- Dynamic Display data semantics;
- project parameter IDs.

## Release plan

v0.9.0 is a **Candidate / pre-release UI pass**.

The user intends to review and polish this UI before promoting the product to **v1.0.0 Release**. Do not label v0.9.0 Stable / Release unless the user explicitly confirms it.


---

## v0.9.1 — Lighting / material refinement after real DAW screenshot

The user compiled v0.9.0 and supplied a real Cubase screenshot. The **layout was accepted**, but the intended illumination was not: the orange knob arcs looked like coloured outlines rather than actual light. This is a visual implementation miss, not a change of direction.

### User-approved correction

Do **not** redesign the layout. Keep the current Display, meter placement, parameter positions and compact Mode button. Refine only material and lighting so the approved concept reads in the real plug-in window.

The lighting stack is now intentionally more legible on a bright ivory background:

- wide, very-low-alpha halo around the active arc;
- medium-energy bloom nearer the arc;
- crisp illuminated value arc;
- bright endpoint lamp / hot core;
- broad, shallow warm spill below the knob to simulate light reaching the matte panel;
- a small amount of warm reflected light clipped into the lower knob body;
- fine white top rim and soft lower shade to give the knob physical material depth;
- lit buttons use the same halo + bottom-spill language instead of only changing border colour.

The goal is still **warm, clean, transparent**, not neon. If later tuning is needed, first adjust alpha/width values in `UTF8LookAndFeel.h`; do not solve visibility by making the whole UI darker.

### Important non-goals

- no layout rewrite;
- no new animation system;
- no dark cyberpunk background;
- no bitmap glow assets;
- no DSP changes;
- no parameter/state changes.

### Compile-warning cleanup bundled with this pass

The user reported a real MSVC C4459 warning in v0.9.0 where `lookaheadMs` hid the namespace parameter identifier. v0.9.1 only renames the conflicting argument/local names (`requestedLookaheadMs`, `currentLookaheadMs`, `snapshotLookaheadMs`). This is a source-cleanliness fix and must not be interpreted as a Lookahead behaviour change.

---

## v0.9.2 — Image-asset rotary architecture

The user rejected further attempts to approximate the approved knob material/glow purely with JUCE drawing. v0.9.2 therefore changes rotary controls to a real 128-frame transparent PNG filmstrip.

Critical rules:

- 128 frames are visual states only; parameter precision remains continuous.
- Numbers, decimal points, signs, `:`, `%`, `dB` and labels stay live font-rendered/editable UI.
- Pointer must remain visible at both frame 0 and 127.
- The orange edge lamp accumulates from minimum to current value; 100% lights the complete usable arc.
- Runtime image assets are embedded via BinaryData, not loaded from an external installation path.
- Buttons are intentionally not converted yet; their asset states need separate visual approval.

See `UI_ASSET_ARCHITECTURE.md` for implementation details.


---

## v0.9.3 — User-approved rollback from bitmap knobs

After real v0.9.2 testing and repeated asset-generation attempts, the user decided to stop pursuing the bitmap-filmstrip knob direction. The active UI returns to the v0.9.1 JUCE/vector warm ivory / warm orange / cyan design language.

Important: this is a **visual implementation rollback only**. v0.9.2 Input Gain / Output Gain and their behaviour remain. Input/Output are displayed as smaller secondary trims; Ratio / Makeup / Mix remain the main controls.

The v0.9.2 asset experiment remains documented and archived so a future AI does not mistake it for an unfinished task and silently re-enable it. Do not reintroduce bitmap knobs unless the user explicitly asks again.


---

## v0.9.4 — Direct numeric-entry contrast

The v0.9.3 light UI was visually accepted, but the user found a small interaction-state defect: double-clicking a slider value creates JUCE's temporary text editor, whose default editable text was white and therefore unreadable on the ivory surface.

This is not a redesign. The active colour language stays unchanged. The fix explicitly styles the edit state as dark warm text on ivory, with a warm-accent caret/focus outline and a soft warm selection highlight. Slider values remain live/editable text and are not converted to bitmap assets.

## 1.0.1 Display working range

The Dynamic Display uses a fixed `0…-90 dB` visible range with grid labels at `0 / -15 / -30 / -45 / -60 / -75 / -90 dB`. This is a Threshold-workflow presentation rule only. Values may be clamped at the drawing boundary, but DSP, meters, loudness, parameter limits, and the `-120 dB` Threshold OFF sentinel must remain independent from the graph floor.
