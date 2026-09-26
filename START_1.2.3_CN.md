# QQ Super Compression 1.2.3 Stable

用户已确认 QQ dB Compression 测试版的算法作为新的 Super Compression。本版恢复正式名称与原来的 VST3 身份，替换 QQ Super Compression 1.2.2；独立 QQ dB Compression 测试插件已从系统安装目录移除。

## 安装与使用

关闭音频宿主，将 Win 中整个 `QQ Super Compression.vst3` 文件夹安装至 `C:\Program Files\Common Files\VST3`，替换同名旧版。重新扫描后，选择 **QQ Super Compression**，界面版本应为 **v1.2.3**。请勿通过改文件名来安装。

原 Super Compression 工程继续识别正式插件，参数数值及原有偏好仍可读取。有限阈值会采用新算法，所以旧工程在这些设置下的声音会变化；−inf 按之前约定保留旧曲线。曾使用独立 QQ dB Compression 的工程需要把该实例替换为正式 Super Compression。

**使用有限 Threshold 才会进入新曲线；−inf 仍是兼容分支。所有 Ratio 默认仍为 1，需要调整才有压缩。** 可用 Single、Threshold −10 dB、Ratio 5:1、Range OFF、Mix 100%、Lookahead 26 ms、其余增益 0 dB 验证：0 dB 稳态信号输出 −8 dB，阈值以下信号不变。

## 算法变化

有限阈值下的 Down 按固定 dB 比例处理：检测电平超过阈值的部分除以 Ratio。公式为 `输出电平 = 阈值 + (检测电平 − 阈值) / Ratio`。例如阈值 −10 dB、Ratio 5:1、检测电平 0 dB，稳态输出 −8 dB；低于阈值的 −12 dB 保持不变。以上描述适用于 Range OFF 和无额外增益；有限 Range 的回零过渡会修改上边界附近的曲线。

Up 仍只提升 UP 阈值以上的内容，下边界仍是 gate。为了保留上下对称，它与互为倒数的 Down 使用相同 dB 斜率，但以作用区间上边界作为 0 dB 增益基准：`提升 dB = (上边界 dB − 检测电平 dB) × (1 − Up Ratio)`。上边界是 Single 的有限 Range、Single Range OFF 时的 0 dB，或 Dual 的 DOWN 阈值。避开边界过渡，在共同有效区间内，8:1 和 1:8 的波形可通过固定增益补偿重合。这不是把 Down 增益简单变号。

Single 到达有限 Range 后动态增益为 0 dB；Dual 到达 DOWN 阈值后停止 Up，仅由 Down 接手。原有 gate/Range 区间内过渡、10 ms 分支开关 Crossfade、Lookahead 检测和延迟均保留，没有增加 Attack/Release。阈值指检测包络，不是每个瞬时音频采样值；Lookahead 因此仍会在突变附近产生预影响。

−inf 的兼容分别按分支判断：Down 阈值为 −inf 时使用旧 Down 公式，Up gate 为 −inf 时使用旧 Up 公式。Dual 的 UP 是 −inf、DOWN 是有限值时，两分支分别使用旧 Up 和新 Down。

Ratio 范围维持 Single 1:1000～1000:1、Up 1:1000～1:1、Down 1:1～1000:1。新的固定 dB Up 可能比原版提升更多，提升量由检测电平距上边界的 dB 数决定，不再受原公式的 60 dB 上限约束。本版数值处理及显示计算支持至 120 dB 提升；深 gate 应结合实际素材与电平表调节，原有图表的固定纵轴范围保持不变。

## 保留内容

Single/Dual、ST/LR/MS、三套皮肤、Ratio 相对 Link、Input/Output 反向 Link、分支开关、A/B、MATCH、侧链、Mix 与 1.2.2 的操作方式保持一致。原版 PDF 手册作为这些控件的参考原样附带；新曲线请以本说明为准。

算法为用户确认的 1.2.3 Stable 版，Windows 已完成验证。完整源码、资产、文档、构建脚本和 JUCE 8.0.15 依赖纳入 Plan B；完成状态以 STABLE_1.2.3.md 中的外部完成记录为准。项目许可保持 Qing Audio 非商业源码共享许可。

本版按用户要求执行 Plan C/D，发布 Windows x64 VST3、Mac 双架构 VST3 与 Universal 2 AU。原 PDF 沿用；平台安装以本版安装指南为准。
