# QQ Super Compression 1.2.42 Stable / 稳定版

## 中文

FULL 在宿主明确停播时继续支持实时监听。针对某些宿主持续送入约 −200 dBFS 的极低残留，1.2.42 增加保守休眠判断：主输入与已选侧链不高于 −180 dBFS，考虑当前增益后至少一秒实际输出低于 −160 dBFS，且滤波、延迟与 Ceiling 恢复尾音已经稳定。满足条件才暂停主要 DSP，并将休眠期间的极低残留归零。普通硬件底噪和实时输入继续处理；极端增益可能让 FULL 保持工作。

播放、录音、离线处理、宿主状态未知、实时输入、已选侧链活动或参数变化均在当前音频块保持或恢复处理。ECO 沿用 1.2.41 的明确停播即停止处理规则。插件主输出没有新增启停 Crossfade，报告延迟不变。

Windows 构建、回归检查、30 项实际 VST3 极低残留场景、连续活动输入新旧样本与 PDC 对照通过。独立 CPU 耗时不能换算为 Cubase ASIO-Guard 百分比；未声称提供了新的量化 Cubase 报告。用户在已安装候选版后明确将 1.2.42 提升为 Stable。中英文说明书均更新为 41 页。

## English

FULL keeps live monitoring available when the host is known to be stopped. Version 1.2.42 adds a conservative sleep path for host residuals around −200 dBFS. Raw main and selected-sidechain peaks must be at most −180 dBFS, gain-adjusted actual output must remain below −160 dBFS for at least one second, and filter, delay and Ceiling recovery tails must settle. Only then does major DSP sleep, with sub-floor residuals becoming zero during sleep. Ordinary hardware noise and live input remain processed; extreme gain may keep FULL awake.

Playback, recording, offline rendering, unknown transport, live input, selected-sidechain activity and parameter changes keep or resume processing in the current block. ECO retains the 1.2.41 known-Stop suspension policy. No new main-output Stop/Start crossfade was added, and reported latency is unchanged.

Windows build and regressions, 30 actual-VST3 low-residual scenarios, active-input sample parity and PDC checks passed. Standalone CPU time is not a Cubase ASIO-Guard percentage; no new quantified Cubase report is claimed. The user explicitly promoted the installed 1.2.42 candidate to Stable. Both updated manuals have 41 pages.

License: Qing Audio Non-Commercial Source-Share License 1.0. Commercial use is prohibited; redistribution requires complete corresponding source and preserved attribution and license notices. See [LICENSE](LICENSE) and [LICENSE_POLICY_CHANGE.md](LICENSE_POLICY_CHANGE.md).
