# QQ Super Compression 1.2.24 — Limiter Ratio Alt Reset Candidate

基线：**1.2.23 Limiter Up Ratio Build Fix 2**。

## 修改

Limiter **SINGLE** 模式下，Ratio 声音参数的新实例默认值本来就是 `200:1`。本版让界面 Alt+单击复位与这个真实默认值一致：

- Limiter SINGLE Ratio：Alt+单击 → `200:1`
- Limiter DUAL UP：Alt+单击仍为 `1:1`
- Limiter DUAL DOWN：Alt+单击仍为 `200:1`
- Normal SINGLE：Alt+单击仍为 `1:1`

Ratio 的实际范围不变：Limiter SINGLE 上压区仍为 `1:8 → 1:1`、下压区仍为 `200:1 → 1000:1`；Limiter DUAL UP 仍为 `1:8 → 1:1`。

## 未改变

- Classic / Super 压缩公式
- Single / Dual DSP
- TP Ceiling / TP Recovery
- Display retrospective / projection cache
- Limiter Link
- Normal 模式 Ratio 范围

`StaticCompressionEngine.h` 未修改。
