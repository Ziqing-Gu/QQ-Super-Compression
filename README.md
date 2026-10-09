# ⚠️ 禁止商业使用 / NO COMMERCIAL USE

## Qing Audio 非商业源码共享许可证 1.0

### 本项目源码公开，但不属于 OSI 认可的开源软件

> **禁止任何商业使用。** 仅允许个人、学习、教育、研究、评估、爱好及其他非商业用途。发布原版、二进制版或修改版时，必须同时免费公开完整对应源代码，保留作者、版权和许可证声明，醒目标明原项目名称、作者、来源链接、修改者、修改日期及修改内容，并使整个修改版继续采用同一许可证。完整条款见 [LICENSE](LICENSE)。
>
> **NO COMMERCIAL USE.** Use is permitted only for personal, educational, research, evaluation, hobby, charitable, and other non-commercial purposes. Any distributed original, binary, or modified version must provide the complete corresponding source without charge, preserve authorship, copyright, and license notices, prominently identify the original project, author, source URL, modifier, date, and changes, and license the entire modified work under the same terms. See [LICENSE](LICENSE).
>
> 许可证政策变更与后续 AI 维护说明见 [LICENSE_POLICY_CHANGE.md](LICENSE_POLICY_CHANGE.md)。 / See [LICENSE_POLICY_CHANGE.md](LICENSE_POLICY_CHANGE.md) for the policy record and future AI maintenance instructions.

# QQ Super Compression 1.3.2 Stable

Qing Audio 压缩/限制器，支持 Classic/Super、Single/Dual、ST/LR/MS、内外侧链与 Limiter。SAFE 默认关闭；Normal 使用 Distort，Safe 使用 Window；统一 1x/4x/8x/16x 过采样作用于所有 Lookahead。Lookahead 档位为 0/26/40/80/100 ms。

Qing Audio compressor/limiter with Classic/Super, Single/Dual, ST/LR/MS, internal/external sidechains and Limiter. SAFE defaults off; Normal uses Distort and Safe uses Window. Unified 1x/4x/8x/16x audio oversampling works at every Lookahead. Presets: 0/26/40/80/100 ms.

## 下载与安装 / Download and installation

**当前正式版 / Latest stable:** [全部 Release / All releases](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/latest) · [QQ Super Compression 1.2.42](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/tag/v1.2.42)

请按操作系统、宿主架构与插件格式选择文件。Mac VST3 仅需安装与宿主架构相符的一包；需要 AU 时另选 Universal 2 AU。 / Choose by OS, host architecture and plugin format. Install only the matching Mac VST3 package; choose Universal 2 AU separately when needed.

- Windows 10/11 x64 · VST3：[QQ.Super.Compression.1.2.42.Windows.x64.VST3.zip](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.2.42/QQ.Super.Compression.1.2.42.Windows.x64.VST3.zip)
- macOS 11+ · Apple Silicon arm64 · VST3：[QQ.Super.Compression.1.2.42.macOS.Apple.Silicon.VST3.zip](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.2.42/QQ.Super.Compression.1.2.42.macOS.Apple.Silicon.VST3.zip)
- macOS 11+ · Intel x86_64 · VST3：[QQ.Super.Compression.1.2.42.macOS.Intel.x86_64.VST3.zip](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.2.42/QQ.Super.Compression.1.2.42.macOS.Intel.x86_64.VST3.zip)
- macOS 11+ · Universal 2 · AU：[QQ.Super.Compression.1.2.42.macOS.Universal.2.AU.zip](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.2.42/QQ.Super.Compression.1.2.42.macOS.Universal.2.AU.zip)
- 中文安装说明 / Chinese installation guide：[QQ.Super.Compression.1.2.42.Installation.Guide.Chinese.txt](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.2.42/QQ.Super.Compression.1.2.42.Installation.Guide.Chinese.txt)
- English installation guide / 英文安装说明：[QQ.Super.Compression.1.2.42.Installation.Guide.English.txt](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.2.42/QQ.Super.Compression.1.2.42.Installation.Guide.English.txt)
- 中文用户手册 / Chinese user manual：[QQ.Super.Compression.1.2.42.User.Manual.Chinese.pdf](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.2.42/QQ.Super.Compression.1.2.42.User.Manual.Chinese.pdf)
- English user manual / 英文用户手册：[QQ.Super.Compression.1.2.42.User.Manual.English.pdf](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.2.42/QQ.Super.Compression.1.2.42.User.Manual.English.pdf)

本版未提供 Linux 成品。升级前请保存工程、退出 DAW 并备份旧插件；macOS 包为临时签名且未经公证，遇到系统安全提示请阅读对应安装说明。
No Linux build is provided. Save the session, quit the DAW and back up the old plugin before upgrading. macOS bundles are ad-hoc signed and not notarized; consult the installation guide for system security prompts.

**注意 / Note:** GitHub 自动生成的 Source code (zip) 和 Source code (tar.gz) 是源码快照，不能直接安装为插件。 / GitHub-generated Source code (zip) and Source code (tar.gz) are source snapshots, not installable plugins.

## 当前稳定版 / Current stable version

**1.3.2 (2026-10-09).** SAFE 开关、Distort/Window、统一过采样与 Display 对齐更新；完整变化见下方逐版本记录及 [双语发行说明](RELEASE_NOTES_1.3.2.md)。Windows 复用已验证成品，macOS 由当前源码工作流生成；完整交付状态见 Release。

**1.3.2 (2026-10-09).** SAFE toggle, Distort/Window, unified oversampling and Display alignment updates. See the per-version history below and [bilingual release notes](RELEASE_NOTES_1.3.2.md). The verified Windows binary is reused; macOS artifacts are built from the current source workflow. See Release for delivery status.

## 版本记录 / Version history

### 1.3.2 — Stable, 2026-10-09

- 中文：检测切换统一为 SAFE 按钮，位于 Limiter 右侧。新实例默认关闭；点亮为黄色 Safe，关闭为 Normal。Normal 显示 Distort 百分比，Safe 显示 Window 毫秒；切换保持实际窗口长度。更新配套中英文 43 页手册。DSP 沿用已验证的 1.3.1 后续方案。
- English: A fixed SAFE button sits to the right of Limiter, off by default in new instances. Lit yellow means Safe; unlit means Normal. Normal shows Distort percent, Safe shows Window in milliseconds; switching retains the actual window. Includes matching 43-page Chinese and English manuals. DSP retains the verified final 1.3.1 design.

### 1.3.1 — Development series consolidated into 1.3.2

- 中文：在固定 Lookahead 基础延迟内加入可调检测窗口。Normal 的 Distort 0% 对应全窗口，100% 对应零窗口；Safe 的 Window 为 0 至当前 Lookahead。手动切换 Lookahead 和 Alt 单击恢复全窗口，工程及 A/B 保留已存设置。过采样统一为 1x/4x/8x/16x，覆盖所有 Lookahead 的动态与 Ceiling；保留 1x 音频下的独立 TP 重建检测。移除 10 ms 档，旧 10/5 ms 状态迁移至 26 ms，最终档位为 0/26/40/80/100 ms。Limiter 上压阈值不再参与 Output/Makeup 联动，Super 最低 -inf、Classic 最低 -90 dB；下压和 Makeup 联动保留。修正 0 ms、Lookahead 切换及 A/B 的 Display 对齐。
- English: Adds an adjustable detector window within the fixed Lookahead base delay. Normal Distort maps 0% to the full window and 100% to zero; Safe Window spans zero to the current Lookahead. Manual Lookahead changes and Alt-click restore the full window; projects and A/B retain saved settings. Unified 1x/4x/8x/16x audio oversampling covers dynamics and Ceiling at every Lookahead, including independent TP reconstruction at native 1x audio. Removes the 10 ms preset; old 10/5 ms states migrate to 26 ms. Final presets are 0/26/40/80/100 ms. Limiter UP Threshold no longer links to Output/Makeup; its Super floor is -inf and Classic floor is -90 dB. DOWN and Makeup linking remain. Corrects Display alignment at 0 ms and across Lookahead/A/B changes.

### 1.3.0 — Detector investigation and comparison candidate

- 中文：针对带 Delay/混响素材的持续噼啪声，引入前后窗口峰值取较小值与取较大值的对比方案，并排查 1:1 Display 参考对齐。较大峰值方案以提前压及峰后压为代价，帮助抑制快速增益变化；相关候选方案在 1.3.1 中进一步整理。本系列未采用隐藏 Attack/Release 作为动态修复。
- English: Investigates persistent crackles on delay/reverb material with a comparison between the smaller and larger of past/future window peaks, alongside unity-ratio Display reference alignment. The larger-peak approach trades pre/post-attenuation for reduced rapid gain changes; the candidate designs are consolidated in 1.3.1. No hidden Attack/Release stage was introduced as the dynamics fix.

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

- [中文说明书，43 页](docs/manuals/QQ-Super-Compression-1.3.2-User-Manual-Chinese.pdf) / [English manual, 43 pages](docs/manuals/QQ-Super-Compression-1.3.2-User-Manual-English.pdf)
- [中文安装指南](docs/QQ-Super-Compression-1.3.2-INSTALL-CN.txt) / [English installation guide](docs/QQ-Super-Compression-1.3.2-INSTALL-EN.txt)
- [当前文档状态 / Current document status](docs/CURRENT_DOCUMENT_STATUS.md)

## 使用要点 / Basic use

Limiter 通常从 Ratio 20:1–1000:1 开始，保持 Output LINK，缓慢降低阈值。0 ms Lookahead 可配合 4x / 8x / 16x 总过采样试听响段与静段的动态对比。下压调好后，可适度加入 1:1–1:1.2 上压，并留意底鼓与军鼓延音。

For Limiter, start with Ratio 20:1–1000:1, keep Output LINK on and lower Threshold gradually. Try 0 ms Lookahead with 4x / 8x / 16x overall oversampling for loud/quiet contrast. After downward compression, optional gentle 1:1–1:1.2 upward compression needs care with kick/snare sustain.

## 编译 / Build

JUCE 固定为 8.0.15；CMake 3.22+。Windows 使用 VS 2022 C++ 工具与 Windows SDK，macOS 使用 Xcode Command Line Tools。macOS 最低部署版本为 11.0。详见 [复现说明 / reproduction](REPRODUCE_1.3.2.md)。Windows Actions 仅手动触发，本次交付复用已验证 Windows 成品。

JUCE is pinned to 8.0.15. Use CMake 3.22+, VS 2022 C++ tools and Windows SDK on Windows, or Xcode Command Line Tools on macOS. The macOS deployment target is 11.0. See [reproduction instructions](REPRODUCE_1.3.2.md). Windows Actions remains manual; this release reuses its verified Windows build.

## 已知限制 / Known limits

Safe 的较长 Window 会提前压及峰后压；较短窗口可能增加染色或噼啪声。统一过采样减少混叠但不保证所有素材无失真。旧工程升级后请核对 OS 和延迟补偿。

Longer Safe windows can cause pre/post-attenuation. Shorter windows can increase coloration or crackles. Unified oversampling reduces aliasing but does not guarantee distortion-free processing. Check OS and host latency compensation after migrating old sessions.

FULL may stay awake at extreme gain or with active input/selected sidechain. ECO cuts tails when the host reports Stop. Plugin CPU timings cannot be translated to an ASIO-Guard percentage; DAW scheduling and buffer conditions matter. macOS artifacts are ad-hoc signed and may require trusted-source quarantine handling as described in the installation guides. Commercial use is prohibited; redistributed binaries require free complete corresponding source and the required notices.
