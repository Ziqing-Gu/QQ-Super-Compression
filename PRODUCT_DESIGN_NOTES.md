# QQ Super Compression — Product Design Notes

> This file exists so future Codex/AI/developers can understand **why this plug-in exists**, not only how the code works. Do not remove it when refactoring or preparing GitHub/Release documentation.

## 1. The original problem

QQ Super Compression was designed for a specific mixing problem:

> Sometimes a source needs dynamic compression, but the engineer does **not** want conventional Attack / Release behaviour to reshape the source's original transient or note onset.

This is not an argument that traditional compressors are bad. Attack and Release are powerful sound-shaping tools, and changing transients is often exactly what a mix engineer wants. The problem is that there are also cases where the engineer wants **dynamic control without intentionally changing the transient character**.

Typical examples include:

- **Guitar** — pick attack and the front edge of a note can become softer, harder, or more exposed depending on compressor timing.
- **Vocals** — consonants, plosives and the beginning of syllables can be reshaped, changing articulation and perceived immediacy.
- **Piano** — the hammer attack is a major part of the instrument's identity; fast/slow compression timing can noticeably alter the perceived strike.
- **Bass** — finger/pick attack strongly affects definition, groove and articulation; conventional timing can change that front edge.

The design target is therefore:

> **Compress the dynamic range while preserving the original transient / onset character as much as practical.**

## 2. Why there is no conventional Attack / Release

Traditional compressors react with a time envelope. If the Attack is slow, some transient may pass before the gain reduction reaches its target. If Attack is fast, the front edge can be reduced more aggressively. Release then determines how the gain recovers after the event.

QQ Super Compression deliberately does not expose that conventional Attack / Release envelope. Instead, it delays the audible path and analyses a future window, so it can know how large the upcoming waveform is **before the corresponding delayed audio reaches the output**.

Conceptually:

```text
future waveform window
        -> estimate level / peak for the delayed sample
        -> derive Ratio gain directly
        -> apply that gain to the correspondingly delayed original waveform
```

The point of Lookahead here is not merely "a faster compressor". It is a way to avoid depending on a conventional Attack envelope to catch a transient after it has already arrived.

## 3. Ratio is not a classic threshold slope

The current Ratio law is:

```text
gain = 1 / (1 + (Ratio - 1) * level)
```

where `level` is the future-window peak, normalised to 0..1.

There is no conventional Threshold control. Ratio is best understood as the strength of the dynamic reduction curve rather than a classic "above-threshold 4:1" slope.

Do not replace this law casually. The rejected v0.1.0 sample-domain waveshaper and the rejected v0.1.1-v0.1.3 rolling-RMS direction are retained in development history for a reason.

## 4. Why Lookahead changes the character

At 0 ms, the detector collapses to instantaneous sample magnitude. That makes the gain sample-dependent and therefore nonlinear in a way that creates audible/measurable harmonic coloration.

As Lookahead becomes longer, the future-window peak becomes more stable across each waveform cycle. A stable tone is then more likely to receive a near-constant gain rather than gain that follows the carrier waveform itself. This is why the longer Lookahead settings progressively approach the intended "dynamic shaping without conventional transient-envelope behaviour" concept.

User PluginDoctor testing established a clear practical relationship:

- 5 ms: distortion boundary about 99.6 Hz;
- 10 ms: about 49.8 Hz;
- 20 ms: about 24.9 Hz;
- 26 ms: boundary moved to roughly the 20 Hz region;
- 40 ms: 20 Hz visibly cleaner than 26 ms;
- 80 ms: cleaner again.

The approved user-facing Lookahead presets remain:

`0 / 10 / 26 / 40 / 80 / 100 ms`.

## 5. 0 ms is intentionally retained as a colour mode

0 ms does **not** represent the most transparent interpretation of the original design goal. It is intentionally retained because the user found its nonlinear sound musically useful.

That distinction matters:

- 10-100 ms are primarily the "dynamic control while preserving transient character" direction.
- 0 ms is a deliberate additional colour/flavour derived from the same Ratio concept.

Future developers must not silently add smoothing or hidden Lookahead to 0 ms in an attempt to "fix" it. If 0 ms is changed, the user must explicitly approve the new sonic meaning first.

## 6. LUFS Match is for fair comparison, not an always-on Auto Gain

Strict BS.1770 / EBU R128 Integrated LUFS Match was added so the engineer can remove loudness bias when judging the compression result.

It compares delayed Dry with compressed Wet **before Makeup and before Mix**, then writes Makeup. The purpose is to let the user judge changes in dynamic shape and transient feel without "louder sounds better" bias.

It is not intended as an automatic gain-riding stage that continuously controls loudness.

## 7. Mix and Ratio are intentionally separate

Ratio defines the shape/intensity of the compressed Wet result. Mix defines how much of that result is blended with the compensated Dry path.

A high Ratio at partial Mix is not mathematically identical to a lower Ratio at 100% Mix; this is an intentional creative degree of freedom, not a bug.

## 8. What the plug-in is *not* claiming

Do not market QQ Super Compression as:

- "perfect transient preservation" in every possible signal;
- a replacement for all conventional compressors;
- a mathematically scale-invariant compressor (the current Ratio law depends on absolute normalised detector level);
- an Attack/Release compressor with hidden timing controls.

A more accurate product statement is:

> **QQ Super Compression is designed for situations where dynamic compression is needed, but conventional Attack / Release transient reshaping is not desired. It uses future-window analysis to reduce dependence on a conventional attack/release envelope, while also retaining a deliberately coloured 0 ms mode.**

## 9. Documentation rule

When Codex/AI writes the GitHub README, Release text or product introduction, lead with the **mixing problem and use case** above. Do not describe the product merely as "a threshold-free compressor" or "a compressor with Lookahead"; those are implementation details, not the reason the product exists.
