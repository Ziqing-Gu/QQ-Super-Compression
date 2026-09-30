# QQ Super Compression 1.2.41 Stable

**Qing Audio 非商业源码共享许可证 1.0；仅允许非商业使用。** 完整条款见 [LICENSE](LICENSE)，许可政策说明见 [LICENSE_POLICY_CHANGE.md](LICENSE_POLICY_CHANGE.md)。

**Qing Audio Non-Commercial Source-Share License 1.0; non-commercial use only.** See [LICENSE](LICENSE) and [LICENSE_POLICY_CHANGE.md](LICENSE_POLICY_CHANGE.md). Third-party licenses remain unchanged.

QQ Super Compression 用于人声、乐器、总线和母带，提供 Classic / Super 曲线、单压 / 双压、ST / LR / MS、内部 / 外部侧链，以及 Hard Clip / TP 最终峰值保护。动态核心通过 Lookahead、Ratio、Threshold、Range 和 Mix 调整，不使用传统 Attack / Release 旋钮。

QQ Super Compression provides Classic / Super curves, Single / Dual compression, ST / LR / MS, internal / external sidechain, and final Hard Clip / TP protection for vocals, instruments, buses and mastering. The dynamics core uses Lookahead, Ratio, Threshold, Range and Mix rather than conventional Attack / Release knobs.

## Limiter 的核心特色 / The defining Limiter feature

Limiter 使用 **0 ms Lookahead** 时，可以让副歌很响，同时保持安静部分的音量，不必把整首歌的音量都抬高很多。配合 **4x / 8x / 16x 核心过采样** 试听，保留响段与静段的动态对比，这是超级压缩 Limiter 最值得尝试的特色。

With **0 ms Lookahead** in Limiter, the chorus can become very loud while retaining quiet-section levels, without raising the whole song by a large amount. Try **4x / 8x / 16x core oversampling** and listen to the contrast between loud and quiet passages. This is the Limiter's standout feature and is well worth trying.

常规用法：Ratio 设为 **20:1 至 1000:1**，保持 **Output LINK**，逐渐下拉 Threshold，让 Output 自动补偿。下压调好以后，可切 DUAL，关闭 Ratio LINK、保留 Output LINK，从 **UP 1:1 到 1:1.2** 少量补充。留意底鼓和军鼓延音；双压效果不好时可改用分段压缩。Mix 留到这套处理调好以后再决定。

Standard use: set Ratio to **20:1-1000:1**, keep **Output LINK** on, and gradually lower Threshold so Output compensates. After setting downward compression, try DUAL with Ratio LINK off and Output LINK on, adding gentle **UP between 1:1 and 1:1.2**. Listen to kick and snare sustain; try segmented compression if Dual is unsuitable. Decide on Mix after settling the processing.

## 1.2.41 ECO 停播省电 / ECO transport-stop suspension

按用户确认的工作方式：**ECO 在宿主停止时立即输出静音并暂停 DSP，不以输入全零为条件；FULL 保留实时监听和原有处理行为。** 界面开关、硬件底噪和侧链输入不阻止 ECO 停播。播放、录音、离线渲染或切回 FULL 时，在当前块恢复；宿主状态未知时继续处理。ECO 停播会截断当时的尾音，停止时需要试听硬件输入请使用 FULL。

ECO immediately mutes and suspends DSP on a known stopped transport, regardless of input noise, sidechain activity or editor visibility. FULL retains live processing. Playback, recording, offline rendering and switching to FULL resume processing in the current block; unknown transport remains active. ECO stop cuts the current tail; use FULL for live monitoring while stopped.

恢复时清除停止前的延迟/滤波器缓存并采用当前参数，保留插件报告的延迟。FULL/ECO 仍按实例保存，新实例记住上次明确选择。诊断中的 `ECO transport stop` 和 sleep 比例用于核对实际宿主是否走到暂停路径。

Resume clears pre-stop carrier/filter histories and uses current controls while preserving reported latency. FULL/ECO remains per-instance with the last explicit selection used for new instances. The diagnostics report ECO transport stop separately from exact-zero sleep.

本版另以等价的条件回绕替代压缩器/TP 环形索引中的整数取余，降低持续输入时的开销，保留音频运算、过采样倍率和保护算法。验证记录见 `Verification/1.2.41-Windows`；独立测试不代表 Cubase ASIO-Guard 实测。1.2.41 已按用户确认提升为 Stable。

## 1.2.40 停播输入测量 / Stopped-input measurements

1.2.39 的 Cubase 实测：ECO 关窗共 983 块，宿主已停止（playing=0%），输入严格全零与休眠均为 0%，阻塞原因为 nonzero input。关窗 7619.1 us/block、开窗 8449.9 us/block，N=1024、Core/Ceiling=16x/16x。报告未提供非零输入幅度，不能据此判断为噪声或忽略它。

The supplied 1.2.39 Cubase report records stopped transport and no all-zero or sleeping blocks. The nonzero input gate keeps DSP awake. Its amplitude/source was not measured, so no noise-floor assumption is justified.

1.2.40 增加停播期间主输入和已选外部侧链的每块峰值范围（科学计数与 dBFS）、侧链选择、外部总线通道数和非有限值计数。测量在输入增益之前完成，播放块不计入；关窗数据继续保留。音频样本、休眠条件及门限保持原样，本版用于补齐诊断，不宣称解决静音负载。Plan B/C/D 保持停止。

1.2.40 adds stopped raw main/selected-external-key block-peak ranges, scientific amplitudes and dBFS, key selection, bus channels and nonfinite counts. Playback is excluded. Audio samples and sleep policy are unchanged; this is a diagnostic candidate, not a claimed silent-load fix. Plan B/C/D remain stopped.

## 1.2.39 诊断入口与默认选择 / Diagnostics and new-instance defaults

用户确认 1.2.38 的 ECO 有改善，停播静音负载仍偏高。1.2.38 悬停诊断缺少 TooltipWindow，且动态文字不断重置悬停计时。1.2.39 补齐稳定的提示，并提供 **右键 FULL/ECO → Performance / Idle diagnostics → Copy report**。面板保留关窗期间数据，打开时冻结快照，Refresh 可刷新。

1.2.38 ECO improvement is user-confirmed; stopped-silence load remains unresolved. 1.2.39 fixes the missing tooltip window and changing tooltip text, and provides an explicit right-click FULL/ECO diagnostic panel with selectable/copyable snapshots.

新实例采用上次明确点击的 FULL/ECO 选择；每个实例保持独立。工程/预设保存的实例状态优先于默认值，重新打开编辑器和 A/B 不覆盖选择。没有偏好时首次默认 FULL；旧工程缺少实例属性时仍按原规则恢复 FULL。

New instances inherit the last explicit FULL/ECO choice. Existing instances remain independent; stored project/preset state takes precedence. Editor reopen and A/B do not overwrite the choice. First use without a preference, and legacy project states lacking the instance property, retain FULL fallback.

音频处理与 1.2.38 相同；本次修复诊断可见性及选择记忆，不宣称静音负载已解决。Plan B/C/D 保持停止。
Audio processing is unchanged from 1.2.38. This update repairs diagnostics and default recall; it does not claim to resolve the remaining silent-load issue. Plan B/C/D remain stopped.

## 1.2.38 性能候选版 / Performance candidate

用户实测 1.2.37 在 Cubase 中未改善停止播放后的高负载；先前的独立测试结果不能视为宿主验收通过。1.2.36 Stable 提升已撤回，Plan B/C/D 发布工作保持停止。
The user reported no improvement from 1.2.37 in Cubase stopped-silence load. Standalone timings did not establish host acceptance. The 1.2.36 Stable promotion remains withdrawn and release work remains stopped.

1.2.38 优化 0 ms 检测器，ECO 隐藏窗口时减少未使用模式的增益曲线运算，并保留切换所需的状态与滤波器历史。1.2.38 记录了关窗期间的处理耗时、全零输入与休眠比例，诊断显示入口在 1.2.39 修复。音频与性能对照见 `Verification/1.2.38-Windows`；Cubase 尚未验收。
1.2.38 streamlines the zero-lookahead detector and skips unused gain-curve work in hidden ECO while retaining state and filter history for transitions. 1.2.38 records closed-editor timing, exact-zero input and sleep statistics; its display entry is repaired in 1.2.39. Audio and performance comparisons are recorded in `Verification/1.2.38-Windows`; Cubase acceptance is pending.

## 1.2.36 功能更新 / Feature changes

- 0 ms 核心：1x / 4x / 8x / 16x；Lookahead 大于 0 ms 时核心为 1x。Core at 0 ms: 1x / 4x / 8x / 16x; above 0 ms the core uses 1x.
- 独立 CEILING OS：Hard Clip 为 1x / 4x / 8x / 16x；TP 为 4x / 8x / 16x。Independent CEILING OS: Hard Clip at 1x / 4x / 8x / 16x; TP at 4x / 8x / 16x.
- 两处初始值均为 8x；TP+存储 1x 临时用 8x，关闭 TP 恢复 1x；旧工程保持原来的实际倍率。Both initially use 8x. TP temporarily uses 8x for a stored 1x choice and restores 1x when disabled. Older projects retain their actual rates.
- 同倍率 Core / Ceiling 在 0 ms 下可共享处理；总延迟以宿主报告为准。Matching Core / Ceiling rates can share processing at 0 ms; use the host's reported total latency.
- FULL / ECO 每个实例独立并随工程保存，独立于 A/B；自 1.2.41 起 ECO 还按宿主停播状态暂停 DSP，见上文。FULL / ECO is per instance, project-saved and independent of A/B. As of 1.2.41, ECO also suspends stopped-transport DSP as described above.
- 停播且全零输入、尾段稳定后休眠；播放、非零主输入或侧链在当前块恢复。After stopping, all-zero inputs and settled tails permit sleep; playback or nonzero main/sidechain input restores processing in the current block.

## 说明书与安装 / Manuals and installation

- [中文说明书：1.2.41，41 页](docs/manuals/QQ-Super-Compression-1.2.41-User-Manual-Chinese.pdf)
- [English manual: 1.2.41, 41 pages](docs/manuals/QQ-Super-Compression-1.2.41-User-Manual-English.pdf)
- [中文安装指南](docs/QQ-Super-Compression-1.2.41-INSTALL-CN.txt)
- [English installation guide](docs/QQ-Super-Compression-1.2.41-INSTALL-EN.txt)
- [当前文档状态 / Current document status](docs/CURRENT_DOCUMENT_STATUS.md)
- [双语更新记录 / Bilingual release notes](RELEASE_NOTES_1.2.41.md)

两本说明书章节和页码对应。旧版本说明书、SOURCE_MANIFEST、CANDIDATE_SOURCE、VERIFICATION 和 RELEASE_NOTES 为历史记录，不代表当前版本的验证或哈希。

Both manuals share the same chapter order and page numbers. Earlier manuals, SOURCE_MANIFEST, CANDIDATE_SOURCE, VERIFICATION and RELEASE_NOTES files are historical records, not evidence for the current build.

## 构建与验证 / Build and verification

Windows：VS 2022 C++ Build Tools、Windows SDK、CMake 3.22+。在 x64 Native Tools 环境运行 BUILD_WINDOWS.cmd / BUILD_WINDOWS.ps1。默认构建目录为 D:\Codex\Temp\QQSC1241-Build。可通过 -JucePath 指定 JUCE，或由脚本识别正式快照内 Dependencies/JUCE-8.0.15。

Windows requires VS 2022 C++ Build Tools, Windows SDK and CMake 3.22+. Run BUILD_WINDOWS.cmd / BUILD_WINDOWS.ps1 from an x64 Native Tools environment. The script accepts -JucePath or detects Dependencies/JUCE-8.0.15 in the formal snapshot. Builds do not install the plugin.

JUCE 固定为 8.0.15。跨平台构建由 .github/workflows 中的手动任务完成，目标为 Windows x64 VST3、Apple Silicon VST3、Intel VST3 和 Universal 2 AU；macOS 部署目标为 11.0。

JUCE is pinned to 8.0.15. Manual GitHub workflows target Windows x64 VST3, Apple Silicon VST3, Intel VST3 and Universal 2 AU, with macOS deployment target 11.0.

1.2.41 的 Windows 自动验证涵盖 Ceiling 压力、状态迁移与 A/B、16 种倍率组合、动态块长、ECO 停播/恢复和 Limiter / Unity 回归。持续输入与 1.2.40 对照的样本差为 0，延迟补偿一致；宿主听感与 ASIO-Guard 仍需在实际工程中判断，未声称新的 Cubase 实测。

Saved 1.2.41 Windows checks cover Ceiling stress, state migration and A/B, all 16 rate combinations, variable blocks, ECO stop/resume and Limiter / Unity regressions. Continuous output matches 1.2.40 sample-for-sample with identical PDC. Host listening and ASIO-Guard still depend on the actual session; no new Cubase measurement is claimed.

当前 Stable 为 1.2.41，见 STABLE_1.2.41.json。STABLE_1.2.36.json 记录早期撤回状态；旧版本验证仅作为历史保存。正式 Plan B/C/D 的执行证据分别在备份报告、GitHub 与最终交付包中核对。

1.2.41 is the current Stable version (STABLE_1.2.41.json). STABLE_1.2.36.json preserves the withdrawn historical promotion. Plan B/C/D completion must be verified from the backup report, GitHub and final delivery package respectively.

按用户最终选择，取消插件主输出的启停 Crossfade；播放、实时录制和离线导出均不叠加此类渐变。监听启停平滑交由 DAW。Existing A/B/parameter smoothing remains unchanged. No new transport crossfade or crossfade DSP overhead is added.
