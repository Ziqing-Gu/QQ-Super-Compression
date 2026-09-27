# QQ Super Compression 1.2.7 Stable

## 中文

- Limiter 可与 Classic / Super、Single / Dual、ST / MS / LR 配合使用；下压 Ratio 为 20:1 至 1000:1，上压仍保留。
- 独立 Limiter 参数组，关闭 Limiter 恢复普通压缩的 Ratio、边界与 Output；Limiter 下的 Output LINK 联动下压阈值与输出，不联动上压门槛。
- 最终 Ceiling 默认 0 dB，范围 -24 至 0 dB，支持 Alt 单击复位、数值输入、拖动及撤销。
- TP 按钮默认开启，可切回采样峰值保护；只在 Limiter 生效。最终保护采用已接受的 Adaptive Ceiling。TP 表测量处理后的输出，保留 20 秒 Hold 与双击清零。
- Output 左侧耳机提供 1:1 监听：保留完整限幅结果，在 Ceiling 后抵消可见 Output Gain。MON 显示对应的监听上限；耳机状态随工程保存，独立于 A/B。
- 耳机模式 MATCH 比较原始输入与实际限幅、监听补偿后的输出；LR/MS 共用补偿，保留相对平衡，并计入干湿混合。耳机与 Bypass 采用约 20 ms 平滑切换，TP 切换保留约 10 ms 过渡。
- 普通压缩 Lookahead 默认起点仍为 26 ms；最终峰值保护需要额外的固定延迟，模式切换时保持延迟一致。Windows 验证详情见 VERIFICATION.md。

中英文说明书各 35 页，保留配色、字体与 Classic / Super 曲线，新增完整 Limiter 操作章节。提供 Windows x64 VST3、Mac Apple Silicon / Intel VST3 与 Universal 2 AU；macOS 11+，临时签名、未经公证。

正式插件身份、已有参数 ID 保留。旧工程缺少新参数时 Limiter 默认关闭；现有普通处理经延迟对齐验证与 1.2.6 一致。最终保护额外固定缓冲约 7-8.5 ms，48 kHz 下约 8.17 ms，开关 Limiter / TP 时保持不变。新版监听不是自动持续响度控制；MATCH 仍需有效音频积累。

已接受的 Windows 回归包含 120 组普通处理逐位一致检查、12 组实际 Limiter VST3 对照、18 组耳机增益关系与 12 组 MATCH 对照。对应误差上界和条件见 VERIFICATION.md。TP 保留少量重建余量，不要求每次读数恰好贴住 Ceiling，也不承诺任意音频零失真。

## English

Limiter mode combines the existing Classic / Super dynamics with an adaptive final Ceiling, a True Peak switch enabled by default, and a separate limiter parameter bank. Downward ratios span 20:1–1000:1; upward processing remains available.

A headphone button beside Output Gain cancels the visible output gain after the complete Ceiling stage. The normal linked settings remain active, MON shows the converted listening ceiling, and the headphone selection is project-local and independent of A/B. In this mode MATCH measures aligned original input against actual monitored output, with common domain correction and dry/wet contribution accounting. Headphone and bypass transitions are smoothed.

The verified Windows binary is promoted unchanged. Matching Chinese and English manuals each contain 35 pages, retaining the established design and Classic / Super curve while adding the full Limiter workflow. Packages cover Windows x64 VST3, Mac Apple Silicon / Intel VST3 and Universal 2 AU. Mac requires 11 or newer; bundles are ad-hoc signed, not notarized.

The formal identity and existing parameter IDs are retained. Old states without new parameters load with Limiter off. Validated normal audio matches 1.2.6 after delay alignment. Final protection adds about 7-8.5 ms of fixed buffering, approximately 8.17 ms at 48 kHz, retained across Limiter / TP switching. Monitoring is not continuous automatic gain control; MATCH still requires valid audio measurement.

Accepted Windows checks include 120 bit-identical normal-processing cases, 12 actual Limiter VST3 comparisons, 18 headphone gain-relationship cases and 12 MATCH cases. See VERIFICATION.md for bounds and conditions. TP retains a small reconstruction margin; readings need not exactly touch Ceiling, and arbitrary audio is not guaranteed distortion-free.
