# QQ Super Compression 1.2.22 — Limiter Dual Up Ratio Candidate

## 改动

Limiter 模式、DUAL 压缩时，UP RATIO 的有效范围由：

```text
1:200 → 1:1
```

调整为：

```text
1:8 → 1:1
```

这一范围同时应用于 ST / L / R / M / S 五个域、旋钮拖动、数值输入、宿主参数范围、工程恢复和 A/B 状态恢复。

## 保持不变

- Normal DUAL UP RATIO 仍为 `1:200 → 1:1`。
- Limiter SINGLE Ratio 的向上半区仍为 `1:200 → 1:1`。
- Limiter DUAL DOWN RATIO 仍为 `200:1 → 1000:1`。
- Ratio LINK 仍保留原来的 `UP × DOWN` 相对联动规则；LINK 开启时，可达范围仍会同时受另一侧 Ratio 当前值及其上下限约束。
- `StaticCompressionEngine.h` 未修改，Classic / Super 声音公式不变。
- TP Ceiling、TIGHT/AUTO/SMOOTH Recovery、Display retrospective projection / cache 优化均未修改。

## 旧状态迁移

旧工程、预置或 A/B bank 中，如果 Limiter DUAL UP RATIO 保存值低于 `1:8`，载入本版时自动夹到 `1:8`。其余 Ratio bank 不受影响。
