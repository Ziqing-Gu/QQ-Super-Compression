# QQ Super Compression UI Design Notes

## v1.1.2 Plan A candidate - Mix-aware Dynamic Display

- Keep the approved 1020x820 layout, Display/Meter geometry, Side Chain popup, and both themes unchanged.
- Remove the visible cyan Wet pre-Makeup trace and its header readout.
- Draw Dry/Input as the neutral reference, Effective GR (including Mix) as a translucent GR-accent band plus lower boundary, and projected Output post-Mix as the strong Output trace.
- Header readout is GR (MIX) so the definition is explicit.
- At EXT, draw the actual post-Key-Gain/post-HPF future-window detector contour behind the main histories using a broad very-low-alpha stroke plus a thin low-alpha stroke. If the external bus is disabled, show EXT N/A and draw no false key line.
- Store raw history evidence and reproject on every paint so Input/Ratio/Threshold/Mix/Makeup/Output edits update the complete visible window even while transport is stopped.
- ST has one linked history; LR and MS retain two stacked domain histories. LIGHT and CLASSIC use the same geometry and logic.
## v1.1.1 Stable baseline - Side Chain HPF popup

- Keep the approved 1020x820 main layout unchanged.
- Add one HPF rotary control to the existing Side Chain floating panel.
- Popup geometry is 330x146 design pixels and grows leftward from the SC header button.
- Column layout: source/meter/listen at left, Key Gain in the middle, HPF at right.
- HPF shows OFF or an integer 20-500 Hz value and uses the cyan technical accent.
- HPF stays available for INT and EXT. Key Gain remains enabled only for EXT.
- LIGHT and CLASSIC must share identical bounds, labels, ranges, automation and functionality.

## v1.1.0 Stable baseline — External Key floating panel

- Keep the approved 1020x820 layout and all main-control geometry unchanged.
- A persistent `SC: INT / SC: EXT` button sits immediately left of the existing LIGHT/CLASSIC theme button.
- Clicking it opens a 230x146 design-pixel floating panel below the header. The panel contains `SOURCE` INT/EXT, `KEY LEVEL`, `SC LISTEN`, and a compact `KEY GAIN` knob.
- LIGHT and CLASSIC share exactly the same bounds, hit targets, labels, controls, state, and functionality. Only palette and LookAndFeel drawing differ.
- Key Gain is enabled only for EXT. The meter displays `N/A` when the external bus is disabled and `-inf dB` for valid silence.
- Closing the panel with the SC button or Escape turns SC Listen off. Closing the editor also turns it off.
- The panel floats above the Display and must not reduce Display/Meter height or move Ratio, Threshold, Makeup, Mix, Mode, Monitor, Lookahead, Oversampling, A/B, Bypass, or theme controls.

## v1.0.4 — Light / Classic UI switch — Stable baseline

- The current warm/light interface and the earlier calm dark interface are both supported without moving any controls or changing DSP behaviour.
- A compact upper-right button displays `LIGHT` or `CLASSIC`; it switches only colours and LookAndFeel drawing for the existing controls.
- Classic uses a low-luminance charcoal chassis, cyan controls/technical state, amber Output and pink Gain Reduction. It deliberately avoids the light theme's broad lamp glow.
- The selected theme is a local `PropertiesFile` preference (`Qing Audio/QQSuperCompression.settings`) and is restored when the editor is reopened.
- Theme is not an APVTS parameter and is excluded from audio processing, host automation, A/B snapshots and project state.
- This explicit user decision supersedes the historical note that rejected a predominantly dark interface: dark is now valid only as the optional Classic skin, while the Light skin remains unchanged.

## v1.0.3 Centered Domain Monitor

- Do **not** shrink the Display/Meter row. It stays 550 design px.
- Lower controls use the previously unused bottom slack: 140 -> 158 design px.
- Technical column order in LR/MS: `MODE` -> Mode+LINK row -> `MONITOR` -> ALL/L/R or ALL/M/S row -> `LOOKAHEAD` -> Lookahead -> optional `OVERSAMPLING`.
- ST hides the Monitor label/buttons and LINK; Mode geometry never moves.
- Mode and Lookahead main rectangles remain exactly **108 x 23** with common left/right edges. LINK remains **34 x 23**, 6 px to the right.
- Monitor occupies exactly the same 108 px primary width using `34 + 3 + 34 + 3 + 34`.
- Monitor active state uses the cyan technical accent. It is an audition status, not a second processing mode.
- Labels change by mode: LR = `ALL / L / R`; MS = `ALL / M / S`.

## v1.0.1 Display-first update

### Revision 3 — compact technical controls

- Mode is **not** a dropdown. It remains a click-cycle button.
- Lookahead **does** remain a dropdown because there are six choices.
- Mode and Lookahead primary controls are visually identical: **108 x 23 design px**, same left/right edges.
- LINK is a small auxiliary control to the right of Mode only; it must not shorten the Mode button. Current geometry: **34 x 23**, 6 px gap.
- ST hides LINK; LR/MS show LINK without moving the Mode button.
- Oversampling follows the same primary-control alignment when it appears at 0 ms.

The warm/light visual direction remains unchanged, but the analysis area is now explicitly prioritised because Threshold selection depends on seeing the dynamic history clearly.

- Fixed design root is enlarged from **1020x670** to **1020x820**.
- Display/Meter row is **550 design px**.
- Lower control row remains compact at **140 design px**.
- Meter width is capped slightly narrower and the Threshold strip is reduced to give the Display more horizontal room.
- ST keeps one full Display.
- LR/MS keep two stacked full-width Displays so the time axis is not halved.
- Enabled Threshold lines show the actual Threshold dB value directly on the graph.
- LINK moves into the Mode row. This is required because the restored transparent core again exposes Oversampling at 0 ms; LINK and Oversampling must never overlap.
- Threshold faders retain Shift fine drag, Alt reset and direct numeric entry.

The v1.0.0 Direct/Analytic engine is rejected; there is no Engine selector in the product.


## v1.0.2 Complete LINK interaction

The visible geometry is unchanged from the user-approved v1.0.1 layout. LINK remains the small auxiliary button beside Mode, but its workflow scope is now complete:

- LR/MS LINK covers Ratio / Threshold / Makeup / Mix.
- LINK preserves relative differences; it never equalises paired values.
- Normal drag, Shift fine drag and direct numeric entry must all apply the same shared delta.
- Shared boundaries stop both values together.
- This is an interaction fix only; do not enlarge/reposition LINK or reclaim Display space.

--- HISTORICAL UI NOTES BELOW ---

## v1.0.0 additions

The overall warm/light direction remains unchanged. The upper analysis region remains intentionally taller and the lower controls compact.

- ST: one Dynamic Display panel.
- LR: two stacked full-width panels labelled L/R.
- MS: two stacked full-width panels labelled M/S.
- Each domain panel has its own Threshold reference line.
- LR/MS expose paired Ratio controls, paired Threshold faders and paired Makeup controls.
- One `LINK` button controls relative linking for all three pair types; it never equalises values.
- Link lives in the compact technical-control column so it does not consume additional Display width.
- Threshold faders retain Shift fine drag and direct numeric entry.

The old note below saying the Dynamic Display geometry must not change is historical for v0.9.0; v1.0.0 explicitly supersedes that point for LR/MS domain splitting.

--- HISTORICAL UI NOTES ---

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

---

## v0.9.7 — Display/Meter priority + Threshold fine interaction

User feedback: the analysis Display had become too small relative to the lower control area. Keep the fixed 1020x670 design root but allocate more vertical height to the upper Display/Meter region and make the lower knobs/buttons more compact.

v0.9.7 layout target:

- Display/Meter visual row: 405 design px (was 350).
- Lower control row: 145 design px (was 166).
- Threshold remains a narrow vertical strip between Dynamic Display and meters.
- Do not shrink the actual meter/display content to make room for new lower controls; analysis visibility has priority.

Threshold interaction must match the established parameter workflow:

- normal drag;
- **Shift = fine drag** (must work on the LinearVertical control, not only rotary controls);
- Alt+left-click = OFF/default;
- double-click numeric entry, including `OFF`;
- Undo/Redo and A/B/state persistence.


---

## v1.0.1 Candidate Revision 2 — Domain Alignment Rules

- Mode is a persistent, visible cycle button. LINK shares a fixed row position and appears in LR/MS; UI update timers must never recalculate their geometry.
- LR/MS visual grammar is vertical: top domain is L/M and bottom domain is R/S. Threshold controls follow that same top/bottom structure.
- Ratio, Makeup and Mix remain in the compact lower control row; in LR/MS each uses two small domain controls with L/R or M/S labels.
- The enlarged Display remains the primary workspace. Do not reclaim Display height for the new Mix controls.
- LINK is intentionally limited to Ratio/Threshold/Makeup relative editing; Mix is independent.


---

## v1.0.1 Candidate Revision 4 — Dynamic Display working scale

The Dynamic Display is a Threshold-working surface, so its vertical pixels should be spent on useful programme dynamics. The visible range is fixed at **0 to -90 dB** with `15 dB` grid spacing (`0, -15, -30, -45, -60, -75, -90`).

Do not use automatic vertical scaling: the Threshold line must remain visually comparable across material. Values outside the visible range are clipped only for drawing; this must never change DSP, meter values, the -120 dB Threshold-OFF sentinel, or stored parameter data.
