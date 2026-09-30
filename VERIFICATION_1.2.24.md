# QQ Super Compression 1.2.24 — Verification Notes

## 基线

基于 1.2.23 Build Fix 2。产品行为只增加一项：Limiter SINGLE Ratio 的 Alt+单击复位值与其声音 bank 默认 `200:1` 对齐。

## 产品源码改动

`Source/LimiterEditor.cpp`：在 mode / Single-Dual 状态刷新时同步设置 Ratio 控件的 Alt-reset 值：

```text
Limiter SINGLE   -> 200:1
Limiter DUAL UP  -> 1:1
Limiter DUAL DOWN-> 200:1
Normal SINGLE    -> 1:1
```

核心 DSP 未修改，`StaticCompressionEngine.h` SHA-256 仍应为：

```text
51B36D3AA7BE1113AA4E90AC6C0C534163A9B7A3A9EDC2DFE24D455CAD3FA6BA
```

## 验收

`revision1224` 使用真实 `FineKnob::mouseDown/mouseUp` Alt+左键事件验证上述四种复位行为，并继续运行 1.2.23 的 Ratio 范围 / legacy clamp / numeric-entry 检查。

Windows `BUILD_WINDOWS.cmd` 默认构建到：

```text
D:\Codex\Temp\QQSC1224-BF1-Build
```

并运行 `revision1211`、`revision1218`、`revision1221`、`revision1223`、`revision1224`、`continuity`、`dual` 与 `QQSCCeilingCheck`。


## Build Fix 1

首次 Windows 验收暴露的是 test fixture bank 选择错误：Limiter mode 下 UI 使用 `limiterDualRatioLink`，而旧 fixture 只初始化了 Normal bank 的 `dualRatioLink`。Build Fix 1 改为在 Editor 创建后直接关闭当前 active Limiter bank 的 Ratio LINK，再执行 DUAL UP numeric-entry 绝对范围检查。产品 `Source/` 不变。
