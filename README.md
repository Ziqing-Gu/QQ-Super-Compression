# QQ Super Compression 0.1.8

QQ Super Compression 是 Qing Audio 开发的开源、无阈值动态处理器。它没有传统压缩器的 Threshold、Attack 和 Release；核心处理由未来窗口峰值分析、Ratio 增益、Makeup、Mix 与可选响度匹配组成。

QQ Super Compression is an open-source, threshold-free dynamics processor by Qing Audio. It has no conventional Threshold, Attack, or Release controls. Its core combines future-window peak analysis, Ratio gain, Makeup, Mix, and optional loudness matching.

| 项目 / Item | 内容 / Value |
|---|---|
| 当前版本 / Current version | 0.1.8 |
| 发布状态 / Release status | Candidate / Test；尚未由用户确认 Stable / not yet user-confirmed Stable |
| 厂商 / Vendor | Qing Audio |
| 格式 / Formats | Windows x64 VST3; macOS Apple Silicon VST3; macOS Intel VST3; macOS Universal 2 AU |
| 框架 / Framework | JUCE 8.0.15 / CMake |
| 许可证 / License | MIT |
| 公开源码 / Public source | https://github.com/Ziqing-Gu/QQ-Super-Compression |

## 快速链接 / Quick links

- [中文安装与使用说明](docs/QQ%20Super%20Compression%200.1.8%20Windows%E4%B8%8EmacOS%E5%AE%89%E8%A3%85%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E%EF%BC%88%E4%B8%AD%E6%96%87%EF%BC%89.txt)
- [English installation and usage guide](docs/QQ-Super-Compression-0.1.8-Windows-macOS-INSTALL.txt)
- [完整中英双语版本记录 / Full bilingual changelog](CHANGELOG.md)
- [中英双语构建说明 / Bilingual build guide](CODEX_BUILD.md)
- [测试清单 / Test checklist](docs/TEST_CHECKLIST.md)
- [开发交接记录 / Development handoff](AI_DEVELOPMENT_HANDOFF.md)

## 信号流程 / Signal flow

```text
Current Input
  |\
  | +--> future-window peak analysis --> threshold-free Ratio gain
  |
  +--> exact Lookahead delay -------------------------------+
                                                              |
Delayed Dry --------------------------------------------------+
  -> Ratio gain
  -> Wet (pre-Makeup)
  -> ST common / LR independent / MS independent Makeup
  -> Dry/Wet Mix
  -> Output
```

所选 Lookahead 同时决定未来分析窗口长度、真实音频延迟和宿主 PDC。Bypass 走同一条延迟 Dry 路径，因此不会暗中改变延迟。

The selected Lookahead controls the future analysis window, real audio latency, and host-reported PDC. Bypass uses the same delayed Dry path, so bypassing does not silently change latency.

## 核心功能 / Core features

### 无阈值 Ratio / Threshold-free Ratio

未来窗口峰值分析器为每个延迟样本寻找该样本及其后续 `N` 个样本中的最大绝对值。Ratio 公式保持为：

The future-window peak analyser finds the maximum absolute level across each delayed sample and the following `N` samples. The Ratio law remains:

```text
gain = 1 / (1 + (Ratio - 1) * level)
```

Ratio 1:1 在 Makeup/Mix 前为 unity；Ratio 增大时 Wet 电平和增益降低。没有传统 Threshold，也没有用户 Attack/Release 包络。

Ratio 1:1 is unity before Makeup/Mix; increasing Ratio lowers Wet level and gain. There is no conventional Threshold and no user Attack/Release envelope.

### Lookahead 与 PDC / Lookahead and PDC

可选预设为 `0 / 10 / 26 / 40 / 80 / 100 ms`。`lookaheadMs` 参数 ID 保持不变；旧的任意值恢复时迁移到最近预设，等距时选择更长值。现有实例恢复项目状态，新实例使用用户最后一次手动选择；首次默认值为 26 ms。

The presets are `0 / 10 / 26 / 40 / 80 / 100 ms`. The `lookaheadMs` parameter ID is unchanged. Legacy arbitrary values migrate to the nearest preset, choosing the longer value on an exact tie. Existing instances restore project state; new instances use the last manual choice, with 26 ms as the first-run default.

0 ms 会把分析折叠为瞬时样本幅度，存在可测谐波失真，但这是用户要求保留的有意风格选项；不能偷偷加入平滑或隐藏 Lookahead。

At 0 ms, analysis collapses to instantaneous sample magnitude. It has measurable harmonic distortion, but it is an intentional flavour requested by the user; no hidden smoothing or lookahead should be added.

### 工作模式 / Processing modes

- **ST — Stereo Linked：**L/R 独立分析，较强衰减控制共同增益，使用一个共享 Makeup。/ L/R are analysed independently; the stronger reduction drives one common gain and one shared Makeup value.
- **LR — Left/Right Independent：**L/R 独立压缩与独立 Makeup。/ L/R compression and Makeup are independent.
- **MS — Mid/Side Independent：**`M=(L+R)*0.5`、`S=(L-R)*0.5`，M/S 独立分析、压缩与 Makeup 后再解码。/ M/S are independently analysed, compressed, and made up before decoding.
- 模式循环为 `ST -> MS -> LR -> ST`。/ Mode cycles `ST -> MS -> LR -> ST`.

### 严格 Integrated LUFS Match / Strict Integrated LUFS Match

0.1.6 起，Match 使用 BS.1770 / EBU R128 Integrated Loudness：每路两级 K-weighting、400 ms 门限块、75% 重叠（100 ms hop）、-70 LUFS 绝对门限和低于绝对门限后响度 10 LU 的相对门限；未完成的末尾块不参与计算。

Since 0.1.6, Match uses BS.1770 / EBU R128 Integrated Loudness: two-stage K-weighting per stream, 400 ms gating blocks, 75% overlap (100 ms hop), a -70 LUFS absolute gate, and a relative gate 10 LU below the absolute-gated loudness. An incomplete final block is ignored.

Match 计算 `Dry Integrated LUFS - compressed Wet pre-Makeup Integrated LUFS`。测量源固定为延迟 Dry 与压缩后、Makeup 前、Mix 前的 Wet，因此当前 Makeup 和 Mix 不污染测量。结果限制在现有 ±36 dB 参数范围内；静音通道或分量没有有效值时保持原 Makeup。

Match computes `Dry Integrated LUFS - compressed Wet pre-Makeup Integrated LUFS`. It compares delayed Dry with compressed Wet before Makeup and Mix, so current Makeup and Mix do not contaminate the measurement. Results are limited to the existing ±36 dB parameter range; a silent channel/component with no valid result keeps its previous Makeup.

ST 使用一套立体声节目测量并写入共同 Makeup；LR 分别测 L/R；MS 分别测 M/S。分析只在宿主播放期间运行，并在新播放或检测到走带跳变时复位。至少需要一个完整 400 ms 块。

ST uses one stereo-programme measurement and writes common Makeup; LR measures L/R separately; MS measures M/S separately. Analysis runs only during host playback and resets on a new playback run or detected transport discontinuity. At least one complete 400 ms block is required.

### A/B、界面与电平表 / A/B, UI, and meters

- A/B 快照包含 Ratio、ST/L/R/M/S Makeup、Mix、Lookahead 和 Mode；全局 Bypass 不属于 A/B。/ A/B snapshots include Ratio, ST/L/R/M/S Makeup, Mix, Lookahead, and Mode; global Bypass is excluded.
- 旋钮支持 Shift 精调、Alt+左键复位、Ctrl/Cmd+Z Undo、Ctrl/Cmd+Shift+Z Redo。/ Knobs support Shift fine adjustment, Alt+left-click reset, Undo, and Redo.
- Dynamic Display：灰色为延迟 Dry/Input，青色为 Makeup 前 Wet，黄色为 Makeup/Mix 后最终 Output。/ Dynamic Display uses grey for delayed Dry/Input, cyan for pre-Makeup Wet, and yellow for final post-Makeup/post-Mix Output.
- 三组双通道表为 Input、Output、Gain Reduction。ST/LR 显示 L/R，MS 显示 M/S；GR 从顶部 0 dB 向下增长。/ Three dual-channel groups show Input, Output, and Gain Reduction. ST/LR show L/R, MS shows M/S, and GR grows downward from 0 dB.
- 每路 GR 具有 2 秒自动峰值保持，只影响显示；切换 Mode 或 Lookahead 会清除旧标记。0.1.8 移除了 `H` 前缀并放大数值。/ Each GR channel has a 2-second automatic peak hold that is display-only. Changing Mode or Lookahead clears stale markers. Version 0.1.8 removes the `H` prefix and enlarges the value.
- 0.1.8 将完整 1020x670 界面按 1:1 比例统一缩放，并把旧的非等比窗口尺寸迁移到能放入旧矩形的最大等比尺寸。/ Version 0.1.8 uniformly scales the complete 1020x670 interface at a true 1:1 aspect ratio and migrates old non-proportional saved sizes to the largest uniform scale fitting the previous rectangle.

## 平台与包 / Platforms and packages

| 包 / Package | 内容 / Contents | 已验证环境 / Validated environment |
|---|---|---|
| `QQ-Super-Compression-0.1.8-Windows-x64.zip` | `QQ Super Compression.vst3` | GitHub Actions `windows-2022`; 64-bit Windows VST3 hosts |
| `QQ-Super-Compression-0.1.8-macOS-Apple-Silicon-VST3.zip` | arm64 VST3 | GitHub Actions `macos-14` |
| `QQ-Super-Compression-0.1.8-macOS-Intel-VST3.zip` | x86_64 VST3 | GitHub Actions `macos-15-intel` |
| `QQ-Super-Compression-0.1.8-macOS-Universal-AU.zip` | Universal 2 AU component | arm64 + x86_64; `auval` passed in CI |

macOS bundle 的 Info.plist 没有声明最低系统版本。早期系统未验证；发布包为临时签名（ad-hoc signed），没有 Apple Developer ID 公证。详情见两份安装说明。

The macOS bundle Info.plist does not declare a minimum system version. Earlier systems are unverified. Release bundles are ad-hoc signed and not notarized with Apple Developer ID. See the installation guides for details.

## 构建与测试 / Build and test

本地和 CI 构建命令、固定依赖、验证目标与已知非致命警告见 [CODEX_BUILD.md](CODEX_BUILD.md)。JUCE-free 的响度参考自测位于 `tests/bs1770_match_selftest.cpp`。

See [CODEX_BUILD.md](CODEX_BUILD.md) for local/CI commands, pinned dependencies, validation targets, and the known non-fatal warning. The JUCE-free loudness reference self-test is at `tests/bs1770_match_selftest.cpp`.

## 版本记录 / Version history

下面是公开 README 直接可见的完整版本区间。更详细记录见 [CHANGELOG.md](CHANGELOG.md)。所有版本均为 Candidate/Test，不代表用户已确认 Stable。

The complete public README range is directly visible below. See [CHANGELOG.md](CHANGELOG.md) for details. Every version remains Candidate/Test and is not user-confirmed Stable.

### 0.1.8 — GR Hold 可读性与统一 1:1 缩放 / GR Hold Readability and Uniform 1:1 Scaling

- 日期 / Date: 2026-08-25; 状态 / Status: Candidate / Test.
- 中文：移除 GR Hold 的 `H` 前缀、增大数值字体；完整 1020x670 界面改为等比缩放并迁移旧的非等比尺寸。DSP、LUFS Match、Lookahead、参数 ID 和状态结构不变。2026-08-26 补齐 MIT、公开 CI、Plan D 与中英双语发布文档。
- English: Removed the GR Hold `H` prefix, enlarged its value, introduced uniform scaling for the full 1020x670 UI, and migrated old non-proportional sizes. DSP, LUFS Match, Lookahead, parameter IDs, and state schema are unchanged. MIT, public CI, Plan D, and complete bilingual release documentation were added on 2026-08-26.

### 0.1.7 — 自动 GR Peak Hold / Automatic GR Peak Hold

- 日期 / Date: 2026-08-25; 状态 / Status: Candidate / Test.
- 中文：加入每路 2 秒自动 GR 峰值保持、模式/Lookahead 清除逻辑、版本小标签，并将 `playHead` 重命名为 `hostPlayHead` 消除名称遮蔽警告；DSP 其余部分不变。
- English: Added per-channel 2-second automatic GR peak hold, Mode/Lookahead clearing, a small version label, and renamed `playHead` to `hostPlayHead` to remove a shadow warning; the remaining DSP is unchanged.

### 0.1.6 — 严格 Integrated LUFS Match / Strict Integrated LUFS Match

- 日期 / Date: 2026-08-25; 状态 / Status: Candidate / Test.
- 中文：用带 K-weighting、400 ms/75% overlap、绝对和相对门限的 BS.1770/EBU R128 Integrated Loudness 替换 RMS Match；保持未来峰值核心不变。
- English: Replaced RMS Match with BS.1770/EBU R128 Integrated Loudness using K-weighting, 400 ms/75%-overlap blocks, and absolute/relative gating; the future-peak core is unchanged.

### 0.1.5 — 固定 Lookahead 预设与记忆 / Fixed Lookahead Presets and Memory

- 日期 / Date: 2026-08-25; 状态 / Status: Candidate / Test.
- 中文：把任意输入改为 `0/10/26/40/80/100 ms` 六档，保留 `lookaheadMs` 并迁移旧值；现有实例读项目，新实例记住最后选择，首次默认 26 ms。
- English: Replaced arbitrary entry with six `0/10/26/40/80/100 ms` presets, retained `lookaheadMs` with legacy migration, restored projects per instance, remembered the last choice for new instances, and used 26 ms as the first-run default.

### 0.1.4 — 可变 Lookahead 未来峰值实验 / Variable-Lookahead Future-Peak Experiment

- 日期 / Date: 2026-08-25; 状态 / Status: Candidate / Test.
- 中文：在 PluginDoctor 检查后淘汰 20 ms RMS 检测器，改用 0–100 ms 未来窗口峰值；Lookahead 同时控制分析、真实延迟与 PDC，Bypass 保持同延迟路径。0 ms 有意保留失真风格。
- English: Rejected the 20 ms RMS detector after PluginDoctor testing and introduced a 0–100 ms future-window peak detector. Lookahead controls analysis, real delay, and PDC; Bypass stays on the same delayed path. The 0 ms distorted flavour is intentional.

### 0.1.3 — 工作流、A/B、Match 与独立 Makeup / Workflow, A/B, Match, and Independent Makeup

- 日期 / Date: 2026-08-25; 状态 / Status: Candidate / Test.
- 中文：加入 A/B 与复制、Undo/Redo、精调/复位、按钮式 Bypass、模式循环、窗口尺寸记忆；ST 共用 Makeup，LR/MS 独立 Makeup；加入初版能量 Match（尚非严格 LUFS）。
- English: Added A/B and copy, Undo/Redo, fine/reset gestures, button Bypass, mode cycling, and editor-size memory. ST uses common Makeup, LR/MS use independent Makeup, and an initial energy Match was added (not yet strict LUFS).

### 0.1.2 — ST/MS/LR、零延迟与双表 / ST/MS/LR, Zero Latency, and Dual Meters

- 日期 / Date: 2026-08-25; 状态 / Status: Candidate / Test.
- 中文：移除固定 10 ms 延迟和 Makeup Gate，加入 ST/MS/LR 模式、双通道电平表与向下增长的 GR；保留 0.1.1 的无阈值电平域 Ratio。
- English: Removed fixed 10 ms latency and Makeup Gate, added ST/MS/LR modes, dual-channel meters, and downward-growing GR, while retaining the 0.1.1 threshold-free level-domain Ratio.

### 0.1.1 — Ratio 引擎修复与电平表/UI / Ratio Engine Fix and Meter/UI Pass

- 日期 / Date: 2026-08-25; 状态 / Status: Candidate / Test.
- 中文：把会随 Ratio 抬高音量并严重失真的样本域 waveshaper 改成电平域增益控制；加入三组表、动态显示布局和 UTF-8/CJK 支持。
- English: Replaced the sample-domain waveshaper—which raised level with Ratio and caused severe distortion—with level-domain gain control; added three meter groups, a Dynamic Display layout, and UTF-8/CJK support.

### 0.1.0 — 原型 / Prototype

- 日期 / Date: 2026-08-25; 状态 / Status: Candidate / Test.
- 中文：首个 Ratio、Makeup、Makeup Gate、Mix 与 0/10 ms 原型。该版 Ratio 是后来被淘汰的样本域 waveshaper。
- English: Initial Ratio, Makeup, Makeup Gate, Mix, and 0/10 ms prototype. Its Ratio was the later-rejected sample-domain waveshaper.

### 版本覆盖审计 / Version coverage audit

- 上一个完整公开中英双语记录 / Previous complete public bilingual record: 无，属于首次纠正 / none; this is the initial correction.
- 本次目标 / Target: 0.1.8.
- 实际连续版本 / Actual continuous versions: 0.1.0, 0.1.1, 0.1.2, 0.1.3, 0.1.4, 0.1.5, 0.1.6, 0.1.7, 0.1.8.
- README 覆盖 / README coverage: 全部 / all.
- CHANGELOG 覆盖 / CHANGELOG coverage: 全部 / all.
- 遗漏 / Omissions: 无 / none.

## 兼容性与开发约束 / Compatibility and development constraints

- 源文件保持 UTF-8；MSVC 保持 `/utf-8`；Windows 使用 Microsoft YaHei 回退，macOS 使用 PingFang SC 回退。/ Keep source files UTF-8 and MSVC `/utf-8`; use Microsoft YaHei fallback on Windows and PingFang SC on macOS.
- 修改代码前阅读 `AI_DEVELOPMENT_HANDOFF.md`，有意义的改动必须追加记录。/ Read `AI_DEVELOPMENT_HANDOFF.md` before code changes and append a record for every meaningful change.
- 0.1.8 仍为 Candidate/Test；只有用户完成宿主与听感验证后才能决定是否标记 Stable。/ Version 0.1.8 remains Candidate/Test; only the user can decide whether it becomes Stable after host and listening validation.
