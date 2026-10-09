# QQ Super Compression 1.3.2 Stable

Release date / 发布日期: 2026-10-09

Changes since the previous public release 1.2.42 / 自上次公开版本 1.2.42 的变化。Covered versions / 覆盖版本: 1.3.0, 1.3.1, 1.3.2. Omitted versions / 遗漏版本: none / 无。

### 1.3.2 — Stable, 2026-10-09

- 中文：检测切换统一为 SAFE 按钮，位于 Limiter 右侧。新实例默认关闭；点亮为黄色 Safe，关闭为 Normal。Normal 显示 Distort 百分比，Safe 显示 Window 毫秒；切换保持实际窗口长度。更新配套中英文 43 页手册。DSP 沿用已验证的 1.3.1 后续方案。
- English: A fixed SAFE button sits to the right of Limiter, off by default in new instances. Lit yellow means Safe; unlit means Normal. Normal shows Distort percent, Safe shows Window in milliseconds; switching retains the actual window. Includes matching 43-page Chinese and English manuals. DSP retains the verified final 1.3.1 design.

### 1.3.1 — Development series consolidated into 1.3.2

- 中文：在固定 Lookahead 基础延迟内加入可调检测窗口。Normal 的 Distort 0% 对应全窗口，100% 对应零窗口；Safe 的 Window 为 0 至当前 Lookahead。手动切换 Lookahead 和 Alt 单击恢复全窗口，工程及 A/B 保留已存设置。过采样统一为 1x/4x/8x/16x，覆盖所有 Lookahead 的动态与 Ceiling；保留 1x 音频下的独立 TP 重建检测。移除 10 ms 档，旧 10/5 ms 状态迁移至 26 ms，最终档位为 0/26/40/80/100 ms。Limiter 上压阈值不再参与 Output/Makeup 联动，Super 最低 -inf、Classic 最低 -90 dB；下压和 Makeup 联动保留。修正 0 ms、Lookahead 切换及 A/B 的 Display 对齐。
- English: Adds an adjustable detector window within the fixed Lookahead base delay. Normal Distort maps 0% to the full window and 100% to zero; Safe Window spans zero to the current Lookahead. Manual Lookahead changes and Alt-click restore the full window; projects and A/B retain saved settings. Unified 1x/4x/8x/16x audio oversampling covers dynamics and Ceiling at every Lookahead, including independent TP reconstruction at native 1x audio. Removes the 10 ms preset; old 10/5 ms states migrate to 26 ms. Final presets are 0/26/40/80/100 ms. Limiter UP Threshold no longer links to Output/Makeup; its Super floor is -inf and Classic floor is -90 dB. DOWN and Makeup linking remain. Corrects Display alignment at 0 ms and across Lookahead/A/B changes.

### 1.3.0 — Detector investigation and comparison candidate

- 中文：针对带 Delay/混响素材的持续噼啪声，引入前后窗口峰值取较小值与取较大值的对比方案，并排查 1:1 Display 参考对齐。较大峰值方案以提前压及峰后压为代价，帮助抑制快速增益变化；相关候选方案在 1.3.1 中进一步整理。本系列未采用隐藏 Attack/Release 作为动态修复。
- English: Investigates persistent crackles on delay/reverb material with a comparison between the smaller and larger of past/future window peaks, alongside unity-ratio Display reference alignment. The larger-peak approach trades pre/post-attenuation for reduced rapid gain changes; the candidate designs are consolidated in 1.3.1. No hidden Attack/Release stage was introduced as the dynamics fix.

## Behavior and upgrade notes / 行为与升级

SAFE is optional and defaults off. A longer Safe window can suppress rapid-gain crackles but attenuate audio before and after a loud event. It cannot repair already clipped input or host dropouts; zero window makes both detectors identical. Distort is a control amount, not a measured THD percentage. Overall OS reduces aliasing, not all nonlinear coloration. Total reported latency includes any oversampling/Ceiling latency in addition to Lookahead.

SAFE 默认关闭。较长 Safe 窗口有助于抑制快速增益变化造成的噼啪声，代价是响声前后的提前压与峰后压；它不能修复已削波输入或宿主掉帧。零窗口时两种检测相同。Distort 是控制量，不是实测 THD 百分比。总过采样减少混叠，不消除所有染色。宿主总延迟还计入过采样与 Ceiling 的延迟。

Older sessions can change tone/CPU load because a stored OS choice now also processes nonzero Lookahead. Check OS, SAFE, Distort/Window, automation and A/B after migration. Save a session copy before upgrading. FULL/ECO behavior from 1.2.42 is retained: ECO stops processing on known Stop; FULL retains live monitoring and conservative low-residual sleep. Playback, recording, offline processing and unknown transport remain processed. No main-output transport crossfade was added.

旧工程的过采样选择现在也作用于非零 Lookahead，音色与负载可能变化；升级前保存工程副本，升级后核对 OS、SAFE、Distort/Window、自动化与 A/B。沿用 1.2.42 的 FULL/ECO：ECO 停播即停止处理；FULL 保留实时监听和保守的极低残留休眠。播放、录音、离线处理及未知宿主状态继续处理，无主输出启停 Crossfade。

## Reproduction and validation / 复现与验证

JUCE 8.0.15; CMake 3.22+; Windows VS 2022 x64, macOS deployment target 11.0. Windows delivery reuses the previously verified 1.3.2 binary; production Source files and CMake hashes are matched to that build. The macOS workflow builds arm64/x86_64 VST3 and Universal 2 AU from this source, runs current detector/window/oversampling/Display/transport checks and AU validation. Completion is confirmed by the workflow run and release evidence, not by this source document alone. Listening in every host/material combination is not implied.

Windows 复用此前验证的 1.3.2 成品，逐项核对生产 Source 与 CMake 源码身份。macOS 从同一源码构建三类成品，执行当前检测、窗口、过采样、Display、启停检查及 AU 验证；实际完成状态以 workflow 与发布证据为准，本文本身不表示构建已通过。未宣称对所有宿主和素材进行听感验证。
