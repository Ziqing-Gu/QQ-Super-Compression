# QQ Super Compression 1.2.15 — Verification Notes

## Intended contract

1.2.15 只优化 TP/Sample-Peak 可视 GR 遥测的计算位置，不改变 1.2.14 的显示结果或任何音频 DSP。

## Local checks in this package

- `tests/tp_display_gr_source_audit.py`：确认 Display / GAIN +/- / Hold 仍共享 OutputCeiling GR，并确认 audible Ceiling 方程未改变。
- `tests/tp_display_gr_performance_audit.py`：确认 `OutputCeiling::process()` 内没有 gain-to-dB/log；实时处理使用线性 gain；dB 转换位于 host sample loop 之后。
- 100,000 个随机 block 数学验证：`max(per-sample GR dB)` 与 `GR dB(min linear gain)` 一致。
- `tests/true_peak_ceiling_target_source_audit.py`：1.2.12 的 TP Ceiling target/residual 规则继续通过。
- `tests/strict_one_to_one_link_isolated_test.py`：严格 1:1 Link 100,000 + 100,000 随机检查通过。
- `tests/limiter_mode_continuity_isolated_test.py --sanitize`：864 场景、100 次 Normal Single/Dual、252,021 边界策略检查通过。

## Expected runtime reduction

用于本轮新增显示遥测的 gain-to-dB 调用从近似 `sampleRate` 次/秒/实例降为 `sampleRate / blockSize` 次/秒/实例。例如 48 kHz / 256 samples：约 48,000 -> 187.5 次/秒；减少 256 倍。

这不代表整插件 CPU 降低 256 倍；TP 的主要成本仍是既有 8x true-peak limiter 与 16x reconstruction analysis。本轮只移除显示遥测中不必要的逐 sample log。

## Not run here

完整 Windows JUCE/MSVC VST3 构建、真实 Cubase 多实例 CPU/卡顿观察仍需用户本机运行。`BUILD_WINDOWS.cmd` 会运行 `revision1211`、`continuity`、`dual` 和 `QQSCCeilingCheck`。
