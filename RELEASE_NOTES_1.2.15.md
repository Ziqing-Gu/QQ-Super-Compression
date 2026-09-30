# QQ Super Compression 1.2.15 — TP Visual Telemetry Performance

- Display / GAIN +/- / Hold 的 TP/Sample-Peak GR 语义保持 1.2.14 不变。
- `OutputCeiling::process()` 不再逐 sample 执行 gain-to-dB/log 转换。
- Audio thread 每个 sample 只保留线性 display gain；`PluginProcessor` 每个 host block 累计最小 gain，并在 block 结束后只转一次 dB。
- `gainReductionDbForDisplay()` 保留给测试/按需读取；实时处理路径改用 `gainForDisplayLinear()`。
- 没有新增 oversampling、FFT、分配或锁；音频 DSP 与 Ceiling 输出方程未改。
- 版本：1.2.15；默认 Windows 构建目录 `D:\Codex\Temp\QQSC1215-Build`。
