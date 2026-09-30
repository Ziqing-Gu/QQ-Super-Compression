# QQ Super Compression 1.2.27 — Verification Notes

## 基线

开发基线：**1.2.26 Stable / Plan B**。

正式 Plan B 与 `D:\Codex\Archives` 备份不修改。本 Candidate 从 1.2.26 完整源码副本继续。

## 产品代码改动范围

- `Source/DynamicDisplay.h`
- `Source/DynamicDisplay.cpp`
- `Source/PluginProcessor.h`
- `Source/PluginProcessor.cpp`

没有修改：

- `Source/StaticCompressionEngine.h`
- TP Ceiling / Recovery DSP
- Limiter Ratio / Link
- Shift Drag
- 音频处理路径的 compressor 算法

## 关键结论

### 1. 已有 coalescing 保留

1.2.26 已经在 processor parameter callback 中只递增 `displayProjectionRevision`，DynamicDisplay 仅在 60 Hz timer 中比较 revision 并重建一次 cache。没有在 slider callback 中直接进行历史重投影。

因此 1.2.27 不新增第二套 dirty queue，也不改变 60 Hz 刷新逻辑。

### 2. 横向时间改为 audio-history counter

旧实现：

```text
x = history point index / 479
```

这意味着 UI Timer 少一次 callback，就少推进一个历史点。

新实现：

```text
ageSeconds = (currentAudioCounter - pointCaptureCounter) / sampleRate
x = right - clamp(ageSeconds / 8 seconds) * plotWidth
```

Timer 延迟只影响瞬时帧数，不再改变历史窗口的真实时间速度。

### 3. LUT 重建只快照一次参数

旧实现：

```text
4097 x processor.getDynamicsGainForDomain(...)
```

每一点都重复读取当前 APVTS / bank / mode 参数。

新实现：

```text
processor.fillDynamicsGainForDomain(detectorGrid, gainLut, 4097, domain)
```

bulk helper 先读取一次当前 dynamics state，再用完全相同的 `StaticCompressionEngine` 静态 law 填 4097 点。

## 自动验收

新增 `revision1227_display_drag_performance_checks.inc`：

- audio-time x mapping；
- 8 秒裁剪；
- 44.1/96 kHz host 下 history counter rate 选择；
- bulk/scalar/static dynamics law parity。

新增 `revision1227_display_drag_performance_audit.py`：

- 60 Hz / 480 / 8 秒规格未下降；
- audio-history counter timeline hooks 存在，并明确使用 history ring 的 analysis sample rate；
- dense 4097 LUT 保留；
- bulk snapshot helper 存在；
- `StaticCompressionEngine.h` SHA-256 保持既定值。

最终 Windows Build Helper 继续运行：

- revision1211
- revision1218
- revision1221
- revision1225
- revision1226
- revision1227
- continuity
- dual
- QQSCCeilingCheck

## 用户实测重点

在 Cubase 播放时只开一个实例，连续拖动：

- Ratio
- Threshold
- Mix
- Input
- Output
- Makeup
- Ceiling（Limiter）
- Classic / Super 切换

观察 Display：

1. 横向速度是否不再在拖动期间明显变慢；
2. 松手后是否不存在追赶/跳变；
3. 参数变化时过去历史仍立即按当前参数重新解释；
4. 多实例表现不得比 1.2.26 更差。
