# QQ Super Compression 1.2.2 — Ratio 1000 与增益联动

Windows x64 VST3 Stable 版，基于 1.2.0 Stable，保留原有动态处理算法与 Lookahead。

| 控件 | 范围 | 默认 |
| --- | --- | --- |
| Single Ratio | 1:1000 → 1:1 → 1000:1 | 1:1 |
| Dual Up Ratio | 1:1000 → 1:1 | 1:1 |
| Dual Down Ratio | 1:1 → 1000:1 | 1:1 |

所有 Ratio 均为有限数值，没有 Infinity 档。Single 的 1:1 仍在旋钮中央。可在数值框输入 `1:1000`、`1/1000` 或 `1000:1`。

Dual 的 Ratio LINK 恢复原有相对联动：从 Up=1、Down=1 开始，Down 调到2时 Up 变为1/2，最大可一起达到 Up=1/1000、Down=1000。若原来是 Up=1/4、Down=8，将 Down 加倍到16，Up 会变为1/8，保留原有比例关系。开启 LINK 不会立即改变已有数值；共同边界会限制两侧的移动。Ratio LINK 默认开启并记住上次的用户选择。

Input 右上方新增小型 LINK，默认关闭，状态随工程保存，A/B 共用该状态。开启后 Input 与 Output 按等量反向 dB 联动；例如 Input +3 dB，Output -3 dB，反向操作也成立。已有增益偏移会保留，开启按钮不会立即改变数值。任意一侧达到 ±24 dB 边界后，两侧同时停止。LR/MS 模式使用更小的按钮。

1:1000 是 Ratio 数值，并不是固定放大1000倍。实际增益取决于检测电平、阈值和 Mix。声音仍需超过向上 Threshold 才得到提升；Dual 达到 Down Threshold 后由向下分支接管。保留原 Range、阈值碰撞、分支 ON/OFF Crossfade 与三套皮肤。高强度上压的 Mix、Display 和数字增益读数支持新的范围；增益柱沿用原固定刻度，超出柱状范围时以数字读数为准。

## 安装与验证

关闭音频宿主，把 `Win/QQ Super Compression.vst3` 整个文件夹复制到 `C:\Program Files\Common Files\VST3`，替换旧版。重新扫描/加载后界面版本应为 v1.2.2。

可先用 Single、Range OFF、Threshold −inf、Ratio 1:1000、Lookahead 26 ms、Mix 100%，Input/Makeup/Output 均为0 dB，试听安静片段。Dual 可用 Up Threshold −inf、Down Threshold 0 dB、Down Ratio 1:1、Up Ratio 1:1000，并先关闭 Ratio LINK 再分别设置。

旧工程保存的绝对参数值可以恢复。上下 Ratio 的归一化刻度均已扩展，旧的 Ratio 自动化位置会对应不同数值；有 Ratio 自动化的工程请用副本验证。Input/Output LINK 只联动当前界面的主动操作，不会在加载状态或回放宿主自动化时额外改写另一条参数。

原中英文手册原样附带；手册中的 Ratio 范围以本说明为准。源码与原许可证随包提供。2026-09-26 用户确认 Stable，并执行正式 Plan B 备份；本次未执行 GitHub Release。
