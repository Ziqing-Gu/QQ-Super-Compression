# QQ Super Compression 1.2.23 — Verification Notes

## 目标

只调整 Limiter 上压 Ratio 的参数范围，不修改压缩 DSP。

### Limiter SINGLE

- 上压：`1:8 → 1:1`。
- Unity 后保留原来的 Limiter 下压区：`200:1 → 1000:1`。
- `1:1 → 200:1` 之间继续保持不可进入的 Limiter Single 间隔。

### Limiter DUAL

- UP：`1:8 → 1:1`。
- DOWN：`200:1 → 1000:1`。

### Normal

Normal SINGLE / DUAL 的上压范围仍为 `1:200 → 1:1`，不受本版影响。

## 迁移

旧状态中的 Limiter SINGLE 或 DUAL 上压 Ratio 若小于 `1:8`，在 1.2.23 中夹到 `1:8`。

## 保护项

`StaticCompressionEngine.h` 必须保持 SHA-256：

```text
51B36D3AA7BE1113AA4E90AC6C0C534163A9B7A3A9EDC2DFE24D455CAD3FA6BA
```

TP、Display、Link、Ceiling、Recovery 不属于本轮改动范围。

## Windows 验收

`BUILD_WINDOWS.cmd` 运行：

- `QQSCLimiterCheck revision1211`
- `QQSCLimiterCheck revision1218`
- `QQSCLimiterCheck revision1221`
- `QQSCLimiterCheck revision1223`
- `QQSCLimiterCheck continuity`
- `QQSCLimiterCheck dual`
- `QQSCCeilingCheck`

`revision1223` 重点检查：

1. Limiter SINGLE host 参数下限为 `0.125`，上压文本输入 `1:32` 会夹到 `1:8`。
2. Limiter DUAL UP 同样为 `0.125 → 1.0`。
3. Normal SINGLE / DUAL 仍保留 `0.005` (`1:200`) 的上压下限。
4. 旧状态中的 Limiter 上压值会按新范围迁移。
5. Single Limiter 的 1:1 / 200:1 分段与 200:1 → 1000:1 下压区仍然正常。

## Build Fix 1 验收说明

Limiter DUAL 的 `1:8 → 1:1` 是控件/参数的绝对范围。`dualRatioLink` 默认开启时，UP 与 DOWN 继续保留既有的乘积耦合；如果 DOWN 先碰到 `1000:1` 上限，UP 可能在高于 `1:8` 的位置停止，这是 LINK 的既有约束，不表示 UP 的范围错误。

因此 Windows 的绝对范围与文本输入夹值测试会先关闭 Dual Ratio LINK，再验证输入 `1:32` 必须夹到 `1:8`；随后已有的 Ratio LINK 回归继续单独验证耦合逻辑。产品 `Source/` 不因 Build Fix 1 改动。
