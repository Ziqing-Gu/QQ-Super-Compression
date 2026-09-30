# QQ Super Compression 1.2.24 — Build Fix 1

## 原因

1.2.24 Windows 验收在 `revision1224` 停止于：

```text
FAIL: Dual Ratio LINK-off test fixture was not established
```

这是测试夹具选择了错误的参数 bank，并非产品 Ratio 范围或 Alt-reset 行为失败。

Limiter / Normal 的 `dualRatioLink` 是独立 mode bank。进入 Limiter 后，编辑器通过 `soundParameterID("dualRatioLink")` 实际读取的是 `limiterDualRatioLink`。旧测试只初始化了 Normal-bank 的 `dualRatioLink` 偏好，因此无法保证 Limiter DUAL 的 LINK 已关闭。

## 修正

Build Fix 1 只修改测试和构建辅助文件：

- 创建 Editor 后，直接把当前 active Limiter bank 的 `soundParameterID(dualRatioLink)` 设置为 OFF；
- 同时检查 processor 的 `readSoundParameter(dualRatioLink)` 与 UI `LINK` 按钮均为 OFF；
- 然后再执行 Limiter DUAL UP `1:32 -> 1:8` 的 numeric-entry clamp 测试；
- Windows 默认构建目录改为 `D:\Codex\Temp\QQSC1224-BF1-Build`，避免复用旧 CMake cache。

产品 `Source/` 与 1.2.24 Candidate 完全一致；没有修改 Classic / Super、TP、Display、Limiter Link 或任何音频 DSP。
