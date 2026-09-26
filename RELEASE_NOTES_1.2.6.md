# QQ Super Compression 1.2.6

## 中文

本次发布整合 1.2.3 之后的改进，并提供新版 28 页中英文手册。

- **Classic / Super**：Classic 使用固定 dB Ratio；Super 使用原有曲线。记住最后手动选择，已有工程恢复保存的模式，A/B 可比较两种算法。
- **阈值**：Classic 最低 −90 dB，到底保持同一算法；Super 保留 −inf。上压处理下门槛以上的有效部分；Single Range 和 Dual 的上下交接保留。
- **切换**：A/B 在相同检测、输入和延迟配置下，对包含 Makeup、Mix 与 Output 的完整动态结果交叉淡变。直接算法切换及 Dual 分支开关也有淡变。
- **MATCH**：取消绝对 −70 LUFS 检测门限，Makeup 支持 ±120 dB，改善深度压缩后的匹配。静音无法产生有效匹配，仍需有效音频积累。
- **Lookahead**：Classic / Super 使用对齐的过去与未来峰值检测，改善弱音之后将出现强音时的提前凹陷。默认仍为 26 ms，无额外延迟、无新增 Attack / Release 控件。
- **Ratio 与 LINK**：Single 1:1000～1000:1；Dual Up 1:1000～1:1，Down 1:1～1000:1，默认均为 1:1。相对 Ratio LINK、反向 Input/Output LINK 保留；I/O LINK 默认开启并记住上次选择。
- **新手册**：中英各 28 页，涵盖算法、Range、Dual、LINK、MATCH、A/B、Display 与预读边界，保留原有文字与配色风格。

新版检测已通过 64 组实际 VST3 阶跃测试；20–6000 Hz 的稳态测试没有明显新增 H2–H7。一些快速调幅信号的残差会增加：已接受的最严苛 Classic 下压测试增加约 4.09 dB。这个指标是调制残差，不是稳态 THD；本版不承诺任意信号零失真或每个局部输出都高于阈值。

正式 VST3 名称与身份保持不变，已有工程会使用新的检测时序，声音可能与 1.2.5 不同。Preview 使用独立身份，工程中需手动换成正式插件并核对设置。未发布独立的 1.2.4/1.2.5 下载，本版包含它们已确认的改进。

提供 Windows x64 VST3、Mac Apple Silicon VST3、Intel VST3 和 Universal 2 AU。macOS 11 或更新；Mac 使用临时签名，未经 Apple 公证。选择与宿主进程架构匹配的包。禁止商业使用，完整对应源码见本版本 GitHub tag；第三方许可保持原样。

## English

This release collects the improvements since 1.2.3 and includes new 28-page Chinese and English manuals.

- **Classic / Super**: Classic uses a fixed dB ratio; Super retains the original curve. The last manual selection is remembered, sessions restore their saved algorithm, and A/B can compare both.
- **Thresholds**: Classic bottoms out at −90 dB without changing laws. Super retains −inf. Upward processing remains above its lower gate, with the existing Single Range and Dual handoff.
- **Switching**: With matching detector, input and latency settings, A/B crossfades the complete dynamics result including Makeup, Mix and Output. Direct algorithm and Dual branch changes also fade.
- **MATCH**: Removes the absolute −70 LUFS gate and supports ±120 dB Makeup for deep compression. Silence cannot produce a valid match; enough valid audio must still be collected.
- **Lookahead**: Both algorithms use aligned past/future peaks to improve the pre-dip before a low-to-high level step. The default remains 26 ms, with no additional latency or attack/release controls.
- **Ratio and links**: Single spans 1:1000–1000:1; Dual Up 1:1000–1:1 and Down 1:1–1000:1. Defaults are unity. Relative Ratio LINK and opposite Input/Output LINK remain; I/O LINK defaults on and remembers its last setting.
- **Manuals**: Both 28-page editions cover algorithms, Range, Dual, links, MATCH, A/B, Display and Lookahead, retaining the established visual and writing style.

The accepted detector passed 64 actual VST3 step cases and steady tests from 20 to 6000 Hz without material added H2–H7. Some rapid amplitude-modulation residuals increase, by up to about 4.09 dB in the accepted Classic downward stress fixture. This measures modulation residual, not steady-tone THD. No arbitrary-signal zero-distortion or per-sample threshold-floor promise is made.

The formal VST3 identity is unchanged. Existing sessions use the new detector timing and may sound different from 1.2.5. The separate Preview identity requires manual replacement and a settings check. Improvements developed in 1.2.4/1.2.5 are included here; they were not separate public downloads.

Available formats: Windows x64 VST3, Mac Apple Silicon VST3, Intel VST3 and Universal 2 AU. macOS 11 or newer; Mac bundles are ad-hoc signed, not Apple-notarized. Match the VST3 architecture to the host process. Non-commercial use only; corresponding source is available at this version's GitHub tag. Third-party terms remain intact.

[下载 / Downloads](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/tag/v1.2.6) · [对应源码 / Source](https://github.com/Ziqing-Gu/QQ-Super-Compression/tree/v1.2.6)
