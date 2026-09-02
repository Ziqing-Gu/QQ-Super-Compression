# QQ Super Compression v1.1.5 Plan C version coverage

- 上一次 GitHub Release / Previous GitHub Release: `v1.1.2`
- 本次目标 / Current target: `v1.1.5`
- 日期 / Date: `2026-09-02`

## 必须覆盖的真实版本 / Required real versions

1. `v1.1.3` - Sidechain Display History Replay
   - 中文：Key Gain 实时重投影完整可见历史；HPF 在松开旋钮后以低优先级重放原始 Key 历史。
   - English: Key Gain reprojects the complete visible history in real time; HPF replays raw Key history at low priority after knob release.
2. `v1.1.4` - Reliable/Faster HPF Display Replay
   - 中文：加入最新请求重试、缩短非鼠标 debounce、按当前域减少重放工作，并显示 `HPF UPDATING`。
   - English: Adds latest-request retries, shorter non-mouse debounce, domain-limited replay work, and an `HPF UPDATING` indicator.
3. `v1.1.5` - Fluid/Cached Dynamic Display Rendering
   - 中文：Display 提升到 60 Hz / 480 点，缓存投影与曲线路径，使用有界稀疏 GR 阴影和不透明子组件绘制，解决深度压缩时瞬间卡顿。
   - English: Advances the Display to 60 Hz / 480 points, caches projections and paths, and uses bounded sparse GR shading plus opaque child painting to remove deep-compression stalls.

## 核对 / Reconciliation

- README 与 CHANGELOG 保留 v1.1.3、v1.1.4、v1.1.5 的中英文变化说明。
- README and CHANGELOG retain bilingual change coverage for v1.1.3, v1.1.4, and v1.1.5.
- 本区间不存在其他真实发布或开发版本；遗漏版本为无。
- There are no other real release or development versions in this interval; omitted versions: none.
