# QQ Super Compression — Product / DSP Design Notes

## Current product goal (v1.0.4)

**Stable baseline:** v1.0.4 Light / Classic UI switch (user-confirmed Stable on 2026-09-01). v1.0.4 changes only the visual theme system and keeps the v1.0.3 approved audio core, parameters, automation and current layout unchanged. v1.0.3 remains the previous Stable rollback reference.

QQ Super Compression is not designed by starting from a conventional Attack/Release compressor. The practical goal is to reduce dynamics while keeping audible waveform/tone changes as small as possible.

The project explored direct sample/amplitude remapping, but user testing established an important trade-off: attempting to eliminate the microscopic Lookahead boundary effect introduced more harmonic colour and much higher CPU/ASIO Guard load. The user therefore chose the older future-window peak design because it is substantially cleaner in real use.

**Priority:** clean/transparent result first. A small future-window pre-influence around abrupt level changes is acceptable and is not treated as a release-blocking defect.

## v1.0.1 transparent core

The audible path is delayed by the selected Lookahead. For each delayed sample, the detector uses the maximum magnitude in the matching future window.

```text
level = peak of the future window

gain = 1 / (1 + (Ratio - 1) * level)
```

There is no Attack or Release envelope. The window peak is deliberately used to avoid following the carrier sample-by-sample.

### Accepted Lookahead trade-off

Because the detector sees the future window, a sudden larger event can influence a short region before the event. Longer Lookahead expands that microscopic region. This behaviour is accepted because the same mechanism stabilises the gain over waveform cycles and greatly reduces the waveshaper-like harmonic distortion that appears when instantaneous sample height drives a nonlinear mapping.

## Threshold

Threshold is only a lower boundary around the same QQ law.

- OFF -> exact pre-Threshold law.
- finite Threshold -> level at/below boundary is unity; above it the same QQ curve is re-anchored continuously at the boundary.
- Threshold must never replace the future-window detector or introduce Attack, Release, knee, RMS smoothing, half-wave segmentation or any other alternate engine.

## Lookahead / Oversampling

Lookahead remains `0 / 10 / 26 / 40 / 80 / 100 ms`.

0 ms intentionally degenerates toward instantaneous sample-domain behaviour. The established anti-aliasing choice is therefore retained only there:

```text
0 ms -> 1x / 8x / 16x
10 ms+ -> fixed 1x
```

Do not add 2x/4x back unless explicitly requested; prior user PluginDoctor tests rejected them as insufficient in the old 0 ms flavour mode.

## ST / LR / MS domains

ST:
- one Ratio / Threshold / Makeup / Mix;
- linked L/R gain determined from the stronger current L/R future-window level.

LR:
- independent L/R Ratio / Threshold / Makeup / Mix.

MS:
- independent M/S Ratio / Threshold / Makeup / Mix.

All domains use the same future-window detector semantics. ST has its own Ratio/Threshold even though it reuses the already-computed L/R window levels.

## v1.0.3 centered domain Monitor

QQ Super Compression uses a **centered-only** domain audition monitor for normal plug-in/headphone reference. Do not copy QQ ChainScope's full SIP/in-place speaker-monitoring feature set into this product: ChainScope is intentionally special because it also serves studio loudspeaker monitoring.

- LR: ALL / L / R. L or R is copied to both outputs with `1/sqrt(2)` (-3.0103 dB) final listening compensation.
- MS: ALL / M / S. `M=(L+R)/2` is copied to both outputs at unity; `S=(L-R)/2` is copied to both outputs with `1/sqrt(2)`.
- The user explicitly rejected applying -3.01 dB to M.
- Monitor acts after the normal Output Gain result only for audible output. Display/Meter/Match remain pre-monitor.
- Monitor is project workflow state, not an automatable sound parameter and not an A/B snapshot member. LR and MS selections are stored separately.

## Relative LINK

One LINK button covers Ratio, Threshold, Makeup **and Mix** pairs in LR/MS.

LINK means equal **delta**, not equal **value**. Capture the pair at edit start, preserve their existing numerical difference, and stop both when either reaches a legal boundary. The same rule applies to normal drag, Shift fine drag and direct numeric entry.

Threshold OFF is conceptual `-inf`; exactly-one-OFF pairs cannot define a finite offset, so the OFF member stays OFF for that gesture.

## Dynamic Display

Display is a working part of Threshold operation. Users need to see the dynamic history clearly while choosing a boundary.

- v1.0.1 default design space is 1020x820.
- upper Display/Meter region gets 550 design px.
- lower controls remain compact at 140 design px.
- LR/MS use two stacked full-width histories.
- each active Threshold line shows its numeric value on the graph.

## Rejected Direct / Analytic experiment (v1.0.0)

The Direct experiment used analytic amplitude from a 4095-tap Hilbert FIR, mapped that amplitude nonlinearly, then reconstructed the sample phase/sign. It successfully made the user Lookahead irrelevant to the mapped sound, but user tests showed:

- audible/measurable harmonic colour remained;
- the Hilbert implementation caused large ASIO Guard / CPU load;
- two simultaneous instances could become very sluggish;
- the Direct mode offered no practical advantage over the cleaner legacy core.

The user explicitly chose to remove it. Keep this history so a future AI does not repeat the same experiment by accident.


---

## v1.0.1 domain Mix clarification

LR and MS are full independent processing domains, so Mix follows the same domain split as Ratio/Threshold/Makeup. In LR, L and R have separate Dry/Wet Mix. In MS, M and S have separate Dry/Wet Mix and the blend occurs before M/S decoding. ST keeps one shared Mix.

As of v1.0.2, the single LINK workflow button also covers Mix. Mix uses the same percentage-point relative-delta rule as Ratio/Threshold/Makeup, so independent domain balances can be moved together without collapsing their existing difference.
