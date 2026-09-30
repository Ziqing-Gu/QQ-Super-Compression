# QQ Super Compression 1.2.20 — Verification Notes

## 基线

开发起点为 1.2.18 TP Recovery 源码；1.2.18 本身建立在用户已设为 Stable 并完成 Plan B 的 **1.2.17 Build Fix 1** 上。本候选没有修改 Plan B 备份。

## 产品源码改动范围

与 1.2.18 输入源码比较，`Source/` 中只有：

- `DynamicDisplay.cpp`
- `DynamicDisplay.h`

发生变化。`StaticCompressionEngine.h` SHA-256 保持：

```text
51B36D3AA7BE1113AA4E90AC6C0C534163A9B7A3A9EDC2DFE24D455CAD3FA6BA
```

因此本轮没有修改 Classic / Super 压缩公式。

## Display 契约

### 蓝线

```text
Blue = current Dynamics transfer + current Mix + current TP attenuation
```

基础 Dynamics / Mix 使用当前参数对历史 detector 证据重新计算。

TP OFF：蓝线不叠加 Sample-Peak Ceiling GR。

TP ON：在基础蓝线上叠加当前 retrospective TP envelope。

### 内部检测对齐

INT 的传输投影使用：

```text
transferInputDb = currentDetectorDb - currentInputGainDb
```

而不是把独立采集的 block carrier peak 与 lookahead detector peak直接组合。Threshold 线本来就使用同一 Display 参考（Threshold - Input Gain），因此有限 DOWN Threshold 下的 TP-OFF transfer 可直接校验。

EXT 由于 detector 与 carrier 独立，继续使用 captured carrier level。

### Orange

Orange 是当前参数重新投影的最终输出参考，包含 Makeup、Output Gain、Ceiling 与相应 TP / Sample-Peak stage。

## 当前环境已执行

- `tests/revision1220_source_audit.py`
  - 核对压缩引擎哈希未变化。
  - 100,000 组 Classic / Super 有限 Threshold + Mix 数学检查：TP OFF 蓝线不低于视觉 Threshold。
  - 核对蓝线定义为 Dynamics/Mix + TP，排除旧的蓝橙同轮廓公式。
  - 核对 Limiter ON/OFF 不清历史，Lookahead 改变会请求 detector replay。
- `tests/strict_one_to_one_link_isolated_test.py`
  - 100,000 + 100,000 组严格 1:1 Link 检查通过。
- `tests/limiter_mode_continuity_isolated_test.py`
  - 864 组场景、100 次 Normal Single/Dual、252,021 次 boundary policy 检查通过。
- `tests/tp_display_gr_performance_audit.py`
  - TP telemetry 仍保持每 sample 线性、每 host block 一次 dB 转换。
- `tests/true_peak_ceiling_target_source_audit.py`
  - TP Ceiling 主目标、16x reconstruction 与 0.01 dB residual safety 规则通过。
- `tests/tp_recovery_source_audit.py`
  - TIGHT / AUTO / SMOOTH 参数、UI/state hook 与 release 排序通过。

## Windows 实机验收

运行 `BUILD_WINDOWS.cmd`。默认将执行：

- `QQSCLimiterCheck revision1211`
- `QQSCLimiterCheck revision1218`
- `QQSCLimiterCheck revision1220`
- `QQSCLimiterCheck continuity`
- `QQSCLimiterCheck dual`
- `QQSCCeilingCheck`

`revision1220` 使用固定历史证据检查：

1. INT 模式同一 detector 但不同 captured carrier peak 不得产生假尖峰；
2. TP OFF、有限 DOWN Threshold 时蓝线不得越过视觉 Threshold；
3. Classic / Super、Ratio、Threshold、Mix 修改必须重新投影旧历史；
4. TP ON 必须在同一基础压缩结果上增加 TP 衰减，并允许低于 Threshold；
5. TP ON 时 Output 改变可通过 TP 驱动量重新影响旧蓝线；
6. Limiter ON / OFF 不得清除已有同源历史。

完整 Windows JUCE/MSVC 编译、Cubase GUI 与听感仍需用户本机验证。
