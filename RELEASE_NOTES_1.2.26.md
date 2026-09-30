# QQ Super Compression 1.2.26 — Shift Drag Continuity Candidate

## 修复

修复部分旋钮在持续拖动过程中突然按下或松开 Shift 时，参数值发生回跳的问题。

旧的 Rotary 拖动依赖 JUCE 原生 `setMouseDragSensitivity()`。原生 drag 以最初 mouseDown/value 为锚点；中途改变 sensitivity 后，已经发生的累计鼠标位移会被新灵敏度重新解释，因此可能出现数值突然回退或前跳。

1.2.26 将 `FineKnob` 的 Rotary drag 改为逐事件增量计算：

```text
当前值
+ 本次 MouseEvent 相对上次 MouseEvent 的位移
× 当前 Shift 灵敏度
= 新值
```

因此 Shift 状态变化本身不会产生任何参数 delta；只有状态变化后的新鼠标位移才使用新的灵敏度。

## 保持不变

- 普通 Rotary 灵敏度：180 px / full range。
- Shift 精调灵敏度：1200 px / full range。
- Ratio 继续使用当前 Slider `NormalisableRange`，包括 Limiter SINGLE `1:8 → 1:1 → 1000:1` 的非线性映射。
- Threshold / Range 继续使用既有 LinearVertical 增量拖动。
- Ceiling 数值拖动继续使用既有增量逻辑。
- Alt reset、数值输入、Undo/Redo、APVTS gesture 语义不改。
- Classic / Super DSP、Limiter LINK、TP Ceiling / Recovery、Display retrospective / projection cache 不改。

## Windows 验收

新增 `revision1226`：

1. 普通拖动中按下 Shift，同一鼠标位置不得改变值；
2. Shift 拖动中松开 Shift，同一鼠标位置不得改变值；
3. Shift 后续位移必须明显比普通拖动精细；
4. 非线性 Ratio 在 Shift transition 时不得跳变；
5. Limiter 模式重绑定后的 Ratio 同样不得跳变；
6. Threshold 的既有增量拖动连续性不得回归。
