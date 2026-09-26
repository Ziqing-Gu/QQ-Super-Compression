# QQ Super Compression 1.2.3 Stable — 2026-09-26

## 中文

用户认可 QQ dB Compression 对比版，并将其算法正式采纳为新的 Super Compression。

- 有限 Down 阈值采用固定 dB Ratio：`输出 = 阈值 + (检测电平 − 阈值) / Ratio`。−10dB/5:1 下，0dB 稳态输入输出−8dB。
- 有限 Up gate 仍只允许提升阈值以上的部分，与倒数 Down 使用相同 dB 斜率。避开边界过渡，在共同有效区间内匹配固定增益后可重合。
- 对应分支的−inf阈值保留原算法。Single Range、Dual交接、Lookahead、10ms分支Crossfade不变。
- 保留1:1000～1000:1范围、全部默认1、Ratio相对Link、Input/Output反向Link、三套皮肤和ST/LR/MS。
- 保留对比版的深阈值及浮点精度修正：有限阈值支持至−120dB哨兵之前，Up计算支持120dB，深Down的混合与电平显示保持精度。
- 恢复正式产品名、原Qscp VST3身份及QQSuperCompression偏好文件；移除系统独立QQ dB Compression测试插件。
- 旧工程参数继续读取，有限阈值下的声音随新算法变化；−inf兼容分支保持。测试版独立实例需手动换为正式版。

Stable、Plan B 和后续 Plan C/D 均由用户批准。发布包括 Windows x64 VST3、Mac Apple Silicon / Intel VST3、Universal 2 AU 及双语安装指南。原 PDF 手册保持原样，算法与参数范围以本版补充说明为准。

## English

The accepted QQ dB Compression comparison algorithm is now the official Super Compression.

- Finite Down uses fixed dB Ratio: output=threshold+(detector-threshold)/Ratio. At-10dB/5:1, steady0dB input produces-8dB output.
- Finite Up gates still lift only above the lower threshold, with the same dB slope as reciprocal Down. Their common interior matches after constant gain compensation, away from boundary transitions.
- Corresponding-inf branches retain the legacy law. Single Range, Dual handover, lookahead and10ms branch crossfades remain unchanged.
- Retains1:1000..1000:1 ranges, unity defaults, relative Ratio Link, opposite Input/Output Link, all skins andST/LR/MS.
- Retains the accepted deep-threshold/float-precision fixes,120dB Up calculation headroom and accurate deep Down audio/Mix/meters.
- Restores the formal product name, originalQscp VST3 identity and QQSuperCompression preferences; removes the separately installed QQ dB Compression test plugin.
- Existing project parameters load; finite-threshold sound changes to the new algorithm. Independent test instances must be replaced with the formal plugin.

The user approved Stable, Plan B and subsequent Plan C/D. The release includes Windows x64 VST3, Mac Apple Silicon / Intel VST3, Universal 2 AU and bilingual installation guides. Original PDFs remain unchanged; current supplemental guides describe the accepted algorithm.

## Since public 1.2.0 / 自上次公开版本

1.2.1 was a local infinity-ratio experiment; the final range in 1.2.2 is finite 1:1000 through 1000:1. 1.2.2 also restores relative Ratio Link and adds equal-and-opposite Input/Output Link. 1.2.3 promotes the accepted finite-threshold dB law. Saved absolute ratios survive; normalized automation from the older range changes mapping.

1.2.1 的无穷大 Ratio 是本地实验；1.2.2 最终采用有限的 1:1000～1000:1，恢复相对 Ratio Link，并增加 Input/Output 等量反向联动。1.2.3 正式采用用户认可的有限阈值 dB 曲线。保存的实际比例保留，旧范围的归一化自动化映射变化。
