# QQ Super Compression — Development History

## 0.1.1 Ratio Engine Fix / Meter & UI Pass — 2026-08-25

### Why 0.1.0 was wrong

The first prototype used the memoryless sample-domain curve:

```text
y = sign(x) * abs(x)^(1 / Ratio)
```

For normal digital audio, `abs(x)` is usually between 0 and 1. Raising such values to an exponent below 1 pushes them **up toward 1**. Therefore increasing Ratio made the signal louder instead of quieter.

More importantly, the curve was applied directly to every audio sample. That makes it a waveshaper, not an automation-like dynamics processor, and it creates strong harmonic distortion. The reported listening result (heavy distortion as soon as processing starts) was therefore expected from the 0.1.0 formula and was not an oversampling bug.

### 0.1.1 Ratio architecture

0.1.1 removes all sample-domain waveshaping from the Ratio stage.

The processor now:

1. measures linked channel power with a fixed 20 ms rolling RMS window;
2. converts that local level into one common gain value;
3. applies that gain equally to the delayed/aligned audio channels.

This is intentionally closer to drawing a volume automation envelope than to reshaping the audio waveform itself. There are still no user Attack or Release controls.

The current threshold-free prototype law is:

```text
gain = 1 / (1 + (Ratio - 1) * level)
```

where `level` is normalised to 0..1 and a full-scale sine-equivalent RMS maps to 1.

Consequences:

- Ratio = 1:1 -> unity gain.
- Larger Ratio always creates more Gain Reduction and a lower Wet level.
- At `level = 1`, Ratio 8:1 gives gain = 1/8 (-18.06 dB).
- At low levels the reduction continuously approaches 0 dB; there is no hard compression threshold.
- A steady-state sine receives a constant gain after the analysis window settles, so Ratio processing itself does not bend the sine wave into a waveshaper shape.

This is a custom threshold-free ratio law, not the standard dB-slope definition used by a conventional threshold compressor. Keep the engine isolated so it can be replaced after listening tests without rebuilding the UI, Makeup Gate, Mix or PDC architecture.

### 20 ms analysis / 0 ms and 10 ms modes

The Ratio detector uses a fixed 20 ms rolling RMS window rather than user Attack/Release controls.

- **0 ms:** the analysis is causal/trailing and is applied immediately.
- **10 ms Stable:** audio is delayed by 10 ms. The same 20 ms rolling window is then approximately centred around the audible delayed sample (10 ms before + 10 ms after), giving the intended pre-read/stable behaviour.

The 10 ms delay is still reported to the DAW with `setLatencySamples()`.

### PDC / Bypass regression rule

The selected latency never changes merely because Bypass is engaged.

- 0 ms mode + Bypass -> 0 ms actual / 0 ms reported.
- 10 ms mode + Bypass -> bypassed Dry still passes through the 10 ms delay and the host is still told 10 ms.

Do not regress this behaviour.

### Makeup Gate

Unchanged conceptually:

- detector source = Wet before Makeup;
- Gate does not mute audio;
- positive Makeup Gain is allowed above the line and transitions toward 0 dB below it;
- negative Makeup Gain remains global;
- fixed 10 ms gate-weight smoothing is continuity protection, not a user compressor Attack/Release.

### Three Meter panels added

The UI now includes three dedicated meters:

- **INPUT** level;
- **OUTPUT** level;
- **GAIN RED.** (Ratio-stage gain reduction before Makeup/Mix).

### Dynamic Display layout pass

The Dynamic Display was reorganised to stop labels from covering one another:

- a dedicated header row;
- a dedicated left dB-scale gutter;
- a dedicated right Makeup Gate value gutter;
- a dedicated bottom legend row;
- curves are clipped strictly to the plot area.

### UTF-8 / Chinese text fix

The 0.1.0 screenshot showed the Chinese regression sentence corrupted despite the project-level `/utf-8` setting.

0.1.1 no longer relies on raw Chinese source bytes for that UI sentence. It is encoded using C++ Unicode escape sequences inside a UTF-8 literal and then converted with `juce::String::fromUTF8()`.

The intended sentence is:

> 补偿门限只控制 Makeup Gain，不会切断低电平声音。

macOS still prefers PingFang SC and Windows prefers Microsoft YaHei through the LookAndFeel. This combination is intended to protect both encoding and glyph coverage.

---

## 0.1.0 Prototype — 2026-08-25

Initial VST3 prototype with Ratio, Makeup Gain, Makeup Gate, Mix, 0/10 ms latency, Dynamic Display and PDC-safe bypass architecture. The initial sample-domain Ratio formula was rejected after the first listening test because it increased level as Ratio rose and produced severe waveshaping distortion.

---

## 0.1.2 ST/MS/LR Modes / Zero Latency / Dual Meter Rework — 2026-08-25

**Status:** Candidate / Test  
**Based on:** 0.1.1  
**Stable baseline:** none has yet been explicitly designated by the user

### User request

After testing 0.1.1, the user reported that the core compression concept was working very well but requested the following simplifications and channel-domain features:

1. remove the 10 ms latency mode because listening tests suggested it had no practical value;
2. remove Makeup Gate because it also appeared unnecessary in actual use;
3. reverse Gain Reduction meter direction so 0 dB starts at the top and increasing reduction travels downward;
4. split Input / Output / Gain Reduction meters into two channels;
5. add ST / MS / LR processing modes;
6. in MS mode, meter labels and values must become M/S;
7. preserve the Dynamic Display meaning confirmed by the user: yellow Output is after Makeup and after Mix.

### Problem / design reason

0.1.1 still contained two experimental mechanisms (10 ms Stable latency and Makeup Gate) that were added before listening validation. User testing did not justify their complexity, so keeping them would add controls, state and PDC concerns without a demonstrated benefit.

The 0.1.1 meter panel was also single-channel/aggregate and its Gain Reduction bar grew upward, which did not match the requested compressor-style visual convention.

### Modification

- Removed `latencyMode` parameter and latency selector UI.
- Removed `StereoDelay` from the audio path and fixed reported plugin latency to 0 samples.
- Removed `makeupGateDb`, `MakeupGate`, gate smoothing, gate knob, gate graph line and gate value pill.
- Retained the 0.1.1 Ratio engine and fixed 20 ms rolling analysis unchanged.
- Added `processingMode` parameter with ST / MS / LR choices.
- Added four continuously warm detector engines: L, R, M and S.
- ST uses stereo-linked gain: the larger of current L/R reductions controls a common gain applied to both channels.
- LR uses independent L and R gains.
- MS uses `M=(L+R)*0.5`, `S=(L-R)*0.5`; M/S compress independently and decode with `L=M+S`, `R=M-S`.
- Ratio / Makeup / Mix remain one shared parameter set in all three modes.
- Reworked meter state into dual-channel Input / Output / Gain Reduction values.
- ST/LR meter labels are L/R; MS labels are M/S.
- Gain Reduction bars now start at the top and extend downward as reduction increases.
- Dynamic Display keeps three histories: Dry/Input, Wet pre-Makeup and final Output after Makeup + Mix.
- Removed the now-unneeded Makeup Gate interaction from Dynamic Display and widened the plot.
- Reworked graph header/legend allocation to reduce text overlap at small window sizes.
- Kept UTF-8 source rules, `/utf-8` on MSVC, and PingFang SC / Microsoft YaHei LookAndFeel fallback.

### Behaviour intentionally unchanged

- No conventional compression Threshold.
- No user Attack or Release controls.
- The custom 0.1.1 Ratio law remains unchanged.
- The 20 ms rolling level analysis remains unchanged.
- Makeup is manual only.
- Mix remains the final processing stage.
- Bypass outputs Dry and reports the same fixed 0-sample latency as active mode.

### Important implementation note

All L/R/M/S detector histories are processed continuously even when their mode is inactive. This is deliberate: switching mode should not start with an empty 20 ms analysis history. CPU cost is small because each detector is only a rolling power accumulator.

### Verification

- [x] Source-level/static inspection of modified files.
- [x] CMake syntax/configure path reaches the expected JUCE-not-found guard when run with FetchContent disabled; no CMake syntax error was found before that guard.
- [x] Removed stale source references to Makeup Gate / 10 ms latency from active CMake target and UI/DSP path.
- [x] ZIP/source-manifest integrity can be checked before delivery.
- [ ] Actual JUCE compile in this environment — not available at time of handoff.
- [ ] VST3 scan/load in DAW.
- [ ] Cubase audio test.
- [ ] User confirmation.

### Known / unverified items

- ST linked behaviour now uses the strongest L/R reduction rather than the 0.1.1 averaged linked detector. This is intentional for the new ST definition but must be auditioned because it can compress asymmetric stereo material more strongly than 0.1.1.
- Mode switching has no audible crossfade by design. All detectors are warm, but actual click/pop behaviour still needs DAW listening validation.
- M/S normalisation uses 0.5 encode. This was chosen to preserve detector level for pure Mid/pure Side cases, but subjective mode matching still needs user testing.

### Rollback

There is currently **no user-confirmed Stable baseline** for QQ Super Compression.

If 0.1.2 fails, use 0.1.1 as the immediate comparison/recovery source because it is the directly preceding candidate, but do **not** label 0.1.1 Stable unless the user explicitly does so.

Do not restore the rejected 0.1.0 sample-domain `abs(x)^(1/Ratio)` Ratio engine.

### Next priority

Build 0.1.2 VST3 and test ST/LR/MS behaviour, dual meters, GR direction, zero-latency host reporting, mode switching and the post-Mix yellow Output history in Cubase.

---

## 0.1.3 Workflow / A-B / Match / Independent Makeup — 2026-08-25

**Status:** Candidate / Test  
**Based on:** 0.1.2  
**Stable baseline:** none explicitly designated by the user

### User-requested workflow/UI changes

- Narrow the right dual-meter panel without reducing its height, giving Dynamic Display more width.
- Remove the subtitle under the product name completely.
- Remember the last resized editor width/height and restore it on the next open.
- Add A/B comparison plus A→B and B→A copy operations.
- Add Shift-drag fine control, Alt+left-click reset, Ctrl/Cmd+Z undo and Ctrl/Cmd+Shift+Z redo to all knobs.
- Replace checkbox-style Bypass with a normal button.
- Replace Mode ComboBox with a one-button ST→MS→LR cycle.
- Keep one ST Makeup, but add independent L/R Makeup in LR and independent M/S Makeup in MS.
- Add playback-integrated Match that automatically sets the relevant Makeup; LR/MS must be measured and written independently.

### Architecture

Ratio and the fixed 20 ms rolling detector were intentionally **not changed**.

Mode-dependent Makeup now sits after the corresponding compressed domain:

```text
ST: linked Wet L/R -> common ST Makeup -> Mix
LR: Wet L -> Makeup L --\
    Wet R -> Makeup R ---+-> Mix
MS: Wet M -> Makeup M --\
    Wet S -> Makeup S ---+-> M/S decode -> Mix
```

Match accumulates Dry vs Wet-pre-Makeup power during host playback for ST, L, R, M and S simultaneously. The current implementation converts the total power ratio to dB and writes it to Makeup. It is an RMS-equivalent integrated-level match, **not BS.1770 LUFS**.

### State / UX

- Existing `makeupGainDb` is retained as ST Makeup for candidate-project compatibility.
- Added L/R/M/S Makeup parameters.
- A/B snapshots include Ratio, all Makeup values, Mix and Mode; Bypass is deliberately excluded.
- A/B data is stored as extra APVTS root properties.
- Editor size is stored separately as a per-user PropertiesFile setting, not as an audio parameter/project setting.
- APVTS now uses a JUCE UndoManager.
- `EDITOR_WANTS_KEYBOARD_FOCUS` is enabled so the plug-in can receive its requested Undo/Redo shortcuts.
- Alt-reset still opens/closes the normal Slider/APVTS gesture so default-reset follows the same undo/host-gesture path as a normal knob edit.

### Verification status

- [x] Source/static review.
- [x] UTF-8 decode check across project text files.
- [x] Documentation/handoff updated.
- [x] Removed old subtitle and active ComboBox UI references.
- [x] CMake version updated to 0.1.3; configure with FetchContent disabled reaches the intentional JUCE-not-found guard without an earlier CMake syntax failure.
- [ ] Actual JUCE compile in this environment — unavailable because the container cannot resolve github.com and no JUCE checkout is installed.
- [ ] VST3 load/scan.
- [ ] Cubase interaction/audio test.
- [ ] User confirmation.

### Risks to test

- Cubase keyboard-shortcut ownership for Ctrl+Z / Ctrl+Shift+Z.
- PropertiesFile path/permissions on Windows and macOS.
- Transport Stop→Play Match accumulator reset behaviour when the host stops calling audio callbacks while stopped.
- Batch parameter writes from A/B and Match with host automation.

### Rollback

If 0.1.3 fails, return to 0.1.2 Candidate for direct comparison. Do not restore the rejected 0.1.0 waveshaper Ratio, the removed 10 ms latency mode, or Makeup Gate unless the user explicitly requests them again.


---

## 0.1.4 Variable Lookahead Peak Experiment — 2026-08-25

**Status:** Candidate / Test  
**Based on:** 0.1.3  
**Stable baseline:** none explicitly designated by the user

User PluginDoctor testing invalidated the fixed 20 ms rolling RMS detector as a final core: Dynamics showed an attack/release-like 20 ms time-window response, and Harmonic Analysis showed carrier-related odd harmonics on a stable sine. The user restated the intended architecture as a short future-waveform read followed by Ratio-derived volume reduction, then asked for an editable millisecond field so different preview lengths can be compared directly.

0.1.4 replaces the active rolling-RMS detector with an allocation-free sliding future-window peak analyser. The user parameter `lookaheadMs` is 0.0–100.0 ms, default 5.0 ms, and controls both the analyser window and the actual audio delay/PDC. L/R/M/S analysers use monotonic maximum queues. The audio stream is delayed by exactly N samples, while the detector sees current input, so gain computed from `[n..n+N]` is applied to delayed sample `n`.

Bypass keeps the same delayed Dry path and the same latency report. Lookahead is included in A/B state. The existing Ratio law is intentionally retained so only the detector variable changes in this experiment. 0 ms is deliberately left as a potentially distortion-heavy instantaneous reference.

Match remains the old integrated-power/RMS prototype because the user explicitly requested strict LUFS later but then asked to wait until the other changes are confirmed. Do not silently convert Match in 0.1.4.

Validation in the AI environment is static only. UTF-8 decoding and basic source checks passed; CMake reaches the intentional JUCE-not-found guard with FetchContent disabled; `StaticCompressionEngine` also compiles/runs against a minimal JUCE stub, which is not a full plugin compile. A model-side algorithm simulation suggested that future-window peak becomes effectively constant on a steady sine once the window is long enough to include representative waveform peaks; none of these checks are compiled VST3 validation. Codex must build the plug-in and the user must verify PluginDoctor Dynamics/Harmonic Analysis and Cubase PDC behaviour.


---

## 0.1.5 Fixed Lookahead Presets / Last-Choice Memory — 2026-08-25

**Status:** Candidate / Test  
**Based on:** 0.1.4

### User validation leading to this version

The user successfully built/tested 0.1.4 in PluginDoctor and mapped the peak-detector Lookahead behaviour. Reported sharp distortion-boundary observations were approximately 5 ms→99.6 Hz, 10 ms→49.8 Hz, 20 ms→24.9 Hz, and 26 ms→the 20 Hz region. The user additionally found 40 ms visibly cleaner than 26 ms at 20 Hz and 80 ms clearly cleaner again. 0 ms remained measurably distorted but sounded subtle/useful enough to keep as a flavour.

### User request

Stop exposing arbitrary milliseconds and replace the experiment field with fixed Lookahead choices: `0 / 10 / 26 / 40 / 80 / 100 ms`. Remember the user's selection so future new instances start with the last chosen value, while existing projects continue restoring their own value.

### Modification

- Replaced Lookahead TextEditor with a ComboBox containing exactly the six approved presets.
- Kept the APVTS ID `lookaheadMs` as a float for 0.1.4 candidate state compatibility; UI/DSP snap it to a preset.
- Legacy arbitrary 0.1.4 project/A-B Lookahead values are migrated to the nearest preset.
- New instances read `lastLookaheadMs` from the existing per-user settings file.
- Manual ComboBox choices write `lastLookaheadMs` immediately.
- Existing project state still overrides the new-instance default when the host restores the instance.
- First-run fallback is 26 ms, selected because it is the shortest approved preset corresponding to the user's observed ~20 Hz boundary.
- Host latency/PDC and Bypass delay continue to use the exact selected preset.

### Kept unchanged

- Future-window sliding peak core.
- Ratio law.
- ST/MS/LR topology and independent Makeup behaviour.
- A/B, Mix, Dynamic Display, meters, window-size memory, rotary interaction and UTF-8 rules.
- Match is still the old RMS/power prototype; strict LUFS remains a confirmed pending requirement and is not implemented in this focused pass.

### Validation

- [x] User PluginDoctor validation of the 0.1.4 underlying future-window experiment (actual external test).
- [x] Source/static preset mapping and UTF-8 checks in this 0.1.5 source pass.
- [ ] JUCE/VST3 compile in current AI environment.
- [ ] Codex Windows Release compile.
- [ ] VST3 scan/load.
- [ ] Cubase preset persistence/new-instance-default/PDC/Bypass regression.
- [ ] User confirmation of 0.1.5.

### Rollback

There is still no user-designated Stable version. If 0.1.5 preset/persistence logic fails, return to 0.1.4 Candidate for the proven arbitrary Lookahead experiment, while retaining the user's PluginDoctor findings. Do not return to the rejected rolling-RMS or sample-waveshaper cores.


---

## 0.1.6 Strict Integrated LUFS Match — 2026-08-25

**Status:** Candidate / Test  
**Based on:** 0.1.5 Candidate

### User requirement

Replace the temporary RMS/power Match with strict LUFS Match. The user explicitly rejected RMS as the final Match definition.

### Previous behaviour / root cause

0.1.3–0.1.5 accumulated unweighted Dry/Wet sample power and converted the total power ratio to dB. That is an RMS-equivalent integrated level difference, not LUFS: it had no K-weighting, no 400 ms gating blocks, no -70 LUFS absolute gate and no -10 LU relative gate.

### Modification

- Added `Source/BS1770LoudnessMatch.h`.
- Added BS.1770-compatible two-stage K-weighting derived per sample rate. At 48 kHz the coefficients reproduce the tabulated BS.1770 coefficients.
- Integrated measurement uses 400 ms blocks with 100 ms hop (75% overlap).
- Applies the -70 LUFS absolute gate and the relative gate 10 LU below the absolute-gated loudness.
- ST stores stereo summed K-weighted block energy; LR and MS store independent mono component block energies.
- Match source remains delayed Dry versus compressed Wet pre-Makeup/pre-Mix.
- Match dB is `Dry LUFS - Wet LUFS`, limited to the existing ±36 dB Makeup range.
- Silent/invalid LR/MS components are not forcibly reset; only valid component Makeup parameters are written.
- Existing Ratio/lookahead engine, six Lookahead presets, A/B, UI, Mix and mode topology are unchanged.

### Why this implementation

The user wants perceptual programme-loudness matching rather than raw energy matching. BS.1770/EBU R128 Integrated Loudness provides the requested K-weighted, gated definition. M/S is not a normative loudspeaker layout, so M and S are each treated as a mono signal using the same BS.1770 K-weighting and two-stage gating math, which preserves the user's requirement that M and S be calculated and adjusted independently.

### Validation

- [x] Standalone C++ compile/test of the new LUFS core.
- [x] Fixed-level test: Dry at -20 dBFS sine vs Wet at -26 dBFS sine returns approximately +6.00 dB Match for ST/L/M paths.
- [x] 48 kHz full-scale 1 kHz mono sine reference test returns approximately -3.004 LUFS, consistent with the BS.1770 -3.01 LKFS reference point.
- [x] Cross-check against `pyloudnorm` DeMan/BS.1770 reference implementation on a 30 s gated 997 Hz test at 44.1 / 48 / 96 kHz: results matched to displayed precision (about -23.0731 / -23.0759 / -23.0932 LUFS).
- [x] UTF-8/source static checks.
- [ ] Full JUCE/VST3 compile in current AI environment.
- [ ] Cubase playback/Match validation.
- [ ] Comparison against an independent trusted LUFS meter on programme material.
- [ ] User confirmation.

### Known risks / open validation

- The existing host transport reset logic still requires Cubase Stop→Play testing, especially if the host stops audio callbacks while stopped.
- Integrated results require at least one complete 400 ms block.
- Block storage is pre-reserved for four hours to avoid normal audio-thread allocation; exceptionally longer continuous measurements may cause vector growth at a 100 ms block boundary.
- The M/S per-component values use strict mono BS.1770 measurement math, but M/S components themselves are not a standard BS.1770 loudspeaker programme layout.

### Rollback

There is still no user-designated Stable version. If 0.1.6 LUFS Match fails, return to 0.1.5 Candidate for the same compression/lookahead/UI core and rework Match only. Do not regress the Ratio core to rolling RMS or the 0.1.0 waveshaper.


---

## v0.1.7 — Auto GR Peak Hold / Version Tag / playHead Warning Cleanup

**日期：** 2026-08-25  
**状态：** Candidate / Test  
**基于：** v0.1.6

### 0.1.6 后续用户验证补记

- 用户实际确认：0.1.6 的 LUFS Match “没问题了”。这属于用户实际验证，但用户没有把 0.1.6 明确指定为 Stable，因此仍不得改写为稳定基线。
- 用户的 0.1.6/Codex 编译出现一个非致命的 `playHead` 名称遮蔽 warning；功能可运行，但要求下一版注意消除。

### 用户需求

用户要求下一版在面板上显示当前版本号，字号小、低调；同时参考 DB-5035 的 Gain Reduction Hold 思路，但不要采用 DB-5035 的“点击后才清零”。QQ Super Compression 的 Hold 应保持一段时间后自动刷新。用户确认采用此前讨论的 2 秒自动 Hold 方案。

### 问题表现 / 设计原因

现有 GR Meter 只显示当前 audio block 的最大 Gain Reduction，短暂深压缩很容易在视觉刷新后消失。DB-5035 的手动清零 Hold 适合长期记录最大值，但本插件希望更适合连续观察，因此采用最近峰值短时保持并自动更新。

### 修改内容

- `MeterState` 新增两路 `gainReductionHoldDb` 原子显示值。
- Processor 在 audio thread 根据每个 audio block 已捕获的最大 GR 更新 Hold，因此不会依赖 30 Hz GUI repaint 去抓峰值。
- Hold 时间固定 2.0 s（按当前 sample rate 换算 samples）。
- 任一路出现更深 GR：立即更新 Hold，并从该峰重新计时 2 s。
- 2 s 内没有更深峰：Hold 保持；时间到后自动刷新到当前 block GR，并开始新的观察周期。
- ST 使用 linked GR，因此两路 Hold 自然一致；LR 独立 L/R；MS 独立 M/S。
- Mode 或 Lookahead 改变时清除旧 Hold，避免把旧域/旧 detector-window 的峰值继续显示。
- GR Meter 增加白色小横线 Hold marker，并在原数值区增加低调 `H x.x` 数值；当前 GR bar 继续实时移动。
- 把 `if (auto* playHead = getPlayHead())` 改为 `hostPlayHead`，针对用户报告的非致命名称遮蔽 warning。
- 面板新增低调版本号，直接使用 `JucePlugin_VersionString`，使 UI 显示与 CMake/JUCE binary version 同源；本版 CMake version 更新为 0.1.7。

### 保持不变

- 0.1.6 strict Integrated LUFS Match 不改。
- future-window peak detector、Ratio law、0/10/26/40/80/100 ms Lookahead、PDC/Bypass 不改。
- ST/MS/LR audio topology、Makeup、Mix、A/B、Dynamic Display、Undo/Redo、窗口尺寸记忆不改。
- Hold 不是音频参数，不进入 APVTS、A/B 或工程状态，不改变实际 Gain Reduction。

### 为什么这样修改

Hold 必须在 audio thread 侧基于 block peak 更新，而不是只在 GUI 30 Hz Timer 内做，否则 GUI 可能错过两个 repaint 之间的短 GR 峰。2 秒倒计时采用 sample 数，播放过程中不依赖 UI 帧率。Hold 是纯 Meter 状态，从而不会污染 DSP 或工程兼容性。

### 验证

- [x] 源码静态检查：两路 Hold 数据流、Mode/Lookahead reset、UI marker/value 路径已连接。
- [x] 源码检索确认局部变量 `playHead` 已移除，使用 `hostPlayHead`。
- [x] CMake project version 更新为 0.1.7，面板版本文字从 `JucePlugin_VersionString` 读取。
- [x] UTF-8 文本检查。
- [ ] 当前 AI 环境完整 JUCE/VST3 编译。
- [ ] Codex Windows Release 编译及 warning 检查。
- [ ] Cubase 2 秒 Hold 实测。
- [ ] 用户确认 0.1.7。

### 已知问题 / 边界

- 2 秒 Hold 只在音频 callback 推进时按 samples 计时；如果宿主停止后完全不再调用 audio callback，Hold 不会靠墙钟时间自行变化。重新播放后会继续按音频时间刷新。
- Hold 刷新使用每个 audio block 的最大 GR，计时精度存在最多约一个 block 的误差；这远小于 2 秒目标且保证不会漏掉 block 内峰值。
- 版本号视觉位置/透明度仍需用户看实际面板确认是否足够低调。

### 回滚

当前仍没有用户明确指定 Stable。若 0.1.7 的 Hold/UI 有问题，优先回滚到 v0.1.6 Candidate（用户已确认 LUFS Match 没问题），只重做 Meter Hold/UI；不要回退 LUFS Match 或 future-window peak 核心。


---

## v0.1.8 — GR Hold Readability / Uniform 1:1 UI Scaling

**日期：** 2026-08-25  
**状态：** Candidate / Test  
**基于：** v0.1.7

### 用户需求

用户在实际面板截图中指出 Gain Reduction Hold 的第二行 `H x.x` 太小、不易辨认，认为 Hold marker/行为本身已经足以表达含义，因此要求去掉 `H`。随后补充指出当前可缩放 UI 并不是“1:1 放大”，要求整个界面按相同 X/Y 比例统一缩放。

### 问题表现 / 根因

- Hold 数值区只有很窄的第二行，0.1.7 使用 7.5 px 字号且还占用 `H ` 两个字符，导致实际缩小窗口时尤其难读。
- 0.1.7 的 editor 允许 width/height 独立变化；`resized()` 只重排/拉伸部分 Rectangle，但字体、描边、控件固有像素尺寸并不会作为一个整体同比例缩放，因此例如 920x670 这类保存尺寸会把界面横向压扁，而不是把 1020x670 设计按一个统一 scale 缩小。

### 修改内容

- GR Hold 第二行从 `H 10.6` 改为仅显示 `10.6`；Hold font 从 7.5 px 提高到 8.5 px。白色 Hold 横线、2 s timing 和 audio-thread peak capture 不改。
- 新增 `contentRoot`：所有 UI widget 放入固定 1020x670 设计坐标根组件。
- Editor `resized()` 只计算一个统一 `uiScale`，对 `contentRoot` 使用单一 `AffineTransform::scale(uiScale)`；因此字体、Meter、Knob、Button、间距和线条一起同比例缩放。
- Editor bounds constrainer 固定 aspect ratio = 1020/670，禁止 X/Y 独立拉伸。
- 原有非比例 `editorWidth/editorHeight` 保存值在 0.1.8 首次打开时取能容纳于旧矩形内的最大统一 scale，再转换成比例窗口；以后继续保存比例尺寸。
- CMake product version 更新为 0.1.8；参数 ID、APVTS/state schema 不变。

### 保持不变

- 0.1.7 自动 GR Hold 的 2 s 逻辑、ST/LR/MS 域行为与 Mode/Lookahead reset 不变。
- 0.1.6 strict BS.1770 / EBU R128 Integrated LUFS Match 不变。
- future-window peak detector、Ratio law、六档 Lookahead、PDC/Bypass、A/B、Makeup、Mix、Dynamic Display 音频数据、Undo/Redo 全部不改。

### 验证

- [x] 源码静态检查：Hold 文本不再包含 `H `，字号更新为 8.5。
- [x] 源码静态检查：所有 UI 控件均已迁移到一个 `contentRoot`；layout 使用固定 1020x670 设计空间，root 只接受单一 X/Y 相同 scale。
- [x] 数学检查：min/max resize bounds 与 1020/670 aspect 一致；旧非比例 size 迁移算法只产生统一 scale。
- [x] UTF-8 检查。
- [ ] 当前 AI 环境完整 JUCE/VST3 编译。
- [ ] Codex Windows Release 编译。
- [ ] Cubase 实际 resize / mouse hit-test / reopening saved size 验证。
- [ ] 用户确认 0.1.8。

### 已知问题 / 边界

- 当前环境无 JUCE checkout，因此不能把静态 UI 检查写成已编译/宿主验证。
- JUCE host/editor resize 边界和不同系统 DPI 下的实际交互仍需 Cubase/Windows/macOS 实测；设计目标是插件内部只使用一个 uniform scale，不额外抵消宿主 DPI scale。

### 回滚

当前仍无用户指定 Stable。若 0.1.8 的统一缩放在宿主中有问题，回到 0.1.7 Candidate，只重做 Editor scaling；不要回退 0.1.7 Hold algorithm、0.1.6 LUFS Match 或 future-window peak 核心。
