# QQ Super Compression 1.2.25 — Verification Notes

## 基线

开发起点为 **1.2.24 Build Fix 1**。上游正式 Stable / Plan B 仍为 **1.2.17 Build Fix 1**，本候选没有修改冻结的 Plan B 备份。

## 产品源码改动范围

相对 1.2.24 Build Fix 1，`Source/` 中只有以下文件变化：

- `DynamicsLimits.h`
- `Parameters.h`
- `PluginProcessor.cpp`
- `PluginEditor.h`
- `PluginEditor.cpp`
- `LimiterEditor.cpp`

核心压缩引擎未修改。`StaticCompressionEngine.h` SHA-256：

```text
51B36D3AA7BE1113AA4E90AC6C0C534163A9B7A3A9EDC2DFE24D455CAD3FA6BA
```

## Ratio 契约

Limiter SINGLE 使用连续 unity-centered 映射：

```text
normalized 0.0 -> 1:8
normalized 0.5 -> 1:1
normalized 1.0 -> 1000:1
```

因此从上压到下压经过真正连续的 `1:1`，不再跳过普通 DOWN Ratio。

Limiter DUAL：

```text
UP   1:8 -> 1:1
DOWN 1:1 -> 1000:1
```

Limiter 新实例的 SINGLE / DUAL UP / DUAL DOWN Ratio 默认均为 `1:1`，Alt+单击也恢复 `1:1`。

## LINK 契约

Limiter LINK 对两个压缩方向一致：

```text
active UP Threshold   <-> Output Gain : inverse 1:1 dB
active DOWN Threshold <-> Output Gain : inverse 1:1 dB
Makeup                <-> Output Gain : inverse 1:1 dB
```

DUAL 两个分支同时活动时，Output 改变会让两个 Threshold 同量反向移动，因此两阈值间距保持不变。Ratio / Mix / algorithm 不参与 Output Link。

Ratio 恰好为 unity 的分支不产生动态增益，因此不作为 Output 的 Threshold companion；这样默认 `1:1` 下 Output 保持自由，同时从 UP 或 DOWN 进入实际压缩后使用同一套 Link 规则。

## 已执行的源码级检查

- `tests/strict_one_to_one_link_isolated_test.py`
  - 100,000 组 source→Output
  - 100,000 组 Output→Threshold
  - 上压 / 下压严格反向 1:1 clamp 检查通过
- `tests/limiter_mode_continuity_isolated_test.py`
  - 864 组 processor 场景
  - 100 次 Normal Single/Dual 切换
  - 252,021 次 boundary policy 检查通过
- `tests/tp_display_gr_performance_audit.py`：通过
- `tests/true_peak_ceiling_target_source_audit.py`：通过
- `tests/tp_recovery_source_audit.py`：通过
- `tests/revision1221_display_cache_audit.py`
  - 1.2.21 projection revision / dirty cache / hidden rendering / 4097-point Display LUT 保持通过
  - LUT 随机探针最大误差约 0.004645 dB
- `tests/revision1225_limiter_unity_link_audit.py`
  - 100,001 点连续映射 + 100,000 随机 round-trip
  - 最坏 round-trip 误差约 `1.59e-12`
  - 核心压缩引擎哈希保持不变

## Windows 实机验收

运行 `BUILD_WINDOWS.cmd`，默认构建目录：

```text
D:\Codex\Temp\QQSC1225-Build
```

默认执行：

- `QQSCLimiterCheck revision1211`
- `QQSCLimiterCheck revision1218`
- `QQSCLimiterCheck revision1221`
- `QQSCLimiterCheck revision1225`
- `QQSCLimiterCheck continuity`
- `QQSCLimiterCheck dual`
- `QQSCCeilingCheck`

`revision1225` 会继续运行 Ratio 参数范围 / legacy clamp / Alt-reset 检查，并额外验证：

1. SINGLE 上压 Threshold ↔ Output 严格反向 1:1；
2. SINGLE 下压 Threshold ↔ Output 严格反向 1:1；
3. unity 默认下 Output 不被无意义 Threshold 锁定；
4. DUAL UP / DOWN 两个方向各自都能参与 Threshold + Makeup + Output Link；
5. 两个 DUAL 分支同时活动时，Output 同步移动两阈值并保持窗口宽度；
6. LINK OFF 时不会发生上述耦合。

完整 Windows JUCE/MSVC 构建和 Cubase 实机使用仍需用户本机验证，当前版本仍为 Candidate。
