# Current checkpoint: 1.2.0 Rev4 Stable — 2026-09-10

Rev4 supersedes the historical checkpoint below. See [STABLE_1.2.0.md](STABLE_1.2.0.md) and [VERIFICATION_REV4_1.2.0.md](VERIFICATION_REV4_1.2.0.md). The user authorized B, then C and D with the completed 23-page PDFs retained unchanged. Current production schema is 14.

---

# QQ Super Compression 1.2.0 Rev3 — verified results

2026-09-10. Windows x64 Release, JUCE8.0.15/VS2022, state schema13.
Current revision only; earlier Rev2 results are retained in its separate output.

## Actual compiled audio processor

- All15 Ratio parameters and decoded host defaults equal1:1. First-use Dual
  Ratio LINK is ON. Existing finite Range0 and saved Ratio/domain banks survive.
- Five-domain activation: at/below UP unity, above UP/below DOWN boost, at DOWN
  unity, above DOWN attenuation independent of Up Ratio. UP=-inf opens the gate.
- Real400Hz input-40dB, UP=-50dB, DOWN=-30dB,48kHz/26ms: actual visible Up knob
  1 ->0dB gain;1/8 ->+7.92198dB;1/32 ->+9.43206dB. The parameter and output agree.
- With the input10dB below DOWN and20dB-wide UP/DOWN intervals at DOWN=-20/-40/
  -60dB, each Ratio1:8 result gives+7.92198dB. Sub-UP audio is not boosted.
- Original599.9Hz full-scale regression:44.1/48/96kHz and10/26/80ms, zero unity
  bursts, maximum sample difference5.96046e-8 versus the retained1.1.9 engine.
- Ten boundary pairs: collision pushing, separation on reversal, equality
  bypass. Actual LR and pure MS processing, signed Mix/Hold, Mix0, absent EXT
  key unity, delayed bypass/PDC,64/511-sample block parity (max error0) pass.
- A/B and host-state roundtrip pass. Schema10 missing fields get explicit new
  defaults on a dirty instance; schema11 finite Range0 and saved banks remain.

Steady float-output H2-H7 THD relative to fundamental, transients excluded:

| Case | Frequency/lookahead | THD |
|---|---|---|
| Single Down |400Hz/26ms|-157.987dB|
| Single Up |400Hz/26ms|-160.312dB|
| Dual Up |400Hz/26ms|-166.398dB|
| Dual Down |400Hz/26ms|-169.571dB|
| Dual Up, strongest Ratio |20Hz/80ms|-168.366dB|

These are specific steady-carrier results, not an unconditional zero-distortion
claim. Confirmed hard gates retain boundary jumps; the Single fixture at
Threshold-40/Range-6 measures12.4926dB downward exit/16.9261dB upward entry.

## Actual editor and settings

- Five domains tested through real control gestures: default reciprocal pair,
  arbitrary product2 pair, both edit directions, actual numeric commits, Alt
  resets, shared limits/reversal, toggle-without-jump, and existing domain LINK.
- Host parameter listeners see balanced gestures for the source and companions;
  switching Single/Dual during a drag closes the old attachment correctly.
- Isolated D-drive settings file verifies first-use ON, last choice across new
  instances, saved project precedence and repeated editor reopen. User settings
  were not changed by validation.
- All three production themes render exact94px ST/Single primary drawing
  areas,216px centre spacing, matching Y positions. ST LINK36x17, LR/MS30x14,
  centred between UP/DOWN columns. Default/minimum1008x672 snapshots inspected.
- Warm and Dark actual material pixels tested across3 origins x5 positions:
  only the span from neutral to current value lights. Classic left/right arcs
  and neutral state inspected in production editor renders. All five domains'
  Single/Dual light origins and mode reattachment pass.
- Full-length fader/Display coordinate alignment, input trim, stopped ST/LR/MS
  changes, real mouse-down without jumps, signed meter and480-point budget pass.
- Existing material cache tests and all ten inherited core/domain/sidechain/
  Display source/mathematical checks pass.

240 changing-boundary LR frames, cached Display projection + software paint:

| Mode | Median | p95 |
|---|---|---|
| Single |4.207ms|5.140ms|
| Dual |4.440ms|5.805ms|

This is a same-machine offscreen measurement, not total DAW UI timing. Preview
PNGs contain synthetic graph/meter fixtures; they are not live audio captures.
JUCE's module helper loads the built VST3 and generates version metadata; no
third-party full VST3 validator or user listening acceptance is claimed.

## Artifact identity

Installed after confirming hosts closed on2026-09-10. Full bundle parity passed.

Binary SHA256: `9AC4E801390563AE082B91CE6CE475F8D5F2F2BD44BA6EDA6DB7CB8687218642`.
Output: `D:\Codex\Outputs\QQ Super Compression 1.2.0 Rev3`.
`Verification/INSTALLATION.json` records host-closure check, installed status,
and full build/output/system bundle parity. The ZIP is separately checked by
reading every bundle entry and hashing its uncompressed contents.
