# QQ Super Compression — Oversampling Design Notes

> This file records the product decision behind Oversampling so future Codex/AI/developers do not "restore" discarded 2x/4x choices or enable Oversampling at every Lookahead simply because it seems conventional.

## Final product rule (v0.1.10 direction)

### Lookahead = 0 ms

Show one small Oversampling button. Single-click cycles:

```text
1x -> 8x -> 16x -> 1x
```

Default remembered 0 ms choice for a new/legacy instance: **8x**.

### Lookahead = 10 / 26 / 40 / 80 / 100 ms

- Oversampling control is **hidden**.
- DSP is forced to **1x internally**.
- The user's previous 0 ms Oversampling choice is preserved in state while hidden.
- Returning to 0 ms restores that remembered 1x/8x/16x choice.

## Why Oversampling exists only for 0 ms

The 0 ms detector collapses to instantaneous sample magnitude. The resulting sample-dependent Ratio gain creates strong harmonic coloration and, at host sample rate, high-order content can fold back below Nyquist as aliasing.

Oversampling is therefore used to reduce **alias fold-back** in this deliberately nonlinear mode.

Important distinction:

> Oversampling is **not** intended to remove the 0 ms harmonic character itself. The coloration is intentionally retained; Oversampling only gives the user cleaner versions of that coloration.

## Why 10 ms and longer do not use Oversampling

**User-verified PluginDoctor result:** after direct comparison, the user found that 10 ms and longer Lookahead modes do not show meaningful aliasing that justifies Oversampling.

Therefore enabling 8x/16x at those settings would mainly add CPU/implementation complexity without solving an observed problem. The product intentionally runs them at 1x.

This supersedes the v0.1.9 experiment where 1x/2x/4x/8x was available at every Lookahead.

## Why there is no 2x or 4x option

This is a deliberate decision, not an omission.

**User-verified PluginDoctor result:**

- 2x Oversampling still left severe aliasing in 0 ms mode.
- 4x Oversampling still left severe aliasing in 0 ms mode.
- The improvement was not enough to justify exposing those intermediate options.

At the same time, the user measured that Oversampling added only a small amount of latency in practice. Therefore there was little value in keeping 2x/4x simply to save a few samples of latency.

The final useful choices are intentionally:

- **1x** — raw/original 0 ms colour, including the strongest aliasing;
- **8x** — default practical balance; materially cleaner aliasing while preserving the 0 ms character;
- **16x** — further alias reduction for users who want the cleanest version of the 0 ms colour.

Do not re-add 2x/4x without new user testing and explicit approval.

## Why 8x is the default instead of 1x or 16x

- 1x is intentionally available for the most raw/original coloration, but aliasing is strong enough that it should not be the default quality choice.
- 8x is the default because it provides a substantial anti-aliasing improvement with only a small observed latency cost.
- 16x is provided because even at 8x the 0 ms coloration remains strong; 16x gives another step of alias reduction without changing the fundamental algorithm.

The default refers to the **remembered 0 ms Oversampling choice**. When Lookahead is non-zero the actual DSP path is still 1x.

## Frequency-response investigation that should not be misinterpreted

During PluginDoctor testing, LinearAnalysis at 0 ms and Ratio > 1 showed an apparent high-frequency rise that became stronger as Ratio increased. A control test at Ratio=1:1 showed the 8x FIR path itself was essentially flat except for the expected roll-off near Nyquist.

This isolated the observation away from a simple "the oversampling FIR boosts treble" explanation. It is associated with the nonlinear 0 ms detector/gain behaviour and the way a nonlinear system appears in linear-analysis measurements. The user subsequently confirmed there was no need to treat this as an Oversampling filter fault.

Do not add compensating EQ or redesign the FIR solely to make that nonlinear LinearAnalysis plot visually flat without new listening/measurement evidence.

## PDC / Dry-Wet alignment requirement

At 0 ms with 8x or 16x, the oversampling FIR itself has latency. Host-reported total latency must include that FIR latency, and the Dry/Mix/Bypass path must use the identical integer delay so there is no steady-state comb filtering or time offset.

At 10 ms or longer, effective Oversampling is 1x, so total latency returns to the original Lookahead-only latency.

## State / A-B behaviour

- The 0 ms Oversampling preference is stored in project state.
- It is included in A/B snapshots and normal Undo/Redo.
- Switching to a non-zero Lookahead does **not** overwrite it.
- Switching back to 0 ms restores it.
- v0.1.8-or-earlier states that have no Oversampling parameter migrate to remembered **8x**.
- v0.1.9 legacy choices migrate as follows: old 1x -> new 1x; old 2x/4x/8x -> new 8x, because 2x/4x were rejected and 8x is the new default practical oversampled mode.

## Verification status terminology

The conclusions about 2x/4x aliasing, 10 ms+ aliasing and low added latency came from **user PluginDoctor/host testing**. The exact v0.1.10 implementation still requires Codex compilation and DAW/PDC regression testing before it can be called fully verified.
