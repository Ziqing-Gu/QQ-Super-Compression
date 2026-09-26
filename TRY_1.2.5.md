> Historical candidate record, superseded by [STABLE_1.2.5.md](STABLE_1.2.5.md) on 2026-09-27. The auditioned binary was promoted unchanged.

# QQ Super Compression 1.2.5 — Classic −90 dB 修订验证版

等待实际工程试听确认，尚未设为 Stable。超级分段压缩及发布流程继续暂停。

- Classic 的阈值最低端现在是有限的 **−90 dB**，继续使用同一固定 dB Ratio 公式，不再突然切换到 Super 算法。Super 保留原来的 −inf。Range OFF 仍是无上界。
- **A/B 使用 20 ms S 形交叉淡变**，共同切换 Ratio、Threshold、算法、Makeup、Mix 和 Output 所决定的完整结果。相同结果的两组设置不会因中间的 Ratio / Makeup 组合而产生额外音量凹陷。支持快速反向切换，不增加音频延迟或持续 Attack / Release。
- 上述 A/B 淡变适用于两组 Input Gain、Lookahead、Oversampling 和 Side Chain 配置相同的情况。改变检测来源或延迟配置仍采用原有重配置路径，不属于本次无缝对比验证范围。直接点击算法按钮仍使用 10 ms 淡变。
- **MATCH / Makeup 范围扩展到 −120～+120 dB**。按用户要求取消 −70 LUFS 绝对检测门限，保留 K 加权和相对响度门控，极低电平也可以参与匹配。真正的零信号、缺失的 Wet 或不足的测量数据不会产生补偿。此测量用于匹配增益，不再称为严格符合 EBU R128 的 Integrated LUFS。
- MATCH 仍在宿主播放时收集数据，写入对应 ST / L / R / M / S 的 Makeup。测量至少需要一个 400 ms 有效窗口；在目标参数下播放稳定片段后再点击。
- Input/Output LINK 首次使用默认打开，显式点击会记住选择，供下次新实例使用。工程中保存的状态优先；重新打开已有实例的界面不会覆盖它。
- Classic / Super 同样记住上次手动点击的选择，供下次新实例使用；首次默认 Classic。已有工程、预置及 A/B 保存的算法优先，回放自动化或召回 A/B 不改写全局偏好。
- Display 显示当前所选参数的处理结果；A/B 短暂淡变期间，历史投影显示目标设置。

兼容说明：插件身份、已有参数 ID 和顺序不变，保存的 Makeup dB 数值继续读取。Makeup 参数范围扩大，因此旧宿主的归一化 Makeup 自动化需要核对；旧范围内的旋钮位置也随新范围变化。原来明确使用 Classic −inf 的状态现在按 −90 dB 处理；Super 的声音保持原算法。

优先试听：截图中的 Super / Single / −inf / Range OFF / 26 ms，A 为 10:1、Makeup +5.07 dB，B 为 1:10、Makeup −14.88 dB；随后测试 Classic 最低阈值与大压缩量 MATCH。

本轮只在 D 盘生成候选文件，并在宿主关闭后替换系统 VST3。不创建桌面副本。

本次修订（2026-09-27）：Classic 的 Single Threshold、Dual 上下阈值以及有限 Range 最低均为 −90 dB，推子下限、数值输入、Alt 复位、宿主读数和 DSP 一致。低于 −90 dB 的历史存储值在 Classic 中按 −90 dB 处理；保留宿主原参数范围以兼容 Super 的 −inf，未编辑的旧存储值可在切回 Super 时继续使用。
