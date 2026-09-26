# QQ Super Compression 1.2.5 Stable — 2026-09-27

用户明确批准当前 Classic90 版本为 Stable，并要求先完成 Plan B，再开展独立前瞻实验。此次稳定化只更新记录与复现脚本；生产源码、资源、参数、版本号和二进制均保留已试听版本。

- Classic 有限阈值下限 −90 dB；Super 保留 −inf，Range OFF 保持无上界。旧存储值在 Classic 中按有效下限解释，未编辑值可在 Super 中继续使用。
- Classic / Super 支持工程状态、A/B 和最后一次明确 UI 选择的记忆；首次 Classic。I/O LINK 首次 ON，并记忆最后一次明确操作，工程状态优先。
- 相同 Input、检测和延迟配置下，A/B 对完整结果做 20 ms S 形淡变；直接算法按钮仍为 10 ms。不同输入、侧链或延迟配置仍使用已有重配置路径。
- MATCH / Makeup 支持 ±120 dB，取消 −70 LUFS 绝对门限，保留 K 加权及相对门控；真正零信号和缺失测量不产生补偿。该匹配测量不等同于严格 EBU R128 Integrated LUFS。
- 插件身份、参数 ID 和顺序不变。旧工程的归一化 Makeup 自动化须核对，因为参数范围扩大。

已知设计边界：当前未来窗口峰值检测会在电平突变前产生预影响；比窗口峰值小的局部声音可能被压到阈值以下。Display 是历史输入与检测电平的投影，不能替代实际输出录音。用户知悉此行为后将本版固定为稳定对照；改进方案不属于本版。稳态正弦低谐波结果不构成任意信号下零失真的保证。

## English

User-approved Stable baseline, preserving the auditioned Classic90 audio and binary without DSP changes. Classic has a finite −90 dB minimum; Super retains −inf. Range OFF remains unbounded. Algorithm choice and initially enabled I/O LINK remember explicit user choices while saved project state takes priority.

Complete-result A/B uses a 20 ms smoothstep crossfade when input, detector and timing configuration match; direct algorithm switching remains 10 ms. MATCH removes the absolute loudness gate and supports ±120 dB Makeup, retaining K weighting and relative gating. Existing normalized Makeup automation requires review after the range expansion.

The future-window detector still has pre-influence around level changes and can attenuate smaller local material below the threshold. Display is a historical projection rather than a captured output waveform. This known behavior is retained in the approved baseline. A separate experiment will investigate alternatives; it is not part of this Stable. Low steady-tone harmonics are not an unconditional zero-distortion claim.
