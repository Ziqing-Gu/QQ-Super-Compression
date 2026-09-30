# QQ Super Compression 1.2.35

## 中文

- FULL/ECO 改为每实例独立设置，随工程保存；新实例与缺少该字段的旧状态默认 FULL，A/B 不覆盖该选择。
- 停止播放且全部输入为零时，等延迟缓冲与 TP 恢复完成后休眠过采样核心与 Ceiling；播放、输入、侧链和参数变化触发恢复。原有 DSP 状态、音频算法与 PDC 保留。
- 保留前轮缓冲容量、mono MS、A/B Ceiling OS、MATCH、表头与显示修复。
- Windows Release、本轮选定音频回归、VST3 加载和安装哈希检查通过；测得的停播/恢复对照最大音频差为 0。Cubase 连续快速启停、实际离线导出与 ASIO-Guard 尚待宿主实测。
- 48 kHz / 256 样本的 16x Limiter 静音 DSP 对照约降低 95%；该比例不等同于宿主 ASIO-Guard 百分比。
- 随附说明书仍为 1.2.7，现已准备更新，尚未生成 1.2.35 手册。

## English

- FULL/ECO is now independent for each instance and saved with the project. New instances and legacy states without this property use FULL; A/B does not recall the performance preference.
- With transport stopped and all inputs exactly zero, the oversampled core and Ceiling can sleep after delays drain and TP recovery settles. Playback, input, sidechain or parameter changes resume processing. Existing DSP state, audio algorithms and reported latency are retained.
- Previous buffer-capacity, mono M/S, A/B Ceiling OS, MATCH and metering/display fixes are retained.
- The Windows Release build, selected audio regressions, VST3 loading and installed-file hash checks passed. Tested stop/resume output matched the continuously processed reference exactly. Rapid repeated transport changes, offline export and ASIO-Guard still require Cubase testing.
- The local 48 kHz / 256-sample silent 16x Limiter benchmark reduced DSP time by about 95%; this is not an ASIO-Guard percentage measurement.
- Bundled manuals remain at 1.2.7. Preparation for a documentation update is underway; no 1.2.35 manual has been produced.