# Current checkpoint: 1.2.0 Rev4 Stable — 2026-09-10

Rev4 supersedes the historical checkpoint below. See [STABLE_1.2.0.md](STABLE_1.2.0.md) and [VERIFICATION_REV4_1.2.0.md](VERIFICATION_REV4_1.2.0.md). The user authorized B, then C and D with the completed 23-page PDFs retained unchanged. Current production schema is 14.

---

# QQ Super Compression 1.2.0 Rev3 试听说明

交付：`D:\Codex\Outputs\QQ Super Compression 1.2.0 Rev3`。
插件版本号仍为1.2.0。Rev3已安装到系统VST3目录，构建、交付和安装文件哈希一致。请重新打开宿主，用新实例检查新的默认值。

- 所有 Ratio 默认1:1。旧工程已保存的数值继续保留。
- SINGLE：Threshold以上、有限Range以下生效。Range默认OFF，表示没有上截止。
- DUAL：UP是下限门槛；UP以下保持原音量，UP与DOWN之间提升，超过DOWN只做向下压缩。UP=-inf表示打开下限，不是关闭向上处理。两阈值重合时整级停止。
- Ratio亮弧从1:1出发。单压向左右展开；双压UP由右向左、DOWN由左向右。
- Light、Dark、Classic的五个主旋钮已统一大小和中心间距；单双压按钮更靠近Ratio。
- 双压Ratio中间的小LINK首次默认开启，之后记住上次选择；已保存工程优先恢复工程内状态。LR/MS的LINK更小。
- LINK按相反倍数相对联动：Down从1到2，Up从1到1/2；原本Down=4、Up=1/2，Down调到8时Up变为1/4。开关LINK不跳值，到范围边界共同停止。
- Ratio LINK与右侧LR/MS声道LINK是两种独立联动。Alt重置、Shift微调和数值输入继续可用；联动开启时也遵守共同边界。

快速检查向上处理：Dual、UP=-50dB、DOWN=-30dB，输入-40dB正弦，Mix100%、各增益0、Lookahead26ms。Up Ratio从1:1调到1:8、1:32，实测提升约0、7.92、9.43dB；输入降到UP以下应恢复原音量。

有限边界采用已确认的硬截止；跨边界瞬间与稳态正弦是不同测试。0ms保留原有染色/过采样逻辑。
安装前的完整版本在本目录 `Rollback - previous installed version`。替换插件前请完全退出宿主。
2026-09-10 用户已确认本版为 Stable，并执行正式 Plan B；说明书修订留待备份后进行。当前成品为 Windows x64 VST3，详见 STABLE_1.2.0.md。
