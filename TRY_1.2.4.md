# QQ Super Compression 1.2.4 — Classic / Super 验证版

此版本等待实际工程试听确认，尚未设为 Stable。发布流程暂停；超级分段压缩暂不改动。

顶部 SC 按钮左侧的 **ALGO: CLASSIC / ALGO: SUPER** 单击切换算法。

- **Classic**：1.2.3 的有限阈值固定 dB Ratio 算法。例如 Threshold −10 dB、Ratio 5:1、输入 0 dB，动态处理后为 −8 dB（未加 Makeup）。
- **Super**：1.2.2 的原 QQ 曲线。有限阈值下，压缩量随检测电平按原有非线性比例变化。
- 两种算法保留 Single / Dual、上下压缩、Range OFF、分支开关、Link，以及 ST / LR / MS。Ratio 仍为 1:1000 至 1000:1，默认 1:1。
- −inf 阈值保留原 QQ 曲线，因此在两种算法相同的工作条件下，切换可能听不到差别。用有限阈值进行对比；按需要手动或使用 MATCH 匹配响度。
- 切换使用 **10 ms 线性 Crossfade**。两套增益作用于同一份已对齐的音频，沿用同一 Lookahead 检测；不增加延迟，也不增加持续的 Attack / Release。淡变中再次切换时，从当前混合比例继续过渡。
- 新实例默认 Classic。算法随工程、预置和 A/B 保存；支持宿主自动化及撤销。读取 1.2.3 / dB 对比版状态自动选 Classic；读取 1.2.2 及更早状态自动选 Super，保持原算法。
- Display 按当前选中算法重新投影历史；音频增益表计反映实际处理后的增益。10 ms 切换过渡期间，历史投影显示目标算法。

## Candidate notes

Classic reproduces the accepted 1.2.3 finite-threshold dB law; Super restores the 1.2.2 rational law. The algorithm parameter is appended to preserve existing host IDs and indices. The two modes share the detector, delay, oversampling and carrier, and crossfade their linear gains over 10 ms at the internal sample rate. Existing Range and branch fades remain intact. No new audio latency is introduced.

New instances default to Classic. Project / A/B state and automation retain the selected algorithm. Historical states migrate to their original algorithm. This is a Windows audition candidate, not a Stable release.
