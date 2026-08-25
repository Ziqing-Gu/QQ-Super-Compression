# QQ Super Compression 版本记录 / Changelog

本文档记录所有真实版本 0.1.0–0.1.8。每条记录均提供语义一致的中文和英文摘要。所有版本当前均为 Candidate/Test；没有任何版本已由用户确认 Stable。

This document records every real version from 0.1.0 through 0.1.8. Every entry includes semantically equivalent Chinese and English summaries. All versions currently remain Candidate/Test; no version has been user-confirmed Stable.

## 版本覆盖审计 / Version coverage audit

- 上一个完整公开中英双语记录 / Previous complete public bilingual record: 无，属于首次纠正 / none; this is the initial correction.
- 本次目标 / Target: 0.1.8.
- 实际连续版本 / Actual continuous versions: 0.1.0–0.1.8.
- README 覆盖 / README coverage: 0.1.0–0.1.8.
- CHANGELOG 覆盖 / CHANGELOG coverage: 0.1.0–0.1.8.
- 遗漏 / Omissions: 无 / none.

## 0.1.8 — GR Hold 可读性、统一缩放与发布文档 / GR Hold Readability, Uniform Scaling, and Release Documentation

- 日期 / Date: 2026-08-25（发布文档纠正 / release-documentation correction: 2026-08-26）
- 状态 / Status: Candidate / Test

中文：

- 移除 GR Hold 数字前多余的 `H`，数值字体从 7.5 提高到 8.5。
- 以 1020x670 为内容根尺寸对整个 UI 统一缩放，窗口限制为原始宽高比。
- 旧的非等比保存尺寸迁移为能放入旧矩形的最大等比尺寸。
- Ratio、未来峰值检测、Lookahead/PDC、严格 LUFS Match、A/B、参数 ID 和状态结构均不变。
- 2026-08-26 补齐 MIT 许可证、公开 GitHub Actions、四平台 Plan D 路线、完整中英双语 README/CHANGELOG/构建说明/安装说明；本次文档纠正不改变 DSP。

English:

- Removed the redundant `H` before the GR Hold value and increased its font from 7.5 to 8.5.
- Uniformly scaled the complete UI around a 1020x670 content root and constrained the window to the original aspect ratio.
- Migrated old non-proportional saved sizes to the largest proportional size fitting the previous rectangle.
- Ratio, future-peak detection, Lookahead/PDC, strict LUFS Match, A/B, parameter IDs, and state schema are unchanged.
- On 2026-08-26, added the MIT license, public GitHub Actions, four-platform Plan D route, and complete bilingual README, changelog, build guide, and installation guides. This documentation correction does not change DSP.

验证 / Validation:

- Windows Release build, BS.1770 self-test, DLL entry-point smoke test, VST3 bundle validation, macOS arm64/x86_64 builds, Universal 2 AU build, and CI `auval` have passed in the Plan D preparation cycle.
- Cubase GUI, listening, and final user acceptance remain pending; therefore status remains Candidate/Test.

## 0.1.7 — 自动 GR Peak Hold、版本标签与警告清理 / Automatic GR Peak Hold, Version Tag, and Warning Cleanup

- 日期 / Date: 2026-08-25
- 状态 / Status: Candidate / Test

中文：

- 为每个活动 GR 表通道/分量加入 2 秒自动峰值保持。
- 更深峰值立即移动标记并重启计时；2 秒内没有更深峰值时，标记自动刷新到当前 GR。
- ST 联动，LR 的 L/R 独立，MS 的 M/S 独立；切换 Mode 或 Lookahead 会清除旧值。
- Hold 仅用于显示，不影响检测增益、音频、Makeup、Mix、Match、A/B 或项目参数。
- 将 `playHead` 重命名为 `hostPlayHead`，消除名称遮蔽警告；加入小型版本标签。

English:

- Added a 2-second automatic peak hold to every active GR meter channel/component.
- A deeper peak moves the marker immediately and restarts the timer; after two seconds without a deeper peak, the marker refreshes to current GR.
- ST is linked, LR holds L/R independently, and MS holds M/S independently. Changing Mode or Lookahead clears stale values.
- Hold is display-only and does not affect detector gain, audio, Makeup, Mix, Match, A/B, or project parameters.
- Renamed `playHead` to `hostPlayHead` to remove a shadow warning and added a small version label.

## 0.1.6 — 严格 Integrated LUFS Match / Strict Integrated LUFS Match

- 日期 / Date: 2026-08-25
- 状态 / Status: Candidate / Test

中文：

- 用 BS.1770 / EBU R128 Integrated Loudness 取代旧的 RMS/能量 Match。
- 每路使用两级 K-weighting、400 ms 门限块、75% overlap（100 ms hop）、-70 LUFS 绝对门限和 -10 LU 相对门限。
- Match 比较延迟 Dry 与压缩后、Makeup 前、Mix 前的 Wet；ST 写入共同 Makeup，LR 分别写 L/R，MS 分别写 M/S。
- 无有效门限结果的静音通道/分量保持原 Makeup。
- 未来窗口峰值压缩核心和 Lookahead 预设不变；加入 JUCE-free BS.1770 自测。

English:

- Replaced the old RMS/energy Match with BS.1770 / EBU R128 Integrated Loudness.
- Each measured stream uses two-stage K-weighting, 400 ms gating blocks, 75% overlap (100 ms hop), a -70 LUFS absolute gate, and a -10 LU relative gate.
- Match compares delayed Dry with compressed Wet before Makeup and Mix. ST writes common Makeup, LR writes L/R separately, and MS writes M/S separately.
- A silent channel/component with no valid gated result keeps its previous Makeup.
- The future-window peak core and Lookahead presets are unchanged. A JUCE-free BS.1770 self-test was added.

## 0.1.5 — 固定 Lookahead 预设与最后选择记忆 / Fixed Lookahead Presets and Last-Choice Memory

- 日期 / Date: 2026-08-25
- 状态 / Status: Candidate / Test

中文：

- 将 0.1.4 的 0–100 ms 任意文本输入改为 `0/10/26/40/80/100 ms` 六档。
- 保留 `lookaheadMs` 参数 ID；恢复旧值时迁移到最近预设，等距时选更长值。
- 现有实例恢复自己的项目值；新实例读取用户最后手动选择；没有记录时默认 26 ms。
- Bypass 和 PDC 继续严格跟随所选 Lookahead；0 ms 继续作为有意风格保留。
- Match 此时仍为 RMS 原型，严格 LUFS 尚未加入。

English:

- Replaced the 0.1.4 arbitrary 0–100 ms text entry with six `0/10/26/40/80/100 ms` presets.
- Retained the `lookaheadMs` parameter ID. Legacy values migrate to the nearest preset, choosing the longer value on an exact tie.
- Existing instances restore their project value; new instances read the user's last manual choice; 26 ms is used when no preference exists.
- Bypass and PDC continue to follow the selected Lookahead exactly, and 0 ms remains an intentional flavour.
- Match was still the RMS prototype; strict LUFS had not yet been added.

## 0.1.4 — 可变 Lookahead 未来峰值实验 / Variable-Lookahead Future-Peak Experiment

- 日期 / Date: 2026-08-25
- 状态 / Status: Candidate / Test

中文：

- PluginDoctor 检查否定了旧的 20 ms rolling-RMS 检测器。
- 加入可编辑 0–100 ms Lookahead，初始默认 5 ms。
- Lookahead 同时控制未来分析窗口、实际音频延迟和宿主 PDC；Bypass 使用相同延迟 Dry 路径。
- 为 L/R/M/S 加入无分配滑动未来峰值分析器。
- 0 ms 有意保持容易产生失真的风格；Ratio 公式和 RMS Match 原型保持不变。

English:

- PluginDoctor testing rejected the old 20 ms rolling-RMS detector.
- Added editable 0–100 ms Lookahead with an initial 5 ms default.
- Lookahead controls the future analysis window, real audio delay, and host PDC. Bypass uses the same delayed Dry path.
- Added allocation-free sliding future-peak analysers for L/R/M/S.
- The distortion-prone 0 ms flavour is intentionally retained. The Ratio law and RMS Match prototype were unchanged.

## 0.1.3 — 工作流、A/B、Match 与独立 Makeup / Workflow, A/B, Match, and Independent Makeup

- 日期 / Date: 2026-08-25
- 状态 / Status: Candidate / Test

中文：

- 移除副标题、缩窄电平表面板、加入编辑器尺寸记忆。
- 加入 A/B、A→B/B→A 复制、Undo/Redo、Shift 精调和 Alt 复位。
- Bypass 改为普通按钮，Mode 改为循环按钮。
- ST 使用共享 Makeup；LR 使用独立 L/R Makeup；MS 使用独立 M/S Makeup。
- 加入能量式 Match 原型；该版还不是严格 LUFS。
- 保留 20 ms 检测逻辑、0 样本对外延迟和动态显示。

English:

- Removed the subtitle, narrowed the meter panel, and added editor-size memory.
- Added A/B, A→B/B→A copy, Undo/Redo, Shift fine adjustment, and Alt reset.
- Changed Bypass to a normal button and Mode to a cycle button.
- ST uses shared Makeup, LR uses independent L/R Makeup, and MS uses independent M/S Makeup.
- Added an energy-based Match prototype; this version was not yet strict LUFS.
- Retained the 20 ms detector logic, zero externally reported latency, and Dynamic Display.

## 0.1.2 — ST/MS/LR、零延迟与双通道表 / ST/MS/LR, Zero Latency, and Dual-Channel Meters

- 日期 / Date: 2026-08-25
- 状态 / Status: Candidate / Test

中文：

- 移除固定 10 ms 稳定延迟，报告 0 样本延迟。
- 移除 Makeup Gate。
- 加入 ST/MS/LR 模式和三组双通道表；MS 显示 M/S，GR 从顶部 0 dB 向下增长。
- ST 联动，LR 独立，MS 独立。
- 保留 0.1.1 的无阈值电平域 Ratio。

English:

- Removed the fixed 10 ms stable latency and reported zero samples of latency.
- Removed Makeup Gate.
- Added ST/MS/LR modes and three dual-channel meter groups. MS displays M/S, and GR grows downward from 0 dB at the top.
- ST is linked, LR is independent, and MS is independent.
- Retained the 0.1.1 threshold-free level-domain Ratio.

## 0.1.1 — Ratio 引擎修复与电平表/UI / Ratio Engine Fix and Meter/UI Pass

- 日期 / Date: 2026-08-25
- 状态 / Status: Candidate / Test

中文：

- 淘汰 0.1.0 的样本域 waveshaper；该实现会随 Ratio 抬高电平并产生严重失真。
- 改为电平域无阈值增益控制，使 Ratio 增大时获得更多 Gain Reduction。
- 加入 Input、Output、Gain Reduction 三组电平表。
- 加入 Dynamic Display 布局以及 UTF-8/CJK 字体回退。

English:

- Rejected the 0.1.0 sample-domain waveshaper, which increased level with Ratio and produced severe distortion.
- Replaced it with threshold-free level-domain gain control so increasing Ratio creates more Gain Reduction.
- Added Input, Output, and Gain Reduction meter groups.
- Added the Dynamic Display layout and UTF-8/CJK font fallback.

## 0.1.0 — 原型 / Prototype

- 日期 / Date: 2026-08-25
- 状态 / Status: Candidate / Test

中文：

- 首个包含 Ratio、Makeup、Makeup Gate、Mix 和 0/10 ms 选项的原型。
- Ratio 使用后来被否定的样本域 waveshaper；增大 Ratio 会抬高电平并引发明显失真。
- 该版仅用于验证产品方向，不应作为 Stable 基线。

English:

- Initial prototype with Ratio, Makeup, Makeup Gate, Mix, and 0/10 ms options.
- Ratio used the later-rejected sample-domain waveshaper; increasing Ratio raised level and caused obvious distortion.
- This version only validated the product direction and must not be treated as a Stable baseline.
