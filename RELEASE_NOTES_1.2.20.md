# QQ Super Compression 1.2.20 — Display GR Restore Candidate

## 目标

恢复 QQ Super Compression 原本的 Display 设计：蓝色 `Cut / Mix` 由当前压缩算法重新计算，不把已经发生过的处理结果写死在历史中；Limiter 仍然只是高 Ratio 的 QQ Super Compression，再叠加 TP 影响。

## 蓝色 Cut / Mix

蓝线按以下顺序计算：

```text
历史 detector / carrier 证据
→ 当前 Classic / Super 曲线
→ 当前 Mix
→ 当前 TP 影响（仅 TP ON）
→ 蓝色 Cut / Mix
```

不把 Makeup 或 Output Gain 直接算入基础 GR。

### TP OFF

蓝线表示当前 QQ 压缩曲线经过 Mix 后的结果。内部 Sidechain、有限 DOWN Threshold、输入高于 Threshold 时，有限 Ratio 的结果可以无限接近 Threshold，但不会被画到 Threshold 以下。

### TP ON

先得到同一份压缩 / Mix 结果，再加入当前 True-Peak Ceiling 的额外衰减。此时蓝线允许低于 Threshold，因为额外下压来自 TP。

## 修正异常尖峰

INT 模式不再把某个 host block 的独立 carrier 峰值与另一个 lookahead detector 峰值直接配对。Display 使用 detector 对齐后的传输电平做静态曲线投影，避免出现压缩曲线本身不可能产生的向上尖峰。

EXT 模式下 detector 与 carrier 本来就是两路独立信号，因此保留 captured carrier 作为显示参考。

## Retrospective Display

历史不保存最终蓝线。下列当前参数改变后，已经存在的历史会重新投影：

- Ratio
- Threshold / Range / Dual boundaries
- Classic / Super
- Single / Dual
- Mix
- TP ON / OFF
- TP Recovery
- Input Gain
- Makeup
- Output Gain
- Ceiling
- Key Gain / HPF
- Lookahead（通过历史 detector replay）

切换 Limiter ON / OFF 不再清空同一来源的历史证据。

## 未改变

- `StaticCompressionEngine.h` 与 1.2.18 输入基线逐字节一致。
- Classic / Super 核心 DSP 未修改。
- 1.2.17 Stable 的严格 Threshold / Makeup / Output 1:1 Limiter Link 未修改。
- Ratio / Mix / Classic-Super 不参与 Output Link。
- 1.2.18 TIGHT / AUTO / SMOOTH TP Recovery DSP 未修改。
- TP Ceiling 8x 主保护、16x reconstruction 检查与 0.01 dB residual safety 未修改。
- 右侧 `GAIN +/-` meter / Hold 继续读取真实实时 DSP GR。
