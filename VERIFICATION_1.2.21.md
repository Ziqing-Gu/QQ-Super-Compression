# QQ Super Compression 1.2.21 — Verification Notes

## 基线

开发链：1.2.17 Stable / Plan B → 1.2.18 TP Recovery → 1.2.20 Display GR Restore → **1.2.21 Display Projection Cache**。

1.2.17 的 Plan B 备份未修改。

## 产品改动范围

本轮产品层主要修改：

- `Source/PluginProcessor.h/.cpp`：加入轻量 `displayProjectionRevision`；参数变化只增加 revision，不执行 GUI 工作。
- `Source/DynamicDisplay.h/.cpp`：Dirty Gate、停止状态去重、隐藏组件跳过渲染、4097 点 Dynamics transfer LUT。

核心 `StaticCompressionEngine.h` SHA-256 仍为：

```text
51B36D3AA7BE1113AA4E90AC6C0C534163A9B7A3A9EDC2DFE24D455CAD3FA6BA
```

## 1.2.20 Ratio 回归失败的原因

原 Windows 测试使用 Limiter Ratio `5 → 20`。Limiter DOWN Ratio 的产品有效范围为 `200 → 1000`，因此两次设置实际都映射为 200。该测试是无效刺激。

1.2.21 改用 `200 → 800`，仍复用同一批历史证据，不新增历史点；Windows `revision1221` 必须验证旧蓝线发生变化。

## 当前环境已执行

- `tests/revision1221_display_cache_audit.py`
  - Projection revision / dirty gate 存在；
  - 停止状态 counter 去重存在；
  - 隐藏组件 render bypass 存在；
  - 4097 点 transfer LUT 存在，且仅在 Dynamics signature 变化时重建；
  - 52,800+ 随机有限阈值 Classic/Super 插值探针中最大误差约 0.0047 dB；
  - `StaticCompressionEngine.h` 哈希保持稳定。
- `tests/revision1220_source_audit.py`：Display GR 契约继续通过。
- `tests/strict_one_to_one_link_isolated_test.py`：100,000 + 100,000 组严格 1:1 Link 通过。
- `tests/limiter_mode_continuity_isolated_test.py`：864 场景、100 次 Normal Single/Dual、252,021 boundary transfers 通过。
- `tests/tp_display_gr_performance_audit.py`：TP telemetry 每 sample 保持线性，dB 转换仍为每 host block 一次。
- `tests/true_peak_ceiling_target_source_audit.py`：TP Ceiling 主目标 / 16x reconstruction / 0.01 dB residual safety 通过。
- `tests/tp_recovery_source_audit.py`：TIGHT / AUTO / SMOOTH 参数与排序通过。

## Windows 实机验收

`BUILD_WINDOWS.cmd` 默认运行：

- `QQSCLimiterCheck revision1211`
- `QQSCLimiterCheck revision1218`
- `QQSCLimiterCheck revision1221`
- `QQSCLimiterCheck continuity`
- `QQSCLimiterCheck dual`
- `QQSCCeilingCheck`

重点验收：

1. 同一份旧历史在 Limiter Ratio 200 → 800 后必须重新变化；
2. Classic / Super、Threshold、Mix、TP / Recovery 继续 retrospective 更新；Ceiling 必须触发历史重投影，但因其同时平移 guard 输入与 target，Blue TP-GR 应保持不变，Orange 应随 Ceiling 平移；
3. TP OFF finite threshold 语义保持；
4. 多开数个可见插件窗口时观察 GUI 流畅度与 Cubase Message Thread；
5. 停止播放时 Display 不应继续无意义滚动/重建；
6. 关闭或隐藏插件窗口后，不应继续产生同等 Display GUI 开销。

本环境无法代替 Windows/Cubase 的多窗口实际性能观察，因此不把“不卡顿”提前标记为通过。


## Build Fix 1

首轮 Windows 集成测试在 Ceiling 断言处失败。复核实际 DSP 路径后确认该断言与产品语义冲突：`getActiveOutputGainDb()` 在 Limiter 模式包含 `limiterOutputDb + ceilingDb + limiterCalibrationDb`，同时 `OutputCeiling` 的 target 也使用同一个 `ceilingDb`。因此单独修改 Ceiling 不改变超限量/TP GR，只改变最终电平参考。

Build Fix 1 将 Windows 回归改为同时验证：

1. Ceiling 参数变化必须增加 `displayProjectionRevision`；
2. 同一历史重新投影后 Blue TP-GR 保持稳定；
3. Orange/final output 必须随 Ceiling 改变。

本 Build Fix 1 不修改产品 `Source/`。


## Build Fix 2 regression-harness correction

Build Fix 1 reached the real Windows/JUCE test executable but stopped at `Limiter toggle cleared retrospective Display history`. Investigation showed the synthetic test had populated private history before `DynamicDisplay::timerCallback()` had ever established its source trackers. The first timer tick after the toggle therefore performed normal first-tick mode/key/generation initialization and cleared the synthetic window.

Build Fix 2 primes one real Display timer tick first, with Normal and Limiter banks explicitly aligned to ST + Internal key, then seeds the fixed evidence. This isolates the actual contract: Limiter OFF/ON must not clear history when the evidence source is unchanged. Product `Source/` remains byte-identical to 1.2.21 / Build Fix 1.
