> 发布已取消：用户反馈静音负载问题，1.2.36 Stable 提升已撤回。以下保留为历史草稿。
> Publication cancelled: the 1.2.36 Stable promotion was withdrawn after the stopped-silence load report. The text below is a historical draft.

# QQ Super Compression 1.2.36 Stable

## 中文

- 0 ms Core OS 与 Limiter Ceiling OS 新增 4x。Core 可选 1x / 4x / 8x / 16x；Hard Clip 同样提供四档；TP 提供 4x / 8x / 16x，初始值均保持 8x。
- TP 与存储 1x 的兼容行为保留：临时采用 8x，关闭 TP 后恢复 1x。旧工程、A/B 和旧离散自动化点迁移到原来的实际倍率。
- 0 ms 下相同 Core / Ceiling 倍率可复用音频过采样链，4x 已纳入共享路径、变量块长和停止/唤醒回归。同倍率 Hard/TP 保持同档延迟，异倍率依宿主重新补偿。
- FULL / ECO 每实例独立保存；停止且全零输入、内部尾段稳定后休眠，播放或任何非零输入在当前音频块恢复。延续 1.2.35 的实例模式、性能与审查修复。
- 当前中英文说明书均为 1.2.36、41 页。完整说明 Limiter 的标准高比例下压 + Output LINK、少量 UP 补偿，以及 0 ms Lookahead 让副歌很响而保留安静部分音量的特色；结合核心 4x / 8x / 16x 试听。
- 用户已确认 Stable。Windows Release 与自动回归通过：216 组 Ceiling 压力用例（44.1 / 48 / 96 kHz），Ceiling=-1 dB 时最坏采样峰值 -1 dBFS、真峰值 -1.00007 dBTP；16 种 Core / Ceiling 组合、状态与 A/B、变量块长和停止唤醒检查通过。唤醒与连续参考最大差 0。
- Windows 已安装成品与验证输出哈希一致。以上为已完成的本地自动验证；不将它描述为新的 Cubase 听感、离线导出或 ASIO-Guard 实测。

## English

- Adds 4x to both 0 ms Core OS and independent Limiter Ceiling OS. Core and Hard Clip offer 1x / 4x / 8x / 16x; TP offers 4x / 8x / 16x. Both initial rates remain 8x.
- Preserves compatibility for TP with a stored 1x choice: temporarily use 8x and restore 1x when TP is disabled. Older projects, A/B and discrete automation points migrate to their original actual rates.
- Matching Core / Ceiling rates share the audio oversampling path at 0 ms. The new 4x rate is covered by sharing, variable-block and stopped-wake checks. Same-rate Hard/TP preserves latency; rate changes may require new host compensation.
- FULL / ECO is stored independently per instance. Sleep follows stopped transport, all-zero inputs and settled tails. Playback or any nonzero input wakes processing in the current audio block. Retains the 1.2.35 instance, performance and review fixes.
- Both manuals now match 1.2.36 and have 41 pages. They cover standard high-ratio downward limiting with Output LINK, gentle upward compensation, and the defining 0 ms Lookahead feature: a very loud chorus while retaining quiet-section levels. Try core oversampling at 4x / 8x / 16x.
- Stable is user-confirmed. The Windows Release build and automatic checks pass: 216 Ceiling stress cases at 44.1 / 48 / 96 kHz; at Ceiling=-1 dB, worst sample peak -1 dBFS and true peak -1.00007 dBTP. All 16 Core / Ceiling combinations, state and A/B, variable blocks and stopped-wake checks pass. Wake output differs from the continuous reference by 0.
- Installed Windows output matches the validated artifact hash. These are completed local automatic checks, not new Cubase listening, offline-export or ASIO-Guard measurements.

## Historical pre-Stable build record

The earlier build record below is retained as history. Its manual-review status has been superseded by the completed bilingual manuals above.

# QQ Super Compression 1.2.36

- 0 ms Core OS 与 Limiter Ceiling OS 均新增 4x；原有 1x、8x、16x 保留。Core 和 Ceiling 独立设置。
- Hard Clip 支持 1x/4x/8x/16x；TP 支持 4x/8x/16x。默认继续 8x。TP+存储 1x 继续按 8x 处理，以保留旧行为。
- 0 ms Core4/Ceiling4 复用同一音频过采样链；异倍率使用独立 Ceiling 链。Ceiling 后级 16x 重建安全检测保留。
- 状态升级为 schema 28，将旧 Normal、Limiter、Ceiling 及 A/B 的选项迁移到相同实际倍率；旧离散自动化 0/0.5/1 仍对应 1x/8x/16x。
- 所有滤波器预先分配；保持同倍率 Hard/TP 的延迟关系，并将 4x 纳入停播静音恢复与大块音频回归。
- Windows Release 构建通过；216 组 Ceiling 上限压力测试通过（44.1/48/96 kHz，4x/8x/16x，Hard/TP）。Ceiling=-1 dB 时测得最坏样本峰值 -1 dBFS、真峰值 -1.00007 dBTP。
- 新旧状态、A/B、旧离散自动化点、按钮切换、16 种 Core/Ceiling 组合、动态块长、停播/唤醒和既有 Limiter/Unity 检查通过。停播唤醒与连续处理参考最大差为 0。
- 以上为自动音频/参数检查，尚未完成本版 Cubase 听感与 ASIO-Guard 实测。中文说明书审阅稿仍对应 1.2.35，待功能确认后更新；英文继续等待中文确认。

- 已在无宿主占用时安装 Windows VST3，安装文件与 D 盘验证输出哈希一致。旧版 1.2.35 已保存为可回退备份；记录见 Verification/1.2.36-Windows/installation-verification.json。
