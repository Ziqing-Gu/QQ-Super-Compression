# QQ Super Compression 1.2.23 — Limiter Up Ratio Candidate

## 变更

Limiter 模式的上压 Ratio 有效范围统一收窄到 `1:8 → 1:1`。

- SINGLE：Ratio 的上压半区由 `1:200 → 1:1` 改为 `1:8 → 1:1`；下压半区仍为 `200:1 → 1000:1`。
- DUAL：UP Ratio 保持 1.2.22 已确认的 `1:8 → 1:1`；DOWN Ratio 仍为 `200:1 → 1000:1`。
- Normal 模式不变：SINGLE / DUAL 上压侧仍可到 `1:200`。

旧项目或 A/B 状态若保存了 Limiter 上压小于 `1:8` 的 Ratio，将按新参数范围夹到 `1:8`。

## 未改变

- Classic / Super 传输公式。
- Single / Dual 音频处理架构。
- TP Ceiling 与 TIGHT / AUTO / SMOOTH Recovery。
- Limiter 严格 1:1 Threshold / Makeup / Output Link。
- Dynamic Display retrospective projection 与 1.2.21 的 Display cache 优化。
- Normal 模式 Ratio 范围。

`StaticCompressionEngine.h` SHA-256 仍为：

```text
51B36D3AA7BE1113AA4E90AC6C0C534163A9B7A3A9EDC2DFE24D455CAD3FA6BA
```
