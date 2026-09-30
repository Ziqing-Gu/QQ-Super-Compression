# QQ Super Compression 1.2.21 — Display Projection Cache Candidate

基线：**1.2.17 Stable / Plan B**，沿用 1.2.18 TP Recovery，并保留 1.2.20 已确认的 Display GR 语义。

本版只继续完善 Dynamic Display 的刷新机制与多实例 GUI 性能，不修改 Classic / Super 核心压缩 DSP、Limiter Link、TP Ceiling 或 TP Recovery 声音算法。

## Display retrospective 规则保持不变

蓝色 `Cut / Mix` 仍按当前参数重新解释历史：

```text
历史 detector / carrier 证据
→ 当前 Classic / Super 曲线
→ 当前 Mix
→ 当前 TP 影响（TP ON）
→ 蓝色 Cut / Mix
```

TP OFF 时，内部检测且有限 DOWN Threshold 的基础压缩 / Mix 结果不会被画到 Threshold 以下；TP ON 时，只有 TP 的额外衰减可以把蓝线继续压低。

修改 Ratio、Threshold / Range、Classic / Super、Single / Dual、Mix、TP、TP Recovery、Input、Makeup、Output、Ceiling、Key / HPF、Lookahead 等当前参数，旧历史仍必须重新投影。

## 修正 Ratio Windows 回归的假失败

1.2.20 的 Windows `revision1220` 测试曾用 `Limiter Ratio 5:1 → 20:1` 判断历史是否变化。但 Limiter DOWN Ratio 的有效下限本来就是 **200:1**，所以 5:1 和 20:1 都会映射为 200:1；该失败不能证明产品历史重投影失效。

1.2.21 将测试改为真实有效的 **200:1 → 800:1**，并新增统一的 Display projection revision 机制，任何宿主/UI 参数变化都会让 retrospective projection cache 失效。

## 多实例 Display 优化

### 1. Projection Revision / Dirty Gate

处理器维护一个轻量的 `displayProjectionRevision`。参数没有变化、历史没有新增、几何没有变化时，Display 不再无条件重算整段历史和 Path。

### 2. 停止状态不再重复塞入相同历史点

Display 以音频历史 counter 为依据；宿主停止、counter 不变时，不再 60 Hz 重复写入同一个 HistoryPoint，也不再为了同一帧反复 repaint。

### 3. 隐藏 Display 不做不可见的路径重建

组件不可见时仍保留历史证据捕获，但暂停 retrospective projection / Path rebuild / repaint。再次显示时一次性根据 revision 重建。

### 4. Classic / Super Transfer LUT

每个可见域维护 4097 点 detector-level transfer LUT。LUT 只在真正影响压缩曲线的参数（Ratio、Threshold、算法、Single/Dual、启用状态、Limiter bank 等）变化时重建。

稳定播放时，8 秒历史的每个点只做 LUT 插值，不再为每个历史点反复读取 APVTS 并重复执行完整 Classic / Super 曲线计算。

随机有限阈值单段测试中，4097 点 LUT 相对直接公式的最大 dB 误差小于 **0.005 dB**。

## 未改变

- `StaticCompressionEngine.h` 未修改。
- Limiter 严格 Link 仍只有 DOWN Threshold、Makeup、Output Gain 1:1。
- Ratio / Mix / Classic-Super 不参与 Output Link。
- TP Ceiling 与 TIGHT / AUTO / SMOOTH Recovery DSP 不变。
- `GAIN +/-` meter / Hold 仍读取真实实时 DSP GR。
- Display 仍按 60 Hz 目标刷新；本版没有通过降低刷新率来换性能。


## Build Fix 1 — Ceiling retrospective test correction

The first 1.2.21 Windows run stopped at `Ceiling did not retrospectively update TP contribution to blue`. That assertion was incorrect. In Limiter mode, the active output gain contains the current Ceiling offset and `OutputCeiling` uses the same Ceiling as its target. Changing Ceiling alone therefore shifts both the signal entering the guard and the guard target by the same dB amount. The true-peak gain reduction is unchanged.

Correct Display contract:

- Ceiling still invalidates the projection revision and re-evaluates the existing history.
- Blue remains `Dynamics + Mix + actual TP attenuation`, so Ceiling alone does not invent additional TP GR.
- Orange/final projected output follows the Ceiling shift.

Product `Source/` is byte-identical to the original 1.2.21 candidate in Build Fix 1.
