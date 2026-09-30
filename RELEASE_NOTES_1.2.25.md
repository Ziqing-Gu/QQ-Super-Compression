# QQ Super Compression 1.2.25 — Limiter Unity Ratio + Bidirectional Link Candidate

基线：**1.2.24 Build Fix 1**。

## 设计调整

Limiter 模式不再把 QQ Super Compression 前级强制限制在极高 DOWN Ratio。Ceiling / TP 本身承担最终 limiting，因此前级压缩器可以从 unity 开始，用 Classic / Super、Ratio、Mix 等塑造进入 Ceiling 之前的动态性格。

### Limiter SINGLE

- 上压范围：`1:8 → 1:1`
- 下压范围：`1:1 → 1000:1`
- `1:1` 是连续范围的正中点，不再存在旧的 `1:1 → 200:1` 空档
- 新实例默认：`1:1`
- Alt+单击：恢复 `1:1`

### Limiter DUAL

- UP Ratio：`1:8 → 1:1`
- DOWN Ratio：`1:1 → 1000:1`
- 新实例 UP / DOWN 默认均为 `1:1`
- Alt+单击 UP / DOWN 均恢复 `1:1`

Normal 模式 Ratio 范围和默认值不变。

## Limiter LINK

Limiter LINK 现在对上压和下压使用一致的严格 1:1 dB 关系：

- 活动 UP Threshold 改变 ΔdB → Output Gain 反向改变同样 ΔdB
- 活动 DOWN Threshold 改变 ΔdB → Output Gain 反向改变同样 ΔdB
- Makeup 改变 ΔdB → Output Gain 反向改变同样 ΔdB
- 编辑 Output Gain → 当前活动的 Threshold 反向移动同样 ΔdB
- DUAL 的 UP / DOWN 同时活动时，Output 会同时移动两个 Threshold，保持两阈值之间的窗口宽度
- Ratio、Mix、Classic / Super、UP/DOWN 算法选择本身仍然不会推动 Output

Ratio 恰好为 `1:1` 时该分支没有动态处理，因此不会为了 LINK 人为制造 Threshold 位移；一旦进入上压或下压，该方向使用完全相同的 Link 规则。

## 状态兼容

旧工程中的 Limiter Ratio 会按新范围恢复：

- SINGLE 小于 `1:8` 的上压值夹到 `1:8`
- SINGLE `1:1 → 1000:1` 的 DOWN Ratio 现在全部合法，不再强制夹到 `200:1`
- DUAL UP 小于 `1:8` 夹到 `1:8`
- DUAL DOWN `1:1 → 1000:1` 全范围合法

## 未改变

- Classic / Super 压缩公式
- Single / Dual 音频 DSP
- Ceiling / True-Peak limiting
- TP Recovery TIGHT / AUTO / SMOOTH
- Dynamic Display retrospective 语义与 1.2.21 projection cache 优化
- Normal 模式 Ratio 行为

`StaticCompressionEngine.h` 保持既有基线。
