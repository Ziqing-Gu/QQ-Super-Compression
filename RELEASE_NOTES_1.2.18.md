# QQ Super Compression 1.2.18 — TP Recovery Candidate

基线：**1.2.17 Stable / Plan B**。

本版只继续完善 True-Peak Ceiling 的恢复包络，不改变 1.2.17 已确认的 Limiter Link、Ceiling 目标、Normal/Limiter 参数隔离和 retrospective Dynamic Display 原则。

## TP Recovery

Limiter + TP ON 时新增三档 Recovery：

- **TIGHT**：最快恢复，优先释放 TP 产生的额外 GR，尽量保留响度。
- **AUTO**：1.2.17 原有自适应 TP 时序，作为默认值；算法公式保持不变。
- **SMOOTH**：更慢、更平滑的恢复，牺牲部分短时响度换取更平静的增益包络。

点击 Ceiling 区域中的 Recovery 按钮循环：`AUTO → TIGHT → SMOOTH → AUTO`。TP OFF 时隐藏该按钮，但保留用户上次选择。

三档模式都继续遵守同一个 True-Peak Ceiling 上限：Recovery 只改变峰值过去后的恢复速度，不改变 8x TP 主保护、16x reconstruction 检查或 0.01 dB residual safety 规则。

## Display / Meter

- 右侧 `GAIN +/-` meter 与 Hold 继续读取真实实时 DSP 的 TP Gain Reduction。
- Dynamic Display 继续遵守 1.2.17 的 retrospective 设计：历史保存的是可重新解释的证据，而不是已经渲染好的 TP 结果。
- 切换 TIGHT / AUTO / SMOOTH 后，已经显示的历史也会按当前 Recovery 重新投影。
- Limiter 蓝色/橙色仍保持同轮廓原则；Recovery 只改变两者共同的 TP 后轮廓。

## 未改变

- Limiter Link 仍然只有 DOWN Threshold、Makeup、Output Gain 严格 1:1。
- Ratio、Mix、Classic/Super、UP Ratio、UP Gate 不参与 Output Link。
- TP Ceiling=0 仍以尽量接近 0 dBTP、但不超过为目标。
- Normal Input / Output 仍为 ±24 dB。
- Normal Makeup ±30 dB；Limiter Makeup / Output 保留 ±120 dB。
- Single / Dual continuity、A/B、状态恢复、算法行为不重构。

## Windows 构建

直接运行 `BUILD_WINDOWS.cmd`。默认构建目录：

```text
D:\Codex\Temp\QQSC1218-Build
```

默认验收包含 `revision1211`、`revision1217`、`revision1218`、`continuity`、`dual` 与 `QQSCCeilingCheck`。脚本不自动安装插件，也不执行 Plan B。
