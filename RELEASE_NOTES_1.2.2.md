# QQ Super Compression 1.2.2 Stable — 2026-09-26

## 中文

- Single Ratio 为1:1000～1000:1，Dual Up 为1:1000～1:1，Dual Down 为1:1～1000:1。默认全部1:1；取消 Infinity 档，Single 的1:1位于中央。
- 恢复 Dual Ratio LINK 的相对联动：保留 Up×Down，开启不跳值，边界共同停止；默认开启并记住上次的用户选择。
- Input 右上方新增小型 LINK，Input/Output 按等量反向 dB 联动，保留原有偏移；默认关闭，随工程保存，A/B 共用。ST 和 LR/MS 使用相应尺寸。
- 修正扩大 Ratio 范围后联动端点的控件/处理参数不同步，以及 Display 对-100 dB低电平误判为静音的问题。
- 保留 Lookahead、原动态曲线、有限 Range 连续回归、阈值碰撞、Dual 分支10 ms Crossfade、三套皮肤及原 VST3 身份。
- 继承通过的真实处理器、真实编辑器和13项源码回归结果；本次Stable提升没有重编译或修改音频代码。
- 旧工程的绝对 Ratio 数值保留；归一化 Ratio 自动化刻度已扩展，需用工程副本验证。原中英文手册保持不变，参数范围以1.2.2说明为准。

用户批准 Stable 与 Plan B。正式备份完成状态以 STABLE_1.2.2.md 指向的外部记录为准。

## English

- Single Ratio spans1:1000 to1000:1; Dual Up spans1:1000 to1:1 and Dual Down spans1:1 to1000:1. Every Ratio defaults to unity, centred in Single. Infinity mode is removed.
- Restored relative Dual Ratio LINK preserves Up×Down, causes no jump on enable, shares travel limits, defaults ON and remembers the last user choice.
- A compact Input/Output LINK above/right of Input applies equal, opposite dB changes while preserving the existing offset. It defaults OFF, saves with projects and is shared across A/B, with ST/LR/MS sizing.
- Fixed linked-endpoint knob/processor desynchronization after the expanded range, and Display treating -100 dB detector levels as silence.
- Retained Lookahead, original dynamics curves, continuous finite-Range transitions, threshold collision rules, Dual10 ms branch fades, all themes and the original VST3 identity.
- Reused the passing actual-processor/editor checks and13 source regressions. Stable promotion does not rebuild or change production code.
- Existing absolute Ratios restore unchanged; normalized Ratio automation uses the expanded scale. Test existing automation in a project copy. Original manuals are retained;1.2.2 guides supersede their Ratio limits.

The user approved Stable and Plan B. The external completion record linked from STABLE_1.2.2.md is the backup authority.
