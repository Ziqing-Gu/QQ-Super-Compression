# QQ Super Compression 1.2.27 — Display Drag Timing Candidate

## 现象

1.2.26 在只打开一个实例时，持续拖动旋钮也可能感觉 Dynamic Display 的横向运动稍微变慢；松开参数后恢复正常。

## 根因

源码检查确认，1.2.21 已经存在 `displayProjectionRevision` + dirty gate：参数变化本身只增加一个轻量 revision，真正的历史重投影最多在 Display 的 60 Hz timer tick 中执行一次。因此本轮不需要再次实现参数事件 coalescing。

剩余问题来自两个独立负担：

1. 历史横坐标按“点序号 / Timer 回调数量”平均排布，而不是按真实音频时间。Mouse Drag 占用 JUCE Message Thread 时，`juce::Timer` 可以被延迟；回调变少后，480 点历史窗口实际上被拉长，于是肉眼看到 Display 进入轻微慢动作。
2. dynamics LUT 在 Ratio / Threshold / Algorithm 等变化时需要重建。旧实现对 4097 个 LUT 点逐点调用 `getDynamicsGainForDomain()`，每一点都会重复读取当前 bank / Ratio / Threshold / Algorithm 等参数。连续拖动时这部分额外占用 Message Thread。

## 1.2.27 修改

### Audio-time timeline

- `ProjectedHistory` 保留每个点的 `captureCounter`。
- 横坐标按 `captureCounter` 相对当前 audio write counter 的真实 sample age 计算。
- sample rate 使用 key-history ring 自己的 analysis rate（`min(host rate, 48 kHz)`），因此 88.2/96/192 kHz 工程不会被错误按 host rate 缩慢一倍或更多。
- 8 秒窗口继续保持不变。
- Timer callback 偶尔迟到时，下一帧会直接回到正确的真实时间位置，不再用“更少的点”换成“更慢的滚动”。

### LUT rebuild snapshot

- 4097 点 LUT 尺寸和插值精度完全不变。
- 固定的 dB→linear detector grid 在 Display 构造时只计算一次。
- 新增 `fillDynamicsGainForDomain()`：一次快照当前 dynamics state 后批量填 LUT。
- 保留 1.2.26 的 `getDynamicsGainForDomain()` scalar 路径不变；bulk helper 镜像同一参数映射和 `StaticCompressionEngine` 静态 law，并由回归测试逐点对照。

## 不变

- 60 Hz Display refresh。
- 480 点 / 8 秒窗口。
- retrospective history 重解释规则。
- Classic / Super。
- Single / Dual。
- Mix。
- TP ON/OFF、Ceiling、Recovery。
- Input / Output / Makeup 对当前投影的影响。
- 右侧实际 GAIN +/- / Hold 计量。
- 1.2.26 Shift Drag Continuity。
- 声音 DSP 和 `StaticCompressionEngine.h`。

## Windows 验收

新增 `revision1227`：

1. newest point 必须位于右边界；
2. 48 kHz 下 1 秒前的点必须位于 8 秒窗口的 7/8 位置；
3. 8 秒前的点必须正好位于左边界；
4. 更老的点必须裁剪/钳制到窗口外；
5. 44.1 kHz host 的 history counter rate 为 44.1 kHz，96 kHz host 的 history counter rate 为 48 kHz；
6. bulk LUT 与 1.2.26 scalar Display law / StaticCompressionEngine law 在 Classic/Super、Single/Dual、Normal/Limiter 代表状态下保持一致；
7. 回归 revision1211 / 1218 / 1221 / 1225 / 1226 / continuity / dual / Ceiling。
