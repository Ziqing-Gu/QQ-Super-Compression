# QQ Super Compression 1.2.28 — Limiter Ceiling Oversampling Candidate

## 目标

修正 1.2.27 中 `OutputCeiling` 固定常驻造成的额外 PDC，并把 Limiter 的非 TP Ceiling 改成 8x Hard Clipper，同时保证 TP ON/OFF 不触发 oversampling/PDC 重建；Dynamic Display 同步采用新的 Ceiling 语义。

## 修改

### Normal latency

`getCombinedLatencySamples()` 仅在 Limiter ON 时加入 `OutputCeiling::latencySamples()`。`processBlockInternal()` 在 Normal 模式完全不调用 OutputCeiling 音频处理，也不推进其 reference delay。

因此 Normal 模式：

- 非 0 ms Lookahead：总 PDC = Lookahead；
- 0 ms / 1x：0 samples；
- 0 ms / 8x 或 16x：只包含用户主动选择的 compressor oversampling FIR latency；
- Limiter TP preference 不影响 Normal PDC。

### Limiter Ceiling

Limiter ON 后启用固定 8x Ceiling pipeline。

- TP OFF：8x Hard Clip。
- TP ON：保留 1.2.27 TP protection / Recovery / 16x residual reconstruction guard。
- 两个 endpoint 共用同一 8x up/down filter。
- Hard branch 通过第一段 8x delay + 第二段 host-rate delay 与 TP 两个 lookahead 阶段对齐。
- TP toggle 继续使用 10 ms smooth blend，但不 reset / prepare / allocate，也不改变 latency。

### Display

Retrospective projection 更新为：

- Limiter OFF：无 Ceiling contribution；
- Limiter + TP OFF：基于已捕获的 reconstructed-peak evidence 计算 8x Hard Clip depth；
- Limiter + TP ON：同一 evidence 进入 TP recovery replay。

Blue 现在在 TP OFF 时也包含实际 Hard Clip attenuation；Orange 继续包含最终 Ceiling 输出。

## 未修改

- StaticCompressionEngine / Classic / Super transfer law；
- Single / Dual、Ratio、Threshold、Range、Mix、Makeup；
- 1.2.27 Display audio-time x-axis、8 秒窗口与 bulk 4097 LUT；
- 1.2.26 Shift Drag；
- TP Recovery 三种模式本身。
