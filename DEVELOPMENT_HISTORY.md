# QQ Super Compression — Development History

## v1.0.3 — Centered Domain Monitor

**日期：** 2026-08-30  
**状态：** Candidate / Test  
**基于：** v1.0.2 Complete Relative LINK Stable

### 目标与实现

本版只增加 LR/MS 的居中试听工作流，不改透明 future-window Peak/Lookahead 压缩核心、Threshold、独立域控制、Relative LINK、Match、Display、meters、Oversampling、PDC 或 Bypass。

- LR：`ALL / L / R`；选择 L 或 R 时把选中通道乘以 `1/sqrt(2)` 后同相复制到双输出。
- MS：`ALL / M / S`；M 以 unity 同相复制到双输出，S 以 `1/sqrt(2)` 同相复制到双输出。
- ST：不显示 Monitor。
- Monitor 位于最终可听输出；Display、meters、Match 和正常处理结果仍使用 Monitor 前的信号。
- LR 与 MS 分别保存选择；状态 schema 由 7 升至 8。Monitor 不进入 APVTS、宿主自动化或 A/B 声音快照；True Bypass 不应用 Monitor。

Version 1.0.3 adds a centered audition Monitor only. It does not alter the transparent future-window Peak/Lookahead compressor core, Threshold, domain controls, Relative LINK, Match, Display, meters, Oversampling, PDC, or Bypass. LR offers ALL/L/R and MS offers ALL/M/S; L/R/S use `1/sqrt(2)` compensation when copied to both outputs, while M remains unity. Monitor is final-audible-only and persists separately for LR/MS outside host automation and A/B snapshots.

### 验证与限制

- [x] source manifest 与七项 Python/source/math 自测。
- [x] JUCE 8.0.15 / MSVC Windows x64 Release、BS.1770 与 Steinberg validator。
- [ ] Plan D 同一 commit 的 macOS arm64 VST3、Intel VST3、Universal 2 AU 与 `auval`。
- [ ] 用户 Cubase 试听、Display/Meter/Match 隔离、工程恢复、A/B、自动化和旧工程迁移复核。

在 Plan D 与用户试听完成前，本版保持 Candidate/Test；v1.0.2 是稳定回滚基线。

---

## v1.0.2 — Complete Relative LINK

**日期：** 2026-08-30  
**状态：** Candidate / Test  
**基于：** v1.0.1 Stable — Display 0…-90 dB Scale / Transparent Core / Independent Domains  
**稳定回滚基线：** v1.0.1 Stable（用户已确认，Plan A/B/C/D 已执行）

### 用户需求

在完善 v1.0.1 中英文说明书时，用户实测发现两处 LINK 工作流缺口：

1. LR/MS 的 Mix 已经拆成独立双参数，但 LINK 仍只联动 Ratio / Threshold / Makeup；用户明确要求 LINK 必须同时控制 Mix。
2. LINK 开启时，鼠标拖动可保持相对关系，但双击数值框直接输入新值不会联动另一侧；用户明确要求直接输入也必须遵守 LINK。

### 根因

- v1.0.1 Revision 2 新增独立 LR/MS Mix 时，为了遵守当时已确认的三参数 LINK 规格，没有把 Mix 注册进 `LinkedPair`、gesture-start 和 value-change 路径。
- 现有 LINK 依赖 `FineKnob::mouseDown()` 调用 `beginLinkedGesture()` 来捕获 source/target 起始值；JUCE 的 TextBox 数值提交不经过 mouse-drag gesture，因此 `onValueChange` 发生时 `activeLinkedPair` 为 none，LINK 正常地提前返回。

### 修改内容

- `LinkedPair` 新增 `mixLR / mixMS`。
- LR/MS Mix 加入与 Ratio/Threshold/Makeup 相同的 relative-link drag/Shift gesture 路由。
- LINK tooltip 更新为 `Ratio / Threshold / Makeup / Mix`。
- 新增 `handleLinkedTextEntry()`：在 Slider `valueFromTextFunction` 提交直接输入时，捕获当前 source/target 值并应用同一 shared-delta / shared-boundary 规则。
- Ratio、Threshold、Makeup、Mix 的 L/R 与 M/S 数值输入全部经过这条路径。
- Threshold OFF 继续使用既有特殊语义：若仅一侧 OFF，不制造有限 dB offset；若两侧都 OFF，可从相同有限值一起进入。
- 版本号更新为 1.0.2。

### 保持不变

- Future-window Peak / Lookahead DSP 完全不改。
- Threshold DSP law、0 ms Oversampling、PDC、Mix DSP 位置、LR/MS 域路由、Display、Meter、LUFS Match、A/B sound snapshot 和 state schema 不改。
- LINK 仍然是“保持相对差值”，绝不在开启时强制两侧变成相同数值。
- v1.0.1 继续作为 Stable 回滚基线。

### 验证

- [x] `domain_link_selftest.py`：新增 Mix、direct-entry、boundary 用例通过。
- [x] `link_ui_source_selftest.py`：确认 Mix gesture/value wiring 与四组 direct-entry routing 存在。
- [x] 既有 transparent core / Threshold / independent Mix / Display self-test 计划回归。
- [ ] JUCE/MSVC VST3 实际编译。
- [ ] Cubase：LINK + Mix 鼠标/Shift 联动。
- [ ] Cubase：LINK + Ratio/Threshold/Makeup/Mix 双击直接输入。
- [ ] 用户确认并决定是否晋升 Stable。

### 回滚

若 v1.0.2 LINK 完整化出现问题，直接回滚到 **v1.0.1 Stable**。不得因此回滚透明 DSP、Threshold、独立 LR/MS Mix、Display 0…-90 dB 或 Mode/Lookahead 已确认 UI。

---

## v1.0.1 — Stable Baseline Promotion

**日期：** 2026-08-30  
**状态：** Stable  

用户明确将 Display `0…-90 dB` 的 v1.0.1 设为新的稳定基线，并报告已执行 Plan A/B/C/D。此前 v1.0.1 各 Candidate Revision 的历史记录继续保留在下方；从此后续开发和失败回滚优先以该 Stable 包为基准。

## v1.0.1 Candidate Revision 3 — Mode / Lookahead Alignment Polish

**日期：** 2026-08-30  
**状态：** Candidate / Test（仍为 1.0.1，不晋升 Stable）  
**基于：** v1.0.1 Candidate Revision 2

### 用户需求

用户确认右下技术控制区功能已经恢复，但视觉比例过于笨重。经讨论后明确规格：Mode 仍为单击循环按钮，不改成下拉；Lookahead 因选项较多继续保留下拉菜单；Mode 主按钮必须与 Lookahead 菜单**完全同宽、同高、同一水平起点**；LINK 只是 Mode 右侧的小型辅助按钮，不能挤占 Mode 主按钮长度。用户明确要求先讨论确认后再修改 UI。

### 修改内容

- Mode 主按钮固定为 `108 x 23` design px。
- Lookahead ComboBox 固定为完全相同的 `108 x 23` design px，并与 Mode 主按钮共享同一个 `choiceX`。
- LINK 改为独立 `34 x 23` 小按钮，与 Mode 间隔 6 px；ST 隐藏，LR/MS 显示。
- Oversampling（仅 0 ms 显示）也沿用相同主控件水平对齐，保持技术控制列规整。
- `resized()` 仍是这些控件唯一的 bounds owner；`updateModeUi()` 不改变 geometry。

### 保持不变

- Transparent Future-Window Peak / Lookahead DSP 完全不改。
- Threshold、LR/MS 独立 Ratio/Threshold/Makeup/Mix、Relative LINK 语义、Display/Meter 尺寸、Oversampling 算法、PDC、A/B、Undo/Redo、项目兼容均不改。
- Mode 交互仍为 `ST -> MS -> LR -> ST` 单击循环；Lookahead 仍为下拉菜单。

### 验证

- [x] 源码静态检查：Mode 与 Lookahead 使用同一 `primaryChoiceW/H` 和 `choiceX`。
- [x] LINK 不再从 Mode bounds 中 `removeFromRight()`，因此不会改变 Mode 长度。
- [x] 现有 Python math/source self-tests 继续通过。
- [ ] JUCE/MSVC VST3 完整编译。
- [ ] Cubase 实际视觉确认。

### 回滚

如本轮纯 UI 对齐修改失败，回滚至 v1.0.1 Candidate Revision 2；不要回滚 DSP、独立 Mix、Threshold 上下布局或 Relative LINK 语义。

---

## v1.0.1 Candidate Revision 2 — Mode/LINK UI Fix + Independent LR/MS Mix

**日期：** 2026-08-30  
**状态：** Candidate / Test（仍为 1.0.1，不晋升 Stable）  
**基于：** v1.0.1 Transparent Core / Independent Domains / Display-First Candidate

### 用户需求

用户在实际构建截图中发现：Mode 切换控件和 LINK 按钮没有显示；同时要求 LR/MS 模式下 Mix 也和 Ratio/Threshold/Makeup 一样按两个域独立。用户进一步指出，LR/MS Display 已经是上下排列，因此 Threshold 两个控制也应上下排列而不是左右并排。

### 根因 / 设计判断

- Mode/LINK 在 `resized()` 先分配 bounds，但 `updateModeUi()` 又由 Timer 周期性重新根据当前 bounds 做 union/split，造成几何归属不唯一；在宿主实际 UI 生命周期中出现按钮不可见/不可用的回归。修复原则是让 `resized()` 成为唯一 bounds owner。
- 旧 Mix 是一个共享参数。若 LR/MS 已经允许两个域独立 Ratio/Threshold/Makeup，那么共享 Mix 会阻止两个域真正独立的 Dry/Wet 比例。
- MS 独立 Mix 必须在 M/S 域内分别混合 Dry/Wet 后再 decode 到 L/R；不能先 decode 再用两个 L/R Mix，否则语义会错误。
- 用户此前明确 LINK 是一个按钮同时锁 Ratio/Threshold/Makeup 的相对变化；本次没有擅自把新增 Mix 纳入 LINK。

### 修改内容

- Mode 按钮使用固定 geometry，并在所有模式始终可见；LINK 固定占 Mode 行右侧，在 LR/MS 显示，ST 隐藏。`updateModeUi()` 不再修改两者 bounds。
- 新增 `mixL/mixR/mixM/mixS` 参数；旧 `mix` 保留为 ST/legacy ID。
- LR：L/R Mix 独立。
- MS：M/S Mix 独立，并在 M/S 域分别完成 Dry/Wet mix 后再重建 L/R。
- 新增四个 Mix smoother、UI knob、APVTS attachment、A/B snapshot/state 字段。
- 旧工程若缺少新 Mix 参数，则从旧共享 Mix 精确复制到 L/R/M/S；旧 A/B 若缺少新属性也从各槽旧 Mix 回填。
- Threshold UI 由左右双 fader 改为上下双 fader：上方对应 L/M Display，下方对应 R/S Display；ST 仍使用一个全高 Threshold。
- Mix UI 在 LR/MS 使用两只小旋钮并显示 L/R 或 M/S 标签；ST 仍显示单一 Mix。

### 保持不变

- Future-window Peak / Lookahead DSP、Threshold 数学、0 ms Oversampling、PDC、Input/Output Gain、LUFS Match、A/B、GR Hold 均不改。
- Display 大小维持 1020x820 design root / 550 px visual row，不缩小。
- LINK 仍只联动 Ratio / Threshold / Makeup，并保持相对差值，不强制相等。

### 验证

- [x] 原 Threshold / Relative LINK / Transparent Core Python 自测继续通过。
- [x] 新 `independent_mix_selftest.py`：200000 随机样本验证 M/S 两边 Mix 相等时与旧共享 Mix 数学等价；验证不等 Mix 时两个域保持独立。
- [x] Source 静态检查确认新 Mix 参数已贯穿 APVTS / smoother / A-B / migration / UI。
- [ ] JUCE/MSVC VST3 完整编译。
- [ ] Cubase 中 Mode/LINK 可见性实测。
- [ ] LR/MS 独立 Mix 声音与 automation 实测。
- [ ] Threshold 上下布局用户视觉确认。

### 回滚

如果本轮 UI/Mix 修改失败，回滚到前一份 v1.0.1 Candidate 源码；不要回滚透明 DSP 核心，不恢复 Direct/Analytic，也不要回到 v0.9.5/v0.9.6。

---

## v1.0.1 — Transparent Core / Independent Domains / Display-First

**日期：** 2026-08-30  
**状态：** Candidate / Test  
**基于：** v0.9.4/v0.9.7 transparent future-window core + v1.0.0 domain/UI workflow work

### 用户需求

用户实际验证 v1.0.0 Direct/Analytic 方案后确认：Lookahead 的确不再改变声音，但 Direct 模式仍产生谐波失真，而且 ASIO Guard / CPU 负担明显变大，多开实例会卡。用户明确表示 Direct 模式没有保留必要，希望只保留旧的透明 Lookahead 算法。

同时保留刚刚确定的正式功能：ST/LR/MS 独立域参数、LR/MS 双 Ratio/双 Threshold/双 Makeup、一个保持相对差值的 LINK、LR/MS 独立 Display。用户再次强调 Display 太小，使用 Threshold 时需要清晰观察动态并寻找阈值点。版本号指定为 1.0.1。

### 问题表现 / 根因

- Direct/Analytic 路线虽然让用户 Lookahead 只剩纯延迟，但非线性幅度映射仍会产生谐波；4095-tap Hilbert FIR 还带来很高持续计算成本。
- 旧 future-window peak 路线在突然电平变化前存在 Lookahead 微观预影响，但用户明确接受这一点，因为实际目标是更干净透明的声音，而不是追求微观边界几何绝对不变。
- v1.0.0 的 Oversampling 被隐藏；恢复旧核心后必须恢复 0 ms-only 1x/8x/16x 逻辑。
- v1.0.0 LINK 与 Oversampling 共用同一 UI bounds；恢复 Oversampling 后在 LR/MS + 0 ms 会发生重叠，因此 LINK 改放 Mode 行。
- 1020x670 / 405 px visual row 即使已经比旧版放大，LR/MS 双 Display 仍然太窄高，不利于 Threshold 工作。

### 修改内容

- 删除活动 Direct/Analytic/Hilbert DSP；`StaticCompressionEngine` 恢复 future-window monotonic peak queue。
- Threshold OFF 精确恢复旧 QQ gain law；有限 Threshold 只作为下边界，不改变 detector。
- LR/MS 四个域继续独立 Ratio/Threshold；ST 使用自己的 Ratio/Threshold，并从当前 L/R window peak 中取更强 level 计算单一 linked gain。
- 0 ms 恢复 1x/8x/16x Oversampling；10 ms+ 固定 1x并隐藏 Oversampling UI；PDC/Dry/Bypass 恢复旧总延迟逻辑。
- 保留 Relative LINK：相同数值 delta、保持原差值、任一端触边即一起停止；不把两个值强制变相同。
- Editor 设计空间改为 1020x820；visual row 550 px，controls 140 px；Meter/Threshold 侧栏略缩窄。
- LR/MS 继续上下两条全宽 Display；Threshold 虚线增加数值标签。
- LINK 移到 Mode 行，避免与 0 ms Oversampling 重叠。
- 1.0.0 Direct/Analytic self-test 移入 `tests/archive/`，保留失败实验历史；新增 transparent core 自测。

### 保持不变

- Input Gain / Output Gain、Mix、LUFS Match、A/B、Undo/Redo、GR Hold、Mode 循环、工程状态迁移逻辑保持。
- Threshold 0.01 dB、Shift fine、Alt reset、双击输入保持。
- 失败的 v0.9.5/v0.9.6 不重新作为算法基线。

### 验证

- [x] Threshold OFF 200000 组随机数学回归通过。
- [x] Threshold 边界连续性自测通过。
- [x] Relative LINK 自测通过。
- [x] Future-window 400 Hz / 26 ms 稳态正弦透明度参考自测通过；0 ms 明显更非线性，符合既有产品定义。
- [x] 源码静态 grep：活动 Source 中无 Analytic/Hilbert/mapAmplitude/reconstructReal 路径。
- [ ] JUCE/MSVC VST3 完整编译。
- [ ] Cubase 扫描/工程恢复。
- [ ] PluginDoctor 动态/谐波/0 ms Oversampling。
- [ ] 用户确认 Display 可读性、LR/MS/Link/Threshold 实际行为。

### 已知问题

- Future-window detector 的 Lookahead 微观预影响是当前明确接受的设计取舍，不应再当作必须消除的 Bug。
- 当前环境没有 JUCE checkout，因此尚未完成真实插件编译。

### 回滚

若 v1.0.1 新域/界面整合失败，DSP 优先回滚到 v0.9.4/v0.9.7 future-window 核心；不要回到 v1.0.0 Direct/Analytic，也不要使用 v0.9.5/v0.9.6 作为回滚点。

---

## 1.0.0 — Independent domains + relative Link / Direct-Analytic experiment (REJECTED)

User clarified the intended architecture: the processor should not be designed as a conventional compressor and should not use `abs(sample)` as the sound amplitude. At that time the analytic-amplitude candidate was used as the experimental core with Lookahead isolated as pure delay. User testing later rejected this DSP path in v1.0.1.

For 1.0.0 the user specified that LR and MS each require **two Ratio, two Threshold and two Makeup** values, and that their Dynamic Displays must also be split by domain. A single LINK button covers all three pair types. Critically, Link means **relative movement**, not equality: existing offsets are preserved.

Implementation notes:

- legacy `ratio` / `thresholdDb` remain ST IDs; new LR/MS IDs are appended for migration safety;
- old projects copy their common Ratio/Threshold into all missing domain values;
- A/B stores all sound values but not Link workflow state; project state stores Link;
- LR/MS display uses stacked full-width histories rather than narrow side-by-side plots;
- pair-boundary logic stops both linked parameters to prevent offset collapse;
- Threshold OFF is conceptual `-inf`, so one-OFF/one-finite pairs do not fabricate a finite offset.

Known test note: finite-Hilbert low-frequency accuracy remains a Candidate issue and must be evaluated before Stable promotion.


---

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


---

## v0.1.9 — FIR Oversampling / PDC Alignment / Warning Cleanup

**日期：** 2026-08-27  
**状态：** Candidate / Test  
**基于：** v0.1.8 Plan B（用户上传 `QQ Super Compression 0.1.8-PlanB-20260825-161114.zip`）

### 用户需求

用户反馈当前版本已分享给朋友使用，整体反响不错，认为主要功能已接近完整形态。除 0.1.8 编译出现的非致命 `constrainer` 名称遮蔽 warning 外，用户要求加入最高 8x Oversampling，主要给 0 ms / 10 ms Lookahead 的主动染色模式使用：允许用户保留该声音特征，同时通过 2x/4x/8x 选择减少 alias fold-back。

### 问题表现 / 根因

- 0/10 ms 的 future-window gain modulation 本身会生成谐波；在 host sample rate 直接运行时，高次内容可能折返到 Nyquist 以下形成 aliasing。这里的“谐波生成”属于用户要保留的声音行为，而“折返 aliasing”才是 Oversampling 希望减轻的部分。
- 0.1.8 Editor 中局部变量 `constrainer` 与 JUCE/类上下文命名触发编译器 shadow warning，虽不影响运行但应清理。
- 加入 FIR Oversampling 后 Wet 会获得额外滤波 latency；若仍只按 Lookahead 延迟 Dry/Bypass，Mix 会产生时间偏移/梳状，因此 PDC 必须升级为 combined latency。

### 修改内容

- 新增 APVTS Choice 参数 `oversampling`，固定 `1x / 2x / 4x / 8x`，默认 1x；参数追加在原 0.1.8 参数序列之后，尽量不改变既有 host-facing 参数顺序。
- 预创建 4 套 `juce::dsp::Oversampling<float>`：1x dummy；2x/4x/8x = maximum-quality `filterHalfBandFIREquiripple` + `useIntegerLatency=true`。CMake 新增 `juce::juce_dsp`。
- Detector、future-window monotonic peak、Ratio smoother 和 Ratio gain application 在选定 oversampled domain 运行。Lookahead 继续先按 host sample rate 取原有 rounded sample 数，再乘 factor，保持六档实际时间/PDC 语义。
- 为避免破坏 0.1.6 起 Match 同时累积 ST/LR/MS 的行为，Oversampling 内部一次生成/下采样 6 路 pre-Makeup Wet：ST linked L/R、LR independent L/R、MS M/S。严格 LUFS accumulation 仍在 host rate 使用这六路。
- Makeup、Mix、最终 output Meter/Display 仍在 host sample rate。
- total latency = Lookahead base samples + JUCE FIR integer latency；独立 host-rate Dry delay 用相同 total latency，Active Mix Dry 与 Bypass 都走该路径。
- Oversampling 进入工程状态、A/B Snapshot、A→B/B→A 和 Undo/Redo；没有 per-user last selection，新实例默认 1x，0.1.8 或更早 state 缺参时也显式迁移到 1x。Editor Timer 对 Undo/Redo/host-side choice 同步时补一次 latency notification。
- `OVERSAMPLING` ComboBox 放在 Lookahead 下方，保持既有固定 1020x670 design root 和 uniform scaling。
- 局部 `constrainer` 改名 `editorBoundsConstrainer`；既有 `hostPlayHead` 命名继续保留。
- CMake project version 更新到 0.1.9，面板 `JucePlugin_VersionString` 因此自动显示低调 `v0.1.9`。

### 保持不变

- Ratio law 仍为 `gain = 1 / (1 + (Ratio - 1) * level)`。
- Lookahead 仍只有 0 / 10 / 26 / 40 / 80 / 100 ms；0 ms 染色语义不取消、不偷偷加隐藏 smoothing。
- 0.1.6 strict BS.1770 / EBU R128 Integrated LUFS Match 定义不改：Dry vs compressed Wet pre-Makeup/pre-Mix。
- ST/MS/LR、Makeup、Mix、2 s GR Hold、Dynamic Display、A/B 其它字段、UTF-8/CJK 和插件产品/internal identity 不改。
- 没有加入自动 Oversampling、没有按 Lookahead 强制某个 factor。

### 为什么这样修改

FIR linear-phase 路径优先服务于本插件已有 Dry/Wet Mix 的时间/相位可控性；integer latency 让 host PDC 与 base-rate Dry delay 能共享明确的整数 sample count。1x 使用 dummy stage，使默认状态尽量贴近 0.1.8，而不是强制所有用户承担 CPU/latency。保留六路 Wet 的代价是 8x CPU 更高，但避免为了性能而暗改 Match 的“所有域同时测量”语义。

### 验证

- [x] 源码静态数据流检查：OS factor → internal Lookahead → 6-Wet downsample → host-rate Match/Makeup/Mix → combined Dry/Bypass delay 已连接。
- [x] CMake version = 0.1.9，新增 `juce::juce_dsp` link；UI version 仍从 JUCE metadata 读取。
- [x] 源码检索：未重新引入局部 `playHead`；原 `constrainer` 局部变量已改为 `editorBoundsConstrainer`。
- [x] JUCE-free `tests/bs1770_match_selftest.cpp` 可继续作为 LUFS 回归基准（实际运行结果另见本版静态检查报告）。
- [ ] 当前 AI 环境完整 JUCE 8.0.15 / VST3 编译。
- [ ] Windows/Codex Release compile + warning 验证。
- [ ] Cubase PDC / Mix / Bypass / project/A-B/Undo 实测。
- [ ] PluginDoctor 0/10 ms 1x/2x/4x/8x aliasing 对比。
- [ ] 用户确认 0.1.9。

### 已知问题 / 边界

- 当前实现为了保持三个 Match domain 同时可用，2x/4x/8x 下会对 6 路 Wet 做 downsample，8x CPU 成本预计明显高于只处理当前模式；这是已知取舍，需要实际 profiling。
- 切换 Oversampling factor 会 reset FIR/detector/delay history，并改变 host latency；运行中可能出现一次 setting-change transient/PDC realignment。当前未加入用户未要求的双路径 crossfade。
- 当前预分配按至少 16384 host samples/block；正常 DAW block 远小于此值。异常大于 reserve 的 callback 会 fail-safe clear，而不是 realtime realloc；需实际宿主确认不会触发。
- 完整 JUCE API/编译尚未在当前环境验证，因此不能把静态检查写成“已编译”。

### 回滚

当前仍无用户指定 Stable。若 0.1.9 Oversampling/PDC 失败，优先回滚到用户上传的 **0.1.8 Plan B Candidate**，只重做 Oversampling / combined latency；不要回退 0.1.8 UI、0.1.7 Hold、0.1.6 strict LUFS Match 或 future-window peak 核心。

### 后续建议

Codex 首先完成 Windows Release 编译并看 warning；随后在 Cubase 测 combined latency/Mix/Bypass，再用 PluginDoctor 比 0/10 ms 的 1x→8x aliasing。只有用户明确确认后才能把版本提升为 Stable。


---

## v0.1.10 — 0 ms-Only 1x/8x/16x Oversampling / Product Rationale Handoff

**日期：** 2026-08-27  
**状态：** Candidate / Test  
**基于：** v0.1.9 Candidate

### 本次用户需求

用户在 v0.1.9 的 Oversampling 实验后完成了 PluginDoctor/宿主对比，并把最终产品逻辑收敛为：

- 仅 Lookahead = 0 ms 需要 Oversampling；
- 0 ms 显示一个单击循环按钮，choices = `1x / 8x / 16x`；
- 默认 remembered 0 ms choice = `8x`；
- 10/26/40/80/100 ms 内部固定 1x，Oversampling UI 隐藏；
- 切离 0 ms 不清除 choice，切回恢复。

用户额外要求源码仓库必须带足够的设计说明，因为之后会让 Codex 在 GitHub 上写插件介绍，而 Codex 无法看到本聊天。用户明确补充插件的真正设计起点：传统压缩的 Attack/Release 会改变吉他、人声、钢琴、贝斯等素材的音头/瞬态与听感；QQ Super Compression 是为了“需要压缩行为，但不想让 Attack/Release 同时重塑瞬态”的场景。

### 为什么没有 2x / 4x

这是本版必须永久保留的用户实测结论：

- PluginDoctor 实测 `2x` 仍存在严重 aliasing；
- `4x` 仍存在严重 aliasing；
- 同时用户观察到 8x Oversampling 增加的延迟并不多；
- 因而保留 2x/4x 的实际价值不高，直接给 `8x` 与 `16x` 更合理。

这不是“漏做了 2x/4x”。未来 AI/Codex 不得因为常见 Oversampling 菜单习惯而自动补回。

### 为什么 10 ms+ 不做 Oversampling

用户 PluginDoctor 实测 10 ms 及以上 Lookahead 基本没有需要处理的明显混叠。继续 Oversampling 只会增加 CPU/路径复杂度，而没有对应的实测问题，因此非零 Lookahead 统一 1x。

### 0 ms 的产品定义

0 ms 是故意保留的 instantaneous nonlinear colour/flavour。Oversampling 只用于减少 alias fold-back，不负责消灭它本身的谐波/染色：

- 1x = 最原始/最重的 0 ms colour；
- 8x = 默认 practical balance；
- 16x = 进一步降低 aliasing。

此前 PluginDoctor LinearAnalysis 在 Ratio>1 下看到高频上翘；Ratio=1:1 + 8x 的控制测试显示 FIR 本身基本平坦，仅接近 Nyquist 正常 roll-off。用户后续确认该问题无需处理，因此不要擅自加 compensating EQ。

### 具体代码修改

- `Parameters.h`：Oversampling factors 从 `{1,2,4,8}` 改为 `{1,8,16}`，stage counts `{0,3,4}`；增加 `effectiveOversamplingChoiceIndex()`，非零 Lookahead 永远返回 1x。
- `PluginProcessor`：预创建 1x dummy / 8x FIR / 16x FIR 三条 6-channel 路径；0 ms 才允许 8x/16x，10 ms+ 运行 1x。
- PDC：`getCombinedLatencySamples()` 使用 effective factor，因此 0 ms 8x/16x 报 FIR latency；10 ms+ 回到原 Lookahead-only latency。
- `PluginEditor`：Oversampling ComboBox 改为 TextButton，0 ms 显示并单击循环 `1x -> 8x -> 16x`；其它 Lookahead 隐藏 label/button。
- State：`oversampling` 参数继续保存 remembered 0 ms choice，默认 index=1(8x)。新增 state schema version 处理 v0.1.9 旧 choice 语义。
- Migration：v0.1.8 或更早缺少 OS -> 8x；v0.1.9 old 1x -> new 1x；old 2x/4x/8x -> new 8x。
- A/B：继续保存 OS choice；隐藏期间不清除。
- 版本号：CMake -> 0.1.10，面板继续自动显示 `v0.1.10`。
- 新增 `PRODUCT_DESIGN_NOTES.md` 和 `OVERSAMPLING_DESIGN_NOTES.md`；README/CODEX_BUILD/CHANGELOG/HANDOFF/TEST_CHECKLIST 同步更新。

### 保持不变

- Ratio law `gain = 1 / (1 + (Ratio - 1) * level)`；
- 0/10/26/40/80/100 ms 六档 Lookahead；
- strict Integrated LUFS Match；
- ST/MS/LR；
- Makeup / Mix；
- 2 s GR Hold；
- Dynamic Display / Meter；
- fixed-aspect uniform UI scaling；
- UTF-8/CJK 与插件 internal identity。

### 当前验证级别

- 用户对 v0.1.9 Oversampling 的 PluginDoctor/宿主测试：已实际发生，作为 v0.1.10 设计依据。
- v0.1.10 源码：已做静态数据流/状态/UI 检查。
- JUCE-free BS.1770 self-test：继续作为回归测试。
- 完整 JUCE/VST3 编译、16x CPU、Cubase PDC/Mix/Bypass/state migration：仍需 Codex/用户实测。

### 回滚

若 v0.1.10 失败，优先回到 v0.1.9 Candidate，只重做 Oversampling finalisation；若 v0.1.9 Oversampling 架构本身有根本问题，则回到用户上传的 v0.1.8 Plan B Candidate，仅重新实现 0 ms-only OS。不得回退 future-window peak、strict LUFS Match、GR Hold 或 uniform UI。

---

## v0.9.0 — Warm Transparent UI Candidate

**日期：** 2026-08-27  
**状态：** Candidate / Test  
**基于：** v0.1.10 — 0 ms-Only 1x/8x/16x Oversampling / Design Documentation

### 用户需求

当前 DSP/产品概念已经获得用户本人和多位网友积极实际反馈。进入正式发布前，先优化单段 QQ Super Compression 的 UI；用户不希望 Display 变成 FabFilter / Cenozoix 类设计，只接受背景/颜色层面的优化。

用户拒绝最初的深黑赛博朋克方案，因为过暗的视觉容易让人联想到重度失真/饱和，与本产品“干净、透亮、尽量保留瞬态”的定位冲突。用户最终选择浅暖、简洁、透明、带柔和内发光的方向，并明确选择 Mode 方案 A：一个按钮单击循环三个模式，而不是下拉菜单。

### 问题表现

0.1.10 功能/UI 工作流可用，但整体仍是黑色开发者原型视觉。视觉语言没有充分表达插件的透明动态控制定位，也缺少接近正式产品的统一层级与材质感。

### 根因

不是 DSP 问题。属于产品视觉身份尚未完成：旧暗色主题来自开发阶段，而不是经用户确认的最终品牌方向。

### 修改内容

- CMake/JUCE 版本更新为 `0.9.0`；面板版本号继续自动读取 metadata。
- 扩展 `UTF8LookAndFeel.h`：新增暖白/沙色 palette、可缩放旋钮、细轨道、柔和多层 glow、按钮状态 glow、浅色 ComboBox。
- Ratio / Makeup / Mix 使用暖橙主 accent；Oversampling / technical accent 使用 cyan；A/B active / Mode / Bypass 根据状态采用柔和灯光语言。
- Editor 主背景由黑色改为暖 ivory/sand；加入轻薄 rounded chassis panels、细 border 与非常轻的投影。
- Dynamic Display 结构和数据完全不改，只改为浅暖背景；Dry=warm neutral grey、Wet=cyan、Output=coral/orange。
- LevelMeters 改为浅暖面板；Input / Output / Gain Reduction 分别使用 neutral / output-orange / coral 色，同时保留原 Meter 范围、方向和 2 s Hold。
- Mode 继续/明确使用单按钮 click-cycle，当前文字直接显示 ST/MS/LR，不使用下拉菜单。
- 新增 `UI_DESIGN_NOTES.md`，记录用户拒绝暗色方案、选择暖透明方向、Glow 实现原则、Mode 交互和不得改动的 DSP 边界。

### 保持不变

- Ratio law 不变。
- Future-window detector / 六档 Lookahead 不变。
- 0 ms 1x/8x/16x Oversampling、默认 8x、10 ms+ fixed 1x 不变。
- PDC、Dry/Wet/Bypass 对齐不变。
- Strict Integrated LUFS Match 不变。
- A/B、Undo/Redo、Makeup、Mix 不变。
- ST/MS/LR DSP 定义和 parameter IDs 不变。
- 2 s GR Peak Hold 不变。
- Dynamic Display 数据语义不变。

### 为什么这样修改

UI 必须与声音定位一致。用户希望看到的是“干净、透亮、温暖、未来感已经变成现实”的工具，而不是暗黑/重失真视觉。Glow 使用多层低透明矢量 stroke/fill，而不是大面积 neon blur，既符合柔和内发光效果，也保持 resize 清晰和可维护性。

Mode 只有三个已固定状态，下拉菜单增加了不必要的交互层；单按钮循环更快、更简洁，也保留已有处理模式参数语义。

### 验证

- [x] 源码静态检查（当前 AI 环境）
- [x] DSP 源文件 hash 对照计划：确认 UI pass 不改 Processor / Ratio engine / loudness match 后再交付
- [ ] JUCE / VST3 完整编译（需 Codex）
- [ ] Windows / Cubase 实测
- [ ] 用户 UI 视觉确认
- [ ] 用户最终 v1.0.0 发布确认

### 已知问题

- Glow / knob/body 的最终明暗、色温和间距仍需要实际 VST3 截图由用户审美确认。
- 不得因为 v0.9.0 是“接近发布”就擅自标记 Stable 或 Release。

### 回滚

如果 v0.9.0 UI 实现有问题，回滚到 `v0.1.10` 的 UI 文件即可；DSP 仍以 0.1.10 核心为准，不要回滚 Ratio / Lookahead / LUFS Match / Oversampling 等已验证设计。

### 后续建议

- Codex 编译后先检查 light-theme 控件边界、字体、PopupMenu、resize、disabled Match、A/B active state、Bypass on-state 和 0 ms Oversampling visibility。
- 用户确认视觉后再做小范围 spacing / glow / colour tuning。
- 最终确认后才升级为 v1.0.0 Release。


---

## v0.9.1 — Lighting & Material Refinement

**日期：** 2026-08-27  
**状态：** Candidate / Test  
**基于：** v0.9.0 — Warm Transparent UI Candidate

### 用户需求

用户实际编译 v0.9.0 后，对当前布局已经比较满意，但明确指出此前承诺的“亮灯/光影”没有在真实界面里体现出来。实机截图中 Ratio / Makeup / Mix 更像橙色描边，而不是概念图中的柔和内发光。用户确认继续修改，但不重做布局。

### 问题表现

0.9.0 虽然已经有多层低透明矢量 stroke/fill，但在浅 ivory 背景下能量过低：外圈 glow、底部 spill 和按钮 glow 在真实 DAW 缩放后几乎被背景吃掉，因此视觉上只剩清晰的橙色弧线。

### 根因

这是 UI 光影参数/材质层级不足，不是布局问题，也不是 DSP 问题。0.9.0 的 glow 思路本身正确，但透明度、扩散宽度、底部反射面积以及旋钮实体材质之间的对比不足。

### 修改内容

- `UTF8LookAndFeel.h`：旋钮 value arc 改为更宽的四层 halo/bloom + crisp arc；增加更大的底部暖光池、endpoint hot core、旋钮内部下半区暖色反射、顶部白色 rim 与下缘 shade。
- 激活按钮：增加三层外 halo 与底部 light spill；保持原有 toggle/always-lit 逻辑和 colour roles。
- `PluginEditor::paint()`：不改 panel geometry，只增加极轻的 ivory vertical gradient、顶部高光 rim、柔和 shadow/border，让发光有可见的材质承载面。
- 版本号 -> 0.9.1。
- 用户报告的真实 MSVC C4459：把 `lookaheadMs` 冲突 argument/local 仅做命名替换，使用 `requestedLookaheadMs` / `currentLookaheadMs` / `snapshotLookaheadMs`；算法不变。

### 保持不变

- 0.9.0 的整体布局、Display / Meter 位置和尺寸不变。
- Mode 仍为单按钮循环 `ST -> MS -> LR -> ST`。
- Ratio / Lookahead / 0 ms 1x-8x-16x Oversampling / PDC / LUFS Match / A-B / Makeup / Mix / GR Hold 均不改。
- Dynamic Display 数据和三条 trace 语义不改。
- 不加入 animation、bitmap glow 或暗色背景。

### 为什么这样改

用户已经确认“布局对了，灯光没出来”，因此最小风险做法是只提高已经批准的灯光语言的可感知度，而不是再次推翻界面。亮背景下需要比暗背景更大的扩散面积和更清晰的局部光源/反射，才能让 glow 被看成光而不是一条色线。

### 验证

- [x] 源码 diff/static 检查：布局坐标没有修改。
- [x] C4459 冲突命名已从 Parameters namespace helper 中移除，并清理两处同名局部变量。
- [ ] JUCE/VST3 完整编译（需 Codex/用户）。
- [ ] Cubase 实际截图确认 glow 强度。
- [ ] 用户确认。

### 已知问题

最终 glow 强度仍属于视觉审美，需要真实 VST3 截图确认。若仍偏弱/偏强，应优先只调 `UTF8LookAndFeel.h` 中 alpha / stroke width / spill size，不修改布局和 DSP。

### 回滚

若 0.9.1 光影效果失败，可直接回到 v0.9.0 UI 文件；DSP 基线不回退。

---

## v0.9.2 — Asset Knobs / Input & Output Gain

**日期：** 2026-08-27  
**状态：** Candidate  
**基于：** v0.9.1

### 用户需求

1. 0.9.0/0.9.1 通过 JUCE 绘制 glow 虽然功能正确，但实机视觉达不到用户要求。用户明确要求改为真正的 UI 图片资产：旋钮使用 128 帧透明 PNG，方便视觉状态与 7-bit MIDI CC 0–127 对应。
2. 图片资产不能包含参数数字/小数点/单位，因为 Ratio、Makeup、Mix 等参数需要直接文本输入；数字必须继续用字体实时显示。
3. 旋钮灯光不是移动单点：应从最小值左侧开始，随数值累计点亮；20% 约亮前 20%，100% 亮完整有效弧；0/127 两端指针都必须可见。
4. 之前已确认需要加入 Input Gain / Output Gain。Input Gain 不改变 Dynamic Display 的 Dry/Input 参考；Output Gain 必须进入最终 Output Display。

### 为什么属于架构变化

旧 UI 的旋钮材质、弧线、Glow、指针全部由 `drawRotarySlider()` 动态矢量绘制。v0.9.2 改为：Slider/APVTS 仍负责参数与交互，LookAndFeel 只按 normalized value 选择 PNG filmstrip 中的一帧。这将视觉资产与参数逻辑分离，未来可换皮而不重写 DSP/参数。

### 修改内容

- CMake 增加 `QQSCAssets` BinaryData target，把 runtime PNG 嵌入 VST3。
- 新增 128-frame / 256px vertical transparent filmstrip。
- `UTF8LookAndFeel::drawRotarySlider()` 改为 `round(normalized*127)` 选择 frame；保留 asset-missing fallback，仅用于防止空白控件。
- 数值 TextBox 保持 JUCE editable text，不进入 PNG。
- 新增 `inputGainDb` / `outputGainDb` APVTS 参数；为避免旧工程风险，新参数追加在完整旧参数序列之后，不插入旧参数中间。
- Input Gain 在 host-rate 先平滑，再进入 Oversampling / detector / compression。
- 新增 untouched host-rate input copy + parallel original Dry delay：Dynamic Display Dry/Input 与 true bypass 使用 pre-Input-Gain 信号；活动 Dry/Mix/Match 使用 Input-Gain 后信号。
- Output Gain 位于 Makeup + Mix 后；Dynamic Display Output 与 Output meter 使用 Output-Gain 后最终信号。
- Match 仍比较处理链内部的 Dry（Input Gain 后）与 Wet pre-Makeup，不让 Output Gain 污染 Match。
- Input/Output Gain 加入 A/B snapshot、state、Undo/Redo；旧 state 缺失时明确恢复 0 dB。
- UI 主布局保留 Display/Meter 不动，底部增加左右两个较小 Trim knob；Ratio/Makeup/Mix 仍是主旋钮；Mode/Lookahead 区保留。

### 保持不变

- Ratio law 不改。
- Future-window Peak / Lookahead presets 不改。
- 0 ms 1x/8x/16x Oversampling 语义不改。
- LUFS Match 算法不改，Match 仍只写 Makeup。
- 2 s GR Hold 不改。
- PDC 总延迟逻辑不改。
- Mode 仍单按钮 ST -> MS -> LR -> ST。
- Dynamic Display / Meter 本身仍实时绘制，不图片化。

### 验证状态

- 已做源码静态检查、资产尺寸/128 frame 检查、PNG alpha 检查、状态/参数引用检查。
- 当前 AI 环境没有 JUCE checkout，未完成真实 VST3 编译。
- 必须由 Codex/用户实测：BinaryData 编译、图片缩放/HiDPI、文本输入、A/B/Undo、Input Gain detector 行为、Display 不跟随 Input Dry、Output Gain 跟随 Output Display、Bypass/PDC。

### 回滚

若 bitmap knob 架构或新增 trims 出现问题，回滚到 v0.9.1 Candidate；不要回退既有 Ratio/Lookahead/Oversampling/LUFS Match/GR Hold 核心。


---

## v0.9.3 — UI Rollback / Features Retained

**日期：** 2026-08-27  
**状态：** Candidate / Test  
**基于：** v0.9.2 — Asset Knobs / Input & Output Gain

### 用户需求

用户明确放弃继续追求图片资产旋钮的“完美 UI”，要求退回上一版 v0.9.1 的 UI 设计/绘制风格，但保留 v0.9.2 已加入的功能。

### 问题表现

用户在真实插件中测试 v0.9.2 后，认为 128 帧 bitmap knob 的实际视觉结果很差；后续多轮 AI 资产生成也无法稳定复现概念图、灯光与刻度规则。继续围绕 bitmap 资产修补会增加 UI 架构复杂度，却没有形成可靠收益。

### 根因

功能层与 UI 层本可独立，但 v0.9.2 把旋钮视觉切换为 BinaryData filmstrip，使美术资产质量直接决定运行时观感。用户最终决定停止这条视觉路线。因此本次根因不是 DSP，也不是 Input/Output Gain 功能错误，而是 **bitmap rotary UI 方向被用户实际审美验证否决**。

### 修改内容

- 以 v0.9.2 为功能基线，不回滚 Processor / Parameters / state schema。
- `UTF8LookAndFeel.h` 恢复为 v0.9.1 的 JUCE/vector rotary drawing。
- CMake 删除活动 `QQSCAssets` BinaryData target/link；PNG 及 `UI_ASSET_ARCHITECTURE.md` 保留为失败实验历史，不再参与运行时。
- 底部仍保留 v0.9.2 的 Input Gain / Ratio / Makeup / Mix / Output Gain / Mode-LOOKAHEAD 布局；Input/Output 继续是较小次级 Trim。
- 版本号更新为 v0.9.3。

### 保持不变

- Input Gain 仍位于 detector/compression 前并改变压缩行为与 Input meters。
- Input Gain 仍 **不移动 Dynamic Display Dry/Input 原始参考**。
- Output Gain 仍位于 Makeup/Mix 后，影响最终音频、Output meters 与 Dynamic Display Output。
- LUFS Match 仍只写 Makeup；A/B、项目状态、Undo/Redo 仍包含两只新 Gain。
- Ratio law、Future-window Lookahead、0 ms 1x/8x/16x Oversampling、PDC/Bypass、ST/MS/LR、2 秒 GR Hold 均不改。

### 为什么这样修改

用户要的是“退回上一版 UI，但保留当前功能”，因此不能直接用 v0.9.1 整包覆盖 v0.9.2，否则会丢失 Input/Output Gain 及其完整信号/state 逻辑。最小风险方案是仅回退视觉实现层与 build asset dependency，同时保持 v0.9.2 功能代码不变。

### 验证

- [x] 源码静态 diff：Processor / Parameters / Meter / Display 等功能/DSP 文件保持 v0.9.2。
- [x] `UTF8LookAndFeel.h` 与 v0.9.1 恢复源逐字节一致。
- [x] CMake 已无 `QQSCAssets` target/link。
- [ ] JUCE/VST3 真实编译（需 Codex）。
- [ ] Cubase 实际 UI/功能验证。
- [ ] 用户最终确认。

### 已知问题

- v0.9.3 保留了 v0.9.1 的 code-drawn glow/material，因此也保留其视觉局限；这是用户主动接受的回退方向，不再继续 bitmap asset 追求。
- 当前仍无用户明确指定 Stable。

### 回滚

若 v0.9.3 UI 恢复发生编译问题，可对照 v0.9.1 的 `UTF8LookAndFeel.h` / CMake；**不要回滚 v0.9.2 的 Input/Output Gain DSP/state 功能**。



---

## v0.9.4 — Editable Numeric Text Contrast

**日期：** 2026-08-28  
**状态：** Candidate / Test  
**基于：** v0.9.3

### 用户反馈

当前界面和功能已基本达到预期，但双击旋钮数值进行直接输入时，JUCE 编辑状态中的数字为白色，在浅色背景上不可读。

### 根因与修复

普通 Slider TextBox 颜色已经设置正确，但编辑时 JUCE 切换到 Label/TextEditor 编辑态，旧 LookAndFeel 没有覆盖这些编辑态 ColourIds。v0.9.4 只补齐 `Label::...WhenEditingColourId`、`TextEditor` text/background/highlight 和 caret 配色。

### 非回归边界

不改 DSP、参数、布局、Input/Output Gain、Display/Meter、A/B/state、Lookahead/Oversampling/PDC、LUFS Match 或 GR Hold。

### 验证级别

当前仅静态检查；需要 Codex 实际编译和 Cubase 双击输入确认。

---

## v0.9.7 — Threshold Rebuild

**日期：** 2026-08-30  
**状态：** Candidate / Test  
**基于：** v0.9.4

### 用户需求

从 v0.9.4 可靠基线重做 Threshold。用户明确指出 Threshold 只是把原始 `-inf` 作用下限改成具体数值，不应该借此重构 QQ Super Compression 的 future-window 核心。v0.9.5 / v0.9.6 均被用户判定有问题。

另外扩大 Display/Meter 上半区，压缩下方控件高度，并补齐 Threshold 的 Shift 微调。

### 根因

前两版把“新增作用下限”和“重新设计 Detector”混在了一起。尤其 v0.9.6 使 Threshold OFF 也改变了 Detector 行为，破坏了旧版声音基线。另一个 UI 缺陷是 Threshold 使用 LinearVertical Slider，而既有 FineKnob 只通过 rotary mouse sensitivity 实现 Shift，因此 Threshold 实际没有获得 Shift fine。

### 修改

- 直接从 v0.9.4 重开分支。
- 原 Detector/queue 不变；只在最终 gain law 增加 Threshold boundary。
- OFF 精确走旧公式。
- Threshold 0.01 dB，支持 LinearVertical Shift 8x fine drag、Alt reset、Undo/Redo、双击输入。
- A/B 和 state 增加 Threshold；旧状态默认 OFF。
- Dynamic Display 增加 effective Threshold 虚线。
- visual row 350 -> 405；controls 166 -> 145，并缩小下方控件。

### 验证

- Threshold OFF 200000 随机数学回归：PASS。
- Threshold 边界连续性自测：PASS。
- 当前仅静态/参考测试，尚未在 Windows/Cubase/PluginDoctor 实测。

### 回滚

失败时回滚 v0.9.4；v0.9.5 / v0.9.6 不作为回滚点。


---

## v1.0.1 Candidate Revision 4 — Display 0…-90 dB Working Scale

**日期：** 2026-08-30  
**状态：** Candidate / Test  
**基于：** v1.0.1 Mode / Lookahead Alignment Polish

### 用户需求

用户确认放大的 Display 布局方向正确，但指出 `-90 dB` 以下的显示区域几乎没有用于寻找 Threshold 的实际价值，导致主 Display 视觉上过于空旷。

### 修改

- Dynamic Display 可视纵轴改为固定 `0…-90 dB`。
- 刻度改为 `0 / -15 / -30 / -45 / -60 / -75 / -90 dB`。
- 小于 -90 dB 或大于 0 dB 的历史值只在绘图坐标阶段钳制到边界。
- 不删除底层数据，也不改变 Threshold 参数范围、Meter、DSP、LUFS、Lookahead、Ratio、Mix、Makeup、LR/MS 或 LINK 行为。

### 为什么这样改

Threshold 已经成为主要工作功能之一。固定 `0…-90 dB` 可以让常用动态范围占据更多像素高度，并保持跨素材一致的视觉参考；不采用自动缩放，避免同一 Threshold 在不同素材上视觉位置漂移。

### 验证

- [x] 源码静态检查：Display 常量为 `0 / -90 dB`。
- [x] 刻度静态检查：`0/-15/-30/-45/-60/-75/-90`。
- [x] 既有 Python DSP/Threshold/LINK/Mix self-test。
- [ ] JUCE/MSVC VST3 编译。
- [ ] Cubase 实际 UI 检查。
- [ ] 用户确认。

### 回滚

若视觉结果不合适，只回滚 `DynamicDisplay.cpp` 的可视范围与刻度；不要回滚 v1.0.1 已确认的 Transparent Core、Threshold、LR/MS、LINK、独立 Mix 和 Mode/Lookahead 对齐。

---

## v1.0.3 — Centered Domain Monitor

**日期：** 2026-08-30  
**状态：** Candidate / Test  
**基于：** v1.0.2 — Complete Relative LINK **Stable**（用户于 2026-08-30 明确设为稳定基线）

### 用户需求与设计澄清

用户发现 LR / MS 独立处理已经完整，但缺少单独监听各域的功能。用户进一步明确：QQ Super Compression 与 QQ ChainScope 的用途不同。ChainScope 为工作室音箱监听设计，因此有 SIP / 原位扬声器监听等特殊能力；普通插件更常见的用途是耳机参考，所以 QQ Super Compression 的 L/R/M/S 单独监听都应居中。

用户特别指出居中复制会产生音量增加，要求参考 QQ ChainScope Mixboard 已成熟的居中方案。讨论中用户明确纠正：M 不能机械套用 `0.7071`。最终批准的定义为：

- L Monitor: `L * 0.70710678` 同极性复制到左右，居中。
- R Monitor: `R * 0.70710678` 同极性复制到左右，居中。
- M Monitor: `M=(L+R)/2` 直接同极性复制到左右，**不额外 -3.01 dB**。
- S Monitor: 按 ChainScope Mixboard 成熟规则，`S=(L-R)/2` 后乘 `0.70710678`，同极性复制到左右，居中。
- ALL 保持正常 stereo。

### 实现

- 新增非 APVTS 的 LR/MS Monitor workflow state；LR 与 MS 分别保存 ALL/First/Second，切换模式互不覆盖。
- Monitor 只作用于写入 DAW 的最终 audible output。正常 `outL/outR` 仍用于 Display、Meter、Match，避免 -3.0103 dB 试听补偿污染分析。
- L/R/S 使用 `1/sqrt(2)=0.707106781...`；M 保持 unity。
- True Bypass 不经过 Monitor，维持“真正 bypass”语义。
- 工程状态 schema 7 -> 8，新增 `qqscMonitorLRSelection` / `qqscMonitorMSSelection`；旧工程默认 ALL。
- Monitor 不进入 A/B，也不进入 DAW automation 参数列表。
- UI 在 Mode 与 Lookahead 之间加入 `MONITOR` 三按钮行。LR=ALL/L/R，MS=ALL/M/S，ST 隐藏。
- 三按钮占用与 primary choice 相同的 108 px 宽度：34+3+34+3+34。Mode/Lookahead 仍为 108x23，LINK 仍为 34x23。
- 下方 control row 从 140 增至 158 px，只使用原本空着的底部余量；550 px Display/Meter row 完全不缩小。

### 为什么这样做

居中监听是 audition/reference 层，不应重写压缩结果。L/R/Side 从一个单独分量复制到双耳时使用等功率 -3.0103 dB 可避免明显监听增益；Mid 在本插件 `M=(L+R)/2` 定义下本身就是中心分量，再减 3 dB 会错误地让中心内容变小。

### 保持不变

- Future-window Peak / Lookahead 核心。
- Ratio/Threshold law、Threshold OFF。
- LR/MS 独立 Ratio/Threshold/Makeup/Mix。
- v1.0.2 Complete Relative LINK，包括 direct numeric entry。
- Match、A/B sound snapshots、Output Gain 参数本体、Display/Meter 数据语义。
- 0 ms Oversampling / PDC / Dynamic Display 0…-90 dB。

### 验证

- [x] `transparent_core_selftest.py` PASS。
- [x] `threshold_rebuild_selftest.py` PASS。
- [x] `domain_link_selftest.py` PASS。
- [x] `link_ui_source_selftest.py` PASS。
- [x] `independent_mix_selftest.py` PASS。
- [x] `display_scale_selftest.py` PASS。
- [x] 新增 `monitor_audition_selftest.py`：L/R/M/S 数学、source wiring、analysis isolation、state/UI 检查 PASS。
- [ ] JUCE/VST3 编译：当前容器尝试 CMake configure，但 github.com DNS 无法解析，JUCE 8.0.15 无法 Fetch，因此未进入 C++ compile 阶段。
- [ ] Cubase 实际监听/界面/工程恢复验证。
- [ ] 用户确认并决定是否提升 Stable。

### 回滚

若 v1.0.3 Monitor 有问题，完整回滚到 **v1.0.2 Complete Relative LINK Stable**。不要回退或改写 v1.0.2 已确认的 LINK、Threshold、独立 Mix、Transparent Core 或 Display。

