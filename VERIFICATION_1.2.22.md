# QQ Super Compression 1.2.22 — Verification Notes

## 目标

只把 Limiter + DUAL 的 UP RATIO 范围改为 `1:8 → 1:1`，避免影响 Normal、Limiter SINGLE 或核心压缩算法。

## 产品源码改动

相对 1.2.21 Build Fix 2，产品 `Source/` 只涉及控制范围 / UI：

- `DynamicsLimits.h`
- `Parameters.h`
- `PluginProcessor.cpp`
- `LimiterEditor.cpp`
- `PluginEditor.cpp`

`StaticCompressionEngine.h` SHA-256 必须继续保持：

```text
51B36D3AA7BE1113AA4E90AC6C0C534163A9B7A3A9EDC2DFE24D455CAD3FA6BA
```

## 已执行的源码 / 隔离检查

- `revision1222_limiter_dual_up_ratio_audit.py`
  - Limiter DUAL UP minimum = `1/8`。
  - Limiter SINGLE upward minimum 仍为 `1/200`。
  - Normal DUAL upward minimum 仍为 `1/200`。
  - `StaticCompressionEngine.h` 未变化。
- `revision1221_display_cache_audit.py`：Display Projection Cache 与 4097 点 LUT 规则保持。
- `strict_one_to_one_link_isolated_test.py`：100,000 + 100,000 组严格 1:1 Limiter Link 通过。
- `limiter_mode_continuity_isolated_test.py`：864 场景、100 次 Normal Single/Dual、252,021 次 boundary policy 通过。
- `tp_display_gr_performance_audit.py`：TP telemetry 性能规则通过。
- `true_peak_ceiling_target_source_audit.py`：TP Ceiling / 16x reconstruction / 0.01 dB residual safety 通过。
- `tp_recovery_source_audit.py`：TIGHT / AUTO / SMOOTH 保持。

## Windows / JUCE 验收

`BUILD_WINDOWS.cmd` 将新增运行：

```text
QQSCLimiterCheck revision1222
```

实际检查：

1. `limiterUpRatio*` 五个宿主参数范围为 `0.125 → 1.0`；
2. 旧状态 `< 0.125` 会夹到 `0.125`；
3. Limiter DUAL UI 旋钮范围为 `1:8 → 1:1`，数值输入不能越界；
4. Limiter SINGLE 仍保持原完整 Ratio 范围；
5. Normal DUAL UP 仍保持 `1:200 → 1:1`；
6. 原 Ratio LINK 规则在新范围内继续工作。

完整 Windows VST3 编译仍需用户本机执行。
