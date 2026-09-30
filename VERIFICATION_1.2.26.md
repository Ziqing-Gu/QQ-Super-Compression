# QQ Super Compression 1.2.26 — Verification Notes

## 基线

开发基线为 **1.2.25 Limiter Unity Ratio + Bidirectional Link Candidate**。1.2.17 Stable / Plan B 备份未修改。

## 产品代码改动

产品 `Source/` 只需要修改 `PluginEditor.h` 中的 `FineKnob::mouseDrag()` / drag anchor 状态。声音 DSP 不参与本轮修复。

Rotary drag 改为当前值的 normalized incremental movement：

```text
deltaPixels = horizontalDelta - verticalDelta   // H/V rotary
normalizedDelta = deltaPixels / sensitivity
newValue = proportionOfLengthToValue(
    clamp(valueToProportionOfLength(currentValue) + normalizedDelta))
```

普通 sensitivity 为 180，Shift 为 1200。因为每个 MouseEvent 都以上一事件为位置锚点、以当前参数值为 value 锚点，所以 modifier transition 不会重算历史累计位移。

## 设计约束

- 不通过 mouseUp/mouseDown 伪造 rebase，避免把一次物理 drag 拆成多个 host automation gesture / Undo transaction。
- 继续只在真实 mouseDown / mouseUp 开闭 Slider/APVTS gesture。
- 非线性参数通过 `valueToProportionOfLength()` / `proportionOfLengthToValue()` 保留现有 NormalisableRange。
- LinearVertical Threshold 已经采用逐事件 delta，不重新实现。
- Alt reset 与参数 rebind cancellation 保持原逻辑。

## 验收

`QQSCLimiterCheck revision1226` 使用真实 MouseEvent 序列验证：

- Normal → Shift transition at same coordinate: zero jump。
- Shift → Normal transition at same coordinate: zero jump。
- Fine movement slower than normal movement。
- Normal Ratio nonlinear mapping transition continuity。
- Limiter SINGLE Ratio after bank rebind transition continuity。
- Threshold LinearVertical transition continuity。

同时继续运行 `revision1211`、`revision1218`、`revision1221`、`revision1225`、`continuity`、`dual` 与 `QQSCCeilingCheck`，防止 LINK、TP、Display、Limiter Ratio 行为回归。
