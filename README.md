# ⚠️ 禁止商业使用 / NO COMMERCIAL USE

## Qing Audio 非商业源码共享许可证 1.0

### 本项目源码公开，但不属于 OSI 认可的开源软件

> **禁止任何商业使用。** 仅允许个人、学习、教育、研究、评估、爱好及其他非商业用途。发布原版、二进制版或修改版时，必须同时免费公开完整对应源代码，保留作者、版权和许可证声明，醒目标明原项目名称、作者、来源链接、修改者、修改日期及修改内容，并使整个修改版继续采用同一许可证。完整条款见 [LICENSE](LICENSE)。
>
> **NO COMMERCIAL USE.** Use is permitted only for personal, educational, research, evaluation, hobby, charitable, and other non-commercial purposes. Any distributed original, binary, or modified version must provide the complete corresponding source without charge, preserve authorship, copyright, and license notices, prominently identify the original project, author, source URL, modifier, date, and changes, and license the entire modified work under the same terms. See [LICENSE](LICENSE).
>
> 许可证政策变更与后续 AI 维护说明见 [LICENSE_POLICY_CHANGE.md](LICENSE_POLICY_CHANGE.md)。 / See [LICENSE_POLICY_CHANGE.md](LICENSE_POLICY_CHANGE.md) for the policy record and future AI maintenance instructions.

# QQ Super Compression 1.2.42 Stable

Qing Audio 的压缩/限制器插件，适用于人声、乐器、总线和母带。支持 Classic / Super 曲线、Single / Dual、ST / LR / MS、内外侧链、Limiter、0 ms Lookahead 下 1x / 4x / 8x / 16x 核心过采样，以及独立的 Hard Clip / TP Ceiling 过采样。

Qing Audio compressor/limiter for vocals, instruments, buses and mastering. Features Classic / Super curves, Single / Dual compression, ST / LR / MS, internal/external sidechain, Limiter, 1x / 4x / 8x / 16x core oversampling at 0 ms Lookahead and independent Hard Clip / TP Ceiling oversampling.

## 当前稳定版 / Current stable version

**1.2.42 (2026-10-01).** FULL 在宿主明确停播且仅有极低残留时，经输入、增益、输出和尾音安全检查后暂停主要 DSP；普通底噪和实时监听继续处理。ECO 沿用停播即停止处理。播放、录音、离线渲染与宿主状态未知时完整处理；无插件主输出启停 Crossfade，延迟不变。

**1.2.42 (2026-10-01).** On a known Stop, FULL suspends major DSP only after conservative low-residual input, gain, output and tail checks; normal noise and live monitoring keep processing. ECO retains transport-stop suspension. Playback, recording, offline rendering and unknown transport fully process. No new main-output Stop/Start crossfade or latency change.

Windows automated checks and actual VST3 residual tests passed. Per-plugin CPU time does not equal Cubase ASIO-Guard percentage. See [release notes](RELEASE_NOTES_1.2.42.md), [validation record](Verification/1.2.42-Windows/validation-report.json) and [Stable record](STABLE_1.2.42.json).

## 版本记录 / Version history

### 1.2.42 — Stable, 2026-10-01

- 中文：新增 FULL 停播极低残留保守休眠，保留实时监听与播放/录音/离线处理；无输出渐变及延迟变化。Windows 自动测试通过，Cubase ASIO-Guard 数值未量化。
- English: Added conservative FULL low-residual sleep on known Stop while retaining live monitoring and playback/recording/offline processing. No output fade or latency change. Windows automated tests passed; Cubase ASIO-Guard was not quantified.

### 1.2.41 — Stable, 2026-10-01

- 中文：ECO 宿主明确停播即停止处理，恢复时清除旧缓存；FULL 保留实时监听。优化 DSP 环形索引，Windows 自动检查通过。
- English: ECO suspends processing on a known host Stop and clears stale state on resume; FULL retains live monitoring. DSP ring-index optimization and Windows automated checks passed.

### 1.2.40 — Diagnostic candidate

- 中文：补充宿主停播输入峰值及侧链来源诊断，未改变音频休眠规则。
- English: Added stopped-input peak and sidechain-source diagnostics without changing sleep behavior.

### 1.2.39 — Diagnostic candidate

- 中文：修复性能诊断提示入口及新实例 FULL/ECO 选择记忆；不宣称解决停播负载。
- English: Repaired performance diagnostics entry and last-choice recall for new FULL/ECO instances; no stopped-load fix was claimed.

### 1.2.38 — Performance candidate

- 中文：优化 0 ms 检测器及 ECO 关窗分析，增加停播计时诊断；后续由 1.2.39 修复诊断入口。
- English: Optimized 0 ms detection and closed-editor ECO analysis and added stopped-time diagnostics; the diagnostics entry was repaired in 1.2.39.

### 1.2.37 — Performance candidate

- 中文：尝试降低停播负载；用户 Cubase 测试未观察到改善，因此没有提升为 Stable。
- English: Attempted to reduce stopped load; the user's Cubase test did not show improvement, so this was not promoted to Stable.

### 1.2.36 — Previous candidate

- 中文：加入独立 Core/Ceiling 过采样倍率及 Limiter 相关更新；早期 Stable 提升随后撤回。
- English: Added independent Core/Ceiling oversampling options and Limiter updates; an earlier Stable promotion was subsequently withdrawn.

更早的详细变更记录保留于各版 `RELEASE_NOTES_*.md`。 / Earlier detailed records remain in the versioned `RELEASE_NOTES_*.md` files.

## 安装与说明书 / Installation and manuals

Windows 10/11 x64 提供 VST3；macOS 11+ 提供 Apple Silicon VST3、Intel x86_64 VST3 与 Universal 2 AU。按宿主实际架构选一个 macOS VST3 包，AU 按宿主需要安装。升级前关闭宿主并备份工程及旧 bundle。

Windows 10/11 x64 uses VST3. macOS 11+ has Apple Silicon VST3, Intel x86_64 VST3 and Universal 2 AU. Select one macOS VST3 architecture for the host. Close the host and back up the session and old bundle before upgrading.

- [中文说明书，41 页](docs/manuals/QQ-Super-Compression-1.2.42-User-Manual-Chinese.pdf) / [English manual, 41 pages](docs/manuals/QQ-Super-Compression-1.2.42-User-Manual-English.pdf)
- [中文安装指南](docs/QQ-Super-Compression-1.2.42-INSTALL-CN.txt) / [English installation guide](docs/QQ-Super-Compression-1.2.42-INSTALL-EN.txt)
- [当前文档状态 / Current document status](docs/CURRENT_DOCUMENT_STATUS.md)

## 使用要点 / Basic use

Limiter 通常从 Ratio 20:1–1000:1 开始，保持 Output LINK，缓慢降低阈值。0 ms Lookahead 可配合 4x / 8x / 16x 核心过采样试听响段与静段的动态对比。下压调好后，可适度加入 1:1–1:1.2 上压，并留意底鼓与军鼓延音。

For Limiter, start with Ratio 20:1–1000:1, keep Output LINK on and lower Threshold gradually. Try 0 ms Lookahead with 4x / 8x / 16x core oversampling for loud/quiet contrast. After downward compression, optional gentle 1:1–1:1.2 upward compression needs care with kick/snare sustain.

## 编译 / Build

JUCE is pinned to 8.0.15. Windows requires VS 2022 C++ Build Tools, Windows SDK and CMake 3.22+; run `BUILD_WINDOWS.cmd` in an x64 Native Tools environment. The helper builds under `D:\Codex\Temp\QQSC1242-Build` by default and does not install. CMake can use the included `Dependencies/JUCE-8.0.15` or fetch the pinned source. Manual macOS workflows build arm64 and x86_64 VST3 and Universal 2 AU with macOS deployment target 11.0.

## 已知限制 / Known limits

FULL may stay awake at extreme gain or with active input/selected sidechain. ECO cuts tails when the host reports Stop. Plugin CPU timings cannot be translated to an ASIO-Guard percentage; DAW scheduling and buffer conditions matter. macOS artifacts are ad-hoc signed and may require trusted-source quarantine handling as described in the installation guides. Commercial use is prohibited; redistributed binaries require free complete corresponding source and the required notices.
