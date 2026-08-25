# QQ Super Compression 0.1.8

**Vendor:** Qing Audio  
**Format:** VST3  
**Framework:** JUCE / CMake  
**License:** MIT  

**Status:** Candidate / Test — not yet user-confirmed Stable

QQ Super Compression is open-source software. The repository builds Windows VST3, macOS Apple Silicon VST3, macOS Intel VST3 and macOS Universal Audio Unit packages through GitHub Actions. The audio-processing behaviour is identical across these builds.

0.1.8 is a focused UI follow-up to 0.1.7. It keeps the 0.1.6 strict Integrated LUFS Match, future-window peak compression core and the 2-second automatic Gain Reduction Peak Hold unchanged. The Hold readout drops the redundant `H` prefix and uses a slightly larger value font, and editor resizing now scales the complete 1020x670 design uniformly in X/Y so the interface enlarges or shrinks at true 1:1 proportions instead of stretching width and height independently.

QQ Super Compression is an experimental threshold-free dynamics processor. It does not use a conventional compressor Threshold/Attack/Release envelope. The active core analyses a future waveform window, derives a level-dependent Ratio gain, applies that gain to the correspondingly delayed original waveform, then applies Makeup and Mix.

中文简介：QQ Super Compression 是一个没有传统 Threshold、Attack 和 Release 参数的实验性动态处理器。它使用未来窗口峰值分析、Ratio、Makeup、Mix 和可选 Lookahead；当前 0.1.8 仍是 Candidate/Test 版本。

## Why 0.1.6 exists

0.1.4 replaced the rejected 20 ms rolling-RMS detector with a future-window peak detector and let the user type any Lookahead from 0–100 ms. The user then built and tested 0.1.4 in PluginDoctor and found a very clear frequency/window relationship:

- 5 ms: distortion drops sharply above about 99.6 Hz;
- 10 ms: distortion drops sharply above about 49.8 Hz;
- 20 ms: distortion drops sharply above about 24.9 Hz;
- 26 ms: the sharp boundary moves to roughly the 20 Hz region;
- 40 ms: 20 Hz distortion is visibly lower than 26 ms;
- 80 ms: 20 Hz is clearly cleaner again;
- longer Lookahead continues to reduce the residual effect.

The user also confirmed that 0 ms, although measurably more harmonic/distorted, can be a useful audible flavour and should remain available.

0.1.5 ended the arbitrary-text experiment and exposed six deliberate Lookahead presets; 0.1.6 keeps them unchanged:

```text
0 ms
10 ms
26 ms
40 ms
80 ms
100 ms
```

## Signal flow

```text
Current Input
  |\
  | +--> fixed-preset future-window peak analysis
  |                    |
  |                    +--> threshold-free Ratio gain
  |
  +--> exact Lookahead delay --------------------------+
                                                        |
Delayed Dry --------------------------------------------+
  -> Ratio gain
  -> Wet (pre-Makeup)
  -> mode-dependent Makeup
       ST: one common Makeup
       LR: independent L / R Makeup
       MS: independent M / S Makeup
  -> Dry/Wet Mix
  -> Output
```

The selected Lookahead controls **both** the future analysis-window length and the real audio-path latency reported to the host for PDC. Bypass uses the same delayed Dry path, so Bypass does not silently change latency.

## LOOKAHEAD presets and persistence

The lower-right control is now a ComboBox with exactly:

`0 / 10 / 26 / 40 / 80 / 100 ms`.

The parameter ID remains `lookaheadMs` so 0.1.4 candidate projects/A-B state can still be read. Arbitrary legacy 0.1.4 values are migrated to the nearest approved preset (exact ties choose the longer preset) when state is restored.

Persistence rules:

- **Existing project / existing instance:** restores its own saved Lookahead preset from project state.
- **New instance:** starts from the user's last manually selected Lookahead preset.
- The last manual selection is stored in the same per-user `QQSuperCompression.settings` file already used for editor-size memory.
- If there is no previous user preference, the 0.1.5+ first-run fallback is **26 ms**, because it is the shortest preset the user found to move the sharp PluginDoctor boundary to approximately the 20 Hz region.

Changing Lookahead changes the host-reported latency. A DAW may perform a one-time PDC realignment when the preset changes during playback.

### 0 ms flavour

0 ms intentionally collapses the analysis to instantaneous sample magnitude. PluginDoctor shows measurable harmonic distortion in this mode, but the user found the audible effect subtle enough to keep as an intentional flavour option. Do not secretly smooth or add hidden Lookahead to 0 ms.

## Level analysis / Ratio law

The active detector remains the allocation-free sliding future-window peak engine introduced in 0.1.4. For a sample delayed by `N` samples, its control level is the maximum absolute level found across that sample and the following `N` future samples.

The Ratio law is deliberately unchanged:

```text
gain = 1 / (1 + (Ratio - 1) * level)
```

where `level` is the 0..1 lookahead-window peak.

- Ratio = 1:1 -> unity before Makeup/Mix;
- increasing Ratio -> more Gain Reduction / lower Wet;
- no conventional compression Threshold;
- no user Attack/Release envelope.

## Modes

### ST — Stereo Linked
- Meter domain: L/R.
- L/R lookahead analysers run independently.
- The stronger reduction controls one common gain for both channels.
- One shared ST Makeup value is used.

### LR — Left / Right independent
- Meter domain: L/R.
- L and R compression are independent.
- L and R have independent Makeup controls.

### MS — Mid / Side independent
- Meter domain: M/S.
- Encode: `M=(L+R)*0.5`, `S=(L-R)*0.5`.
- M and S are independently analysed/compressed.
- M and S have independent Makeup before decode.

Mode cycles `ST -> MS -> LR -> ST`.

## Match — strict Integrated LUFS (retained from 0.1.6)

The old RMS/power Match is removed from the active path. Match now uses the BS.1770 / EBU R128 Integrated Loudness structure:

- two-stage K-weighting per measured stream (head-related high shelf + RLB high-pass);
- 400 ms gating blocks;
- 75% overlap, i.e. 100 ms hop;
- absolute gate at **-70 LUFS**;
- relative gate at **-10 LU** from the absolute-gated loudness;
- incomplete block at the end is not used;
- Match value = `Dry Integrated LUFS - compressed Wet pre-Makeup Integrated LUFS`;
- Makeup result is limited to the existing ±36 dB parameter range.

Measurement source remains intentionally unchanged: **delayed Dry versus compressed Wet before Makeup and before Mix**. Therefore current Makeup and Mix do not contaminate the Match measurement.

Mode behaviour:

- **ST:** Dry L/R and linked-compressed Wet L/R are measured as one stereo BS.1770 programme and one common Makeup value is written.
- **LR:** L and R each run the same mono BS.1770 gated Integrated LUFS calculation; valid channels are matched independently.
- **MS:** M and S each run the same mono BS.1770 gated Integrated LUFS calculation; valid components are matched independently before MS decode. M/S are processing-domain components rather than a normative loudspeaker layout, but the per-component measurement uses the same K-weighting and two-stage BS.1770 gating math.
- If a channel/component never produces a valid gated Integrated LUFS result (for example a completely silent Side), Match leaves that Makeup value unchanged instead of forcing it to 0 dB.

The Match analyser runs only during host playback and resets at a new playback run / detected transport discontinuity according to the existing transport logic. At least one complete 400 ms block is required before Match can become available.

A JUCE-free reference sanity test is included at `tests/bs1770_match_selftest.cpp`.


## A/B

A/B snapshots include Ratio, ST/L/R/M/S Makeup, Mix, Lookahead and Mode. Bypass remains global and is not part of A/B.

## UI / workflow retained

- Narrow dual-meter panel; Dynamic Display receives the larger width.
- Resizable editor remembers the user's last size, but 0.1.8 constrains the window to the original 1020x670 aspect ratio and scales the complete UI uniformly. Old saved non-proportional sizes are migrated to the largest uniform scale that fits inside the previous rectangle.
- A / B / A→B / B→A.
- All rotary controls: Shift-drag fine, Alt+left-click reset, Ctrl/Cmd+Z Undo, Ctrl/Cmd+Shift+Z Redo.
- Bypass is a normal button.
- Mode is a cycle button.
- No explanatory subtitle line under the product title; a small low-contrast build version is shown for identification.

## Dynamic Display

- grey = delayed Dry / Input;
- cyan = compressed Wet pre-Makeup;
- yellow = final Output after Makeup and after Mix.

## Meters and 2 s GR Peak Hold

Three dual-channel groups: INPUT LEVEL, OUTPUT LEVEL, GAIN REDUCTION. ST/LR show L/R; MS shows M/S. Gain Reduction starts at 0 dB at the top and grows downward.

0.1.7 introduced an automatic **2 second Gain Reduction Peak Hold** to each active GR meter channel/component; 0.1.8 keeps the Hold algorithm unchanged:

- a deeper GR peak immediately moves the Hold marker and restarts the 2 s timer;
- if no deeper peak arrives for 2 s, the Hold marker automatically refreshes to the current GR instead of requiring a manual reset click;
- LR holds L/R independently; MS holds M/S independently; ST linked mode naturally shows the same linked GR on both channels;
- Hold is display-only and does not affect detector gain, audio, Makeup, Mix, Match, A/B or project parameters;
- changing Mode or Lookahead clears the stale Hold marker so a peak from a different analysis domain/window is not shown for another two seconds.

The GR bar continues to show current block GR while a small horizontal marker and a second numeric value show the recent held peak. In 0.1.8 the redundant `H` prefix is removed and the Hold value font is slightly larger for readability.

## UTF-8 / CJK safety

- source files stay UTF-8;
- MSVC keeps `/utf-8`;
- macOS uses PingFang SC fallback path;
- Windows uses Microsoft YaHei fallback path;
- do not add ANSI/GBK literals.

## Development handoff

Read `AI_DEVELOPMENT_HANDOFF.md` before modifying code. Every meaningful code change must append a development record before delivery.

## Build

See `CODEX_BUILD.md`.
