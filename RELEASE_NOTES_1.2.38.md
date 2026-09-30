# 1.2.38 Performance Candidate / 性能候选版

## 范围与状态 / Scope and status

1.2.37 的独立测试通过，但用户在 Cubase 中仍报告停播静音负载偏高。它没有通过用户的实际宿主验收。1.2.38 的代码优化与诊断不构成 Cubase 问题已解决的声明。保持 Candidate；Plan B/C/D 和发布停止。

1.2.37 passed standalone checks but the user reported no stopped-silence improvement in Cubase. Those checks did not establish real-host acceptance. 1.2.38 remains a candidate; optimization and instrumentation are not a claim that Cubase is fixed. Release work stays stopped.

## 修改 / Changes

- 0 ms 检测器直接返回当前样本的幅值，省去无用的峰值队列维护；音频线程仅获取检测电平，省去丢弃的 unity gain/GR 转换。非零前瞻保持双峰窗算法。
- ECO 且窗口隐藏时，省去当前模式不使用的增益曲线。所有检测器、参数平滑器继续推进；每块尾部恢复全部模式计算以维护下采样滤波器历史，支持下一块切换模式、A/B 和打开窗口。小块不足以安全裁剪时保留完整计算。
- A/B 各银行只计算其 ST/LR/MS 矩阵实际使用的增益。
- ECO 关窗时暂停 Ceiling 过采样内部的纯显示峰值、增益衰减统计与缓存；声音保护仍执行原有检测。
- FULL/ECO 按钮提示保留各模式最近一次开窗/关窗期间的平均处理时间、块大小、实际倍率、全零输入、休眠、播放和未知传输状态比例。仅原子计数与计时，不在音频线程写文件、分配内存或加锁。

- Zero-lookahead detection uses the current sample magnitude directly. The audio processor requests detector levels without discarded unity-gain/GR conversions. Nonzero lookahead retains its two-window algorithm.
- Hidden ECO skips gain curves for unused modes while advancing all detectors and smoothers. Complete gain calculation resumes at each block tail to retain downsampler state for mode/A-B/editor transitions. Small blocks keep full calculation when needed for safety.
- Each A/B bank evaluates only the gain domains consumed by its matrix.
- Hidden ECO also skips Ceiling display-only peak/gain statistics and caches; audible safety detection continues unchanged.
- The FULL/ECO tooltip retains timing and processing-state statistics for each editor/performance phase. Audio callbacks use counters and timestamps without file I/O, allocation or locks.

过采样倍率、声音曲线、Ceiling 保护与 PDC 保持原设置。Ceiling 音频保护本身仍需要处理；本次不以降低保护质量换取省 CPU。
Oversampling factors, transfer curves, Ceiling protection and PDC retain their existing settings. Audible Ceiling protection still requires processing.

## Cubase 验证 / Cubase check

同一段音频与设置下，分别选 FULL 和 ECO，关闭窗口播放一段时间；重新打开后，把鼠标移到 FULL/ECO 按钮可查看保存的 `FULL closed` / `ECO closed` 数据。停止播放后的检查请另做一次关窗/开窗，以得到独立的静音区间。`Last idle gate` 显示最后一块的休眠状态或阻塞原因（宿主播放、无传输信息、参数变化、非零输入、A/B、尾音等）。`sleep` 为实际进入休眠的块比例；`exact zero` 与 `playing` 来自插件收到的输入和宿主状态。

Compare the same audio/settings with the editor closed in FULL and ECO. Reopen and hover FULL/ECO to read the retained closed-editor measurements. For stopped silence, close/reopen separately to collect a distinct idle interval. Sleep is the fraction of callbacks entering the sleep path; exact zero and playing reflect the inputs and transport actually received by the plugin.

这些是插件回调耗时，不能换算或当作 Cubase ASIO-Guard 百分比。测试证据见 `Verification/1.2.38-Windows`。
These are plugin callback timings, not Cubase ASIO-Guard percentages. Evidence is retained in `Verification/1.2.38-Windows`.
