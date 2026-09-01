# QQ Super Compression — Product / DSP Design Notes

## Current product goal (v1.1.2 Plan A candidate)

v1.1.2 makes the Display describe QQ Super Compression's actual product semantics instead of conventional-compressor terminology.

- Mix is an essential compression-depth control. Effective GR is calculated from the linear Dry/compressed-Wet blend; it is not core GR dB multiplied by Mix.
- The dedicated GR meter, Hold, and historical GR visualization all include Mix. Makeup and Output Gain remain compensation/output controls and do not redefine GR.
- Wet pre-Makeup remains an internal signal required by Match and the audio blend, but no longer consumes a visible Display trace.
- The history stores pre-Input carrier level and actual future-window detector level. Current Input, Ratio, Threshold, Mix, Makeup, and Output Gain reproject the complete visible window.
- EXT adds a weak detector-key contour after Key Gain and HPF. It is detector context, not audible-carrier content.
- This is a Display/meter interpretation change only. The approved future-window DSP, Match measurement source, parameters, state schema 10, PDC, Monitor, Bypass, sidechain, HPF, and dual-theme layout remain intact.

中文摘要：Mix 是 QQ Super Compression 增益衰减定义的一部分；Display 与右侧 GR Meter 都必须显示包含 Mix 的有效 GR。Wet pre-Makeup 继续供 Match 与内部混合使用，但不再作为可见曲线。外部侧链激活时，Display 用弱化轮廓显示 post-Key-Gain/post-HPF 的实际检测 Key。

**Stable baseline remains:** v1.1.1 Side Chain HPF. **Current local candidate:** v1.1.2 after Plan A.
## Current product goal (v1.1.1 Stable)

v1.1.1 builds directly on v1.1.0 External Key and adds a detector-only Side Chain HPF. This is meaningful because a high-pass filter changes the frequency weighting and therefore the time-varying gain-reduction contour; it is not equivalent to changing Input Gain or Key Gain.

- OFF is exact v1.1.0 full-band key behaviour.
- Active 20-500 Hz uses a second-order Butterworth response with logarithmic control travel.
- Both INT and EXT can be filtered. EXT order is Key Gain, then HPF.
- Only detector, key meter, and SC Listen see the filtered signal; the carrier is never filtered.
- The approved future-window Peak/Lookahead law, Ratio, Threshold, domains, Mix, PDC, Match, Display, Monitor, Bypass, and theme behaviour remain unchanged.

**Stable baseline:** v1.1.1 after Plan A and Plan B on 2026-09-02. **Previous Stable rollback:** v1.1.0.

## Current product goal (v1.1.0 Stable)

**Current Stable:** v1.1.0 External Key, built directly from the user-confirmed v1.0.4 Stable baseline. The user explicitly promoted v1.1.0 on 2026-09-02 after Plan A; formal Plan B is complete. **Previous Stable rollback:** v1.0.4. Detailed Cubase sidechain, PDC, listening, and legacy-project checks remain recorded as manual follow-up.

QQ Super Compression is not designed by starting from a conventional Attack/Release compressor. The practical goal is to reduce dynamics while keeping audible waveform/tone changes as small as possible.

The project explored direct sample/amplitude remapping, but user testing established an important trade-off: attempting to eliminate the microscopic Lookahead boundary effect introduced more harmonic colour and much higher CPU/ASIO Guard load. The user therefore chose the older future-window peak design because it is substantially cleaner in real use.

**Priority:** clean/transparent result first. A small future-window pre-influence around abrupt level changes is acceptable and is not treated as a release-blocking defect.

## v1.1.0 External Key design contract

External Key is valuable because the existing future-window gain law can follow a drum key without adding a conventional Attack/Release envelope to the bass, pad, vocal, bus, or mastering carrier. The detector may change source; the approved carrier path and compression law may not.

- `INT`: exact v1.0.4 detector source, main signal after Input Gain.
- `EXT`: optional mono/stereo sidechain bus after dedicated Key Gain. External audio is detector/listen-only and must never leak into the normal output.
- A disconnected or silent external bus means a zero detector level and therefore unity gain; never mute or replace the main carrier.
- ST uses the stronger key L/R future-window level and one common ST gain.
- LR uses independent key L/R detectors; a mono key is duplicated to both.
- MS converts a stereo key with `M=(L+R)/2` and `S=(L-R)/2`; a mono external key deliberately drives both M and S detectors in common so the S path is not left untriggered.
- Key Source and Key Gain are sound parameters: APVTS, automation, project state, undo, migration, and A/B. They are appended after all v1.0.4 parameter IDs.
- `SC LISTEN` is a non-persistent safety audition override. It is not a host parameter or A/B member, is ignored by true Bypass, remains aligned to total plug-in latency, and resets OFF on panel/editor close and state restore.
- Source changes clear detector history only; they do not disturb the carrier/dry alignment. Lookahead changes rebuild queues from the selected key history.
- No Attack, Release, RMS envelope, key filter, or alternate compression curve is introduced in v1.1.0.

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
