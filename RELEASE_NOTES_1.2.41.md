# QQ Super Compression 1.2.41 Stable / 稳定版

## 中文

ECO 现在依据宿主的播放状态工作：宿主明确停播，且未录音、未离线渲染时，当前实例停止动态核心、过采样与 Ceiling 处理，并输出静音。硬件底噪、侧链活动和界面开关都不会阻止停播暂停；当时的延迟和 TP 尾音会被截断。需要停播时监听实时输入，请将该实例设为 FULL。FULL 沿用全零输入且尾音稳定后才休眠的规则。

播放、录音、离线渲染、宿主状态未知或切回 FULL 时，从当前音频块恢复完整处理。恢复前清除停播前的音频与滤波历史，采用当前参数，并保持宿主报告的延迟补偿。插件主输出没有新增启停 Crossfade；监听平滑交由 DAW。FULL/ECO 每个实例独立，工程保存的状态优先，新实例沿用上次手动选择。

隐藏 ECO 编辑器仍可减少界面分析负荷；播放中的 Core OS 与 CEILING OS 倍率及声音处理不因此降低。本版将部分 DSP 环形索引的整数取余改为等价条件回绕，不改音频公式、过采样品质或峰值保护规则。

Windows 1.2.41 自动验证、16 种 Core/Ceiling 组合的连续输入对照、停播/恢复及状态迁移测试通过；持续输入与 1.2.40 的样本差为 0，PDC 一致。见 `Verification/1.2.41-Windows/validation-report.json`。独立测得的每块 CPU 时间不能换算为 Cubase ASIO-Guard 百分比；本记录不声称 1.2.41 已经过用户 Cubase 工程实测。

中英文 41 页说明书已按上述行为修订并经用户确认。Threshold 在中文版统一称为“阈值”。许可仍为 Qing Audio Non-Commercial Source-Share License 1.0；禁止商业使用，分发需提供完整对应源码并保留许可证及来源声明。

## English

ECO now follows the host transport. On a known Stop without recording or offline rendering, the instance stops its dynamics core, oversampling and Ceiling work and outputs silence. Hardware noise, sidechain activity and editor visibility do not prevent suspension; pending delay and TP tails are cut. Choose FULL for live input monitoring while stopped. FULL retains its exact-zero, tail-settled sleep policy.

Full processing resumes in the current block when playback or recording starts, during offline rendering, with unknown transport state, or when switching to FULL. Resume clears pre-stop audio/filter history, applies current settings and retains reported latency. No new Stop/Start crossfade is applied to the plugin main output; monitoring fades are left to the DAW. FULL/ECO remains per instance, saved project state takes precedence, and new instances recall the last manual selection.

Closing an ECO editor still reduces analysis work; during playback it does not lower Core OS or CEILING OS or change the sound. Equivalent conditional wrap replaces integer remainder in selected DSP ring indexes without changing the audio formula, oversampling quality or peak-protection law.

Windows 1.2.41 automated checks, 16 Core/Ceiling configurations under continuous input, transport stop/resume and state migration passed. Continuous output matches 1.2.40 sample-for-sample with identical PDC. Evidence is in `Verification/1.2.41-Windows/validation-report.json`. Standalone CPU time is not a Cubase ASIO-Guard percentage; this record does not claim a new Cubase session measurement.

Both 41-page manuals were revised and approved by the user. The Qing Audio Non-Commercial Source-Share License 1.0 remains in force; commercial use is prohibited and distribution requires complete corresponding source and preserved license/source notices.
