# QQ Super Compression changelog

### 1.3.5 — Stable, 2026-10-11

- 中文：Display 新增 Scale 按钮，在 -30 / -60 / -90 dB 之间切换，普通模式与 Limiter 共用。图表、阈值推子和拖动精度同步刻度，保留实际阈值与声音；超出视野的阈值保留真实读数并以星号提示。取消 Limiter 固定 -45 dB 限制，继续移除其 Range。Scale 随工程保存，A/B 和 Limiter 切换保持一致。更新中英文 44 页说明书。修正复用 Windows 构建目录时版号资源未更新的问题。用户已确认并指定为 Stable；双语手册配套发行。
- English: Adds a shared Display Scale button for -30 / -60 / -90 dB views in Normal and Limiter. Graphs, threshold faders and drag sensitivity follow the view without rewriting actual thresholds or changing audio; out-of-view readouts retain their value and show an asterisk. Removes the fixed Limiter -45 dB restriction while keeping Limiter Range disabled. Scale persists with the project and stays consistent across A/B and Limiter changes. Updates both 44-page manuals and fixes Windows resource-version refresh in reused build directories. Promoted to Stable by explicit user instruction, with matching bilingual manuals.

### 1.3.4 — Stable, 2026-10-11

- 中文：Limiter 的 Display 与 Single / UP / DOWN 阈值统一为 -45 至 0 dB；移除 Limiter 的 Range 控件及其处理作用，普通压缩保持原行为。按用户要求保留切换 Limiter 时原有的延迟变化。中文手册重新整理 Lookahead / Window、独立 SAFE 章节、Range 的功能与操作顺序、噼啪声提示，以及上压和下压的对称关系。用户已确认并指定为 Stable；配套 44 页英文手册已完成。
- English: Limiter Display and Single / UP / DOWN thresholds now span -45 to 0 dB. Removes Limiter Range controls and processing while preserving normal compression behavior. Existing Limiter-switch latency behavior is retained at the user's request. The approved Chinese manual clarifies Lookahead / Window, SAFE, Range operation and crackles, and the upward/downward compression relationship. Promoted to Stable by explicit user instruction; matching 44-page English manual is included.


#### 1.3.4 发布说明 / Release details

- 新增 / Added: 配套 44 页英文说明书，独立讲解 Lookahead/Window、SAFE、Range 及上压/下压对称关系。 Matching 44-page English manual with separate timing, SAFE, Range and upward/downward chapters.
- 修复与行为 / Fixes and behavior: Limiter 全部阈值范围与图表统一为 -45..0 dB；不再使用 Range。旧低阈值收至 -45 dB，UP 不与 Output/Makeup 联动。 Limiter thresholds/display use -45..0 dB; Range is inactive; old lower thresholds clamp to -45 dB, and UP stays independent of Output/Makeup.
- 性能 / Performance: 本次没有额外性能功能；复用已验收的 Windows 二进制。 No additional performance feature; the accepted Windows binary is reused.
- 兼容与升级 / Compatibility and upgrade: 插件身份和已有宿主参数保留；普通压缩范围不变。保存工程副本并试听旧 Limiter 工程。 Plugin identity and existing host parameters remain; normal compression ranges stay unchanged. Save a session copy and audition old Limiter settings.
- 已知限制 / Known limits: SAFE 可能提前压及峰后压；OS 减少混叠而非所有谐波；切换 Limiter 的 Ceiling 额外延迟保留。 SAFE may cause pre/post-attenuation; OS reduces aliasing, not all harmonics; Ceiling-related latency changes on Limiter switching remain.

### 1.3.3 — Stable, 2026-10-11

- 中文：统一 Window 毫秒控件；SAFE 移至右下，0 ms 禁用。Limiter 采用完整 ST 界面、单张 Display、一组共用阈值和 Ratio/Makeup/Mix/ON，内部保留可调关联的左右独立检测；移除参数 LINK、双压 Ratio LINK 和声道 Monitor（Limiter 固定完整立体声输出），保留 0–100% 音频 L/R Link（覆盖动态及 Ceiling）。普通 LR/MS 参数 LINK 同步对应 Dual ON。MATCH 支持播放中累计匹配，点击不清空、不重复叠加补偿；停止保留，下次播放重置。ECO 缺失分析时不提供不完整播放的 MATCH。详见新版双语用户手册。
- English: Window in ms; relocated SAFE disabled at zero Lookahead. Limiter has a full ST presentation with a single history panel and common boundaries/Ratio/Makeup/Mix/enables, while retaining independent L/R detection with adjustable gain coupling, no parameter LINK, UP/DOWN Ratio LINK or channel Monitor (always stereo output), and a numeric 0–100% audio link through dynamics and Ceiling. Normal LR/MS parameter LINK pairs matching Dual enables. MATCH retains the playback capture across clicks without re-adding correction; stop retains it and restart resets it. ECO skips are not treated as a complete capture. Promoted to Stable after user acceptance.


#### 1.3.3 行为与升级 / Behavior and upgrade

- 新增 / Added: 数值 L/R Link、统一 Window 和播放累计 Match。 Numeric stereo linking, unified Window and cumulative playback Match.
- 修复 / Fixes: 普通 LR/MS 的 Mode LINK 同步 Dual ON；重复 Match 不清空记录、不叠加旧补偿。 Normal LR/MS Mode LINK synchronizes Dual enables; repeated Match retains data without stacking correction.
- 行为 / Behavior: Limiter 一组共用控制、ST Display，无 Mode/Monitor/参数 LINK/Ratio LINK；SAFE 移到右下，0 ms 禁用。 Limiter uses shared controls and ST Display without Mode, Monitor, parameter LINK or Ratio LINK; SAFE moves to the lower right and is disabled at 0 ms.
- 性能 / Performance: FULL 后台累计，ECO 关窗暂停分析；未改变音频 OS 质量。 FULL keeps measuring with the editor closed; ECO pauses analysis there without reducing OS quality.
- 兼容与注意 / Compatibility: 旧 ST Link 100%，旧 LR/MS 0%；原 L/M 参数成为共用值，可能改变旧双组/MS 工程声音。 Old ST migrates to 100% Link and LR/MS to 0%; L/M settings become shared and may change older split-channel/MS sessions.
- 已知限制 / Known limits: ECO 中缺失分析的播放需重新开始才能完整 Match；不同延迟设置切换可能触发宿主补偿。 Restart playback after missed ECO analysis for a complete Match; latency changes may trigger host compensation.

### 1.3.2 — Stable, 2026-10-09

- 中文：检测切换统一为 SAFE 按钮，位于 Limiter 右侧。新实例默认关闭；点亮为黄色 Safe，关闭为 Normal。Normal 显示 Distort 百分比，Safe 显示 Window 毫秒；切换保持实际窗口长度。更新配套中英文 43 页手册。DSP 沿用已验证的 1.3.1 后续方案。
- English: A fixed SAFE button sits to the right of Limiter, off by default in new instances. Lit yellow means Safe; unlit means Normal. Normal shows Distort percent, Safe shows Window in milliseconds; switching retains the actual window. Includes matching 43-page Chinese and English manuals. DSP retains the verified final 1.3.1 design.

### 1.3.1 — Development series consolidated into 1.3.2

- 中文：在固定 Lookahead 基础延迟内加入可调检测窗口。Normal 的 Distort 0% 对应全窗口，100% 对应零窗口；Safe 的 Window 为 0 至当前 Lookahead。手动切换 Lookahead 和 Alt 单击恢复全窗口，工程及 A/B 保留已存设置。过采样统一为 1x/4x/8x/16x，覆盖所有 Lookahead 的动态与 Ceiling；保留 1x 音频下的独立 TP 重建检测。移除 10 ms 档，旧 10/5 ms 状态迁移至 26 ms，最终档位为 0/26/40/80/100 ms。Limiter 上压阈值不再参与 Output/Makeup 联动，Super 最低 -inf、Classic 最低 -90 dB；下压和 Makeup 联动保留。修正 0 ms、Lookahead 切换及 A/B 的 Display 对齐。
- English: Adds an adjustable detector window within the fixed Lookahead base delay. Normal Distort maps 0% to the full window and 100% to zero; Safe Window spans zero to the current Lookahead. Manual Lookahead changes and Alt-click restore the full window; projects and A/B retain saved settings. Unified 1x/4x/8x/16x audio oversampling covers dynamics and Ceiling at every Lookahead, including independent TP reconstruction at native 1x audio. Removes the 10 ms preset; old 10/5 ms states migrate to 26 ms. Final presets are 0/26/40/80/100 ms. Limiter UP Threshold no longer links to Output/Makeup; its Super floor is -inf and Classic floor is -90 dB. DOWN and Makeup linking remain. Corrects Display alignment at 0 ms and across Lookahead/A/B changes.

### 1.3.0 — Detector investigation and comparison candidate

- 中文：针对带 Delay/混响素材的持续噼啪声，引入前后窗口峰值取较小值与取较大值的对比方案，并排查 1:1 Display 参考对齐。较大峰值方案以提前压及峰后压为代价，帮助抑制快速增益变化；相关候选方案在 1.3.1 中进一步整理。本系列未采用隐藏 Attack/Release 作为动态修复。
- English: Investigates persistent crackles on delay/reverb material with a comparison between the smaller and larger of past/future window peaks, alongside unity-ratio Display reference alignment. The larger-peak approach trades pre/post-attenuation for reduced rapid gain changes; the candidate designs are consolidated in 1.3.1. No hidden Attack/Release stage was introduced as the dynamics fix.

---

# QQ Super Compression 1.2.8 - 2026-09-28

## 中文

- 修复 Limiter 下 Single / Dual 双向切换时召回目标模式旧向下阈值的问题；现在接续当前 Single Threshold / Dual DOWN Threshold。
- ST、L、R、M、S 独立接续，且不受 Limiter LINK、声道 LINK 或 Ratio LINK 开关影响。
- Makeup、Mix、Output Gain、Ratio、算法选择及正常压缩的 Single / Dual 独立记忆保持不变。
- 保留插件身份、参数 ID/顺序和 state schema 22。
- Windows 完整 Limiter、Unity、LUFS 回归通过；1.2.7 与 1.2.8 的 120 组正常模式及 24 组 Limiter 实际 VST3 对照为零残差。

## English

- Fixes stale destination downward-threshold recall when switching Single / Dual in Limiter mode; the current Single Threshold / Dual DOWN Threshold now carries in both directions.
- ST, L, R, M and S continue independently, regardless of Limiter, channel or Ratio link switches.
- Makeup, Mix, Output Gain, ratios, algorithm choices and normal-mode independent Single / Dual memories remain unchanged.
- Preserves plug-in identity, parameter IDs/order and state schema 22.
- Full Windows Limiter, Unity and loudness regressions pass; 120 normal and 24 Limiter real-VST3 comparisons between 1.2.7 and 1.2.8 have zero residual.

---

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


---

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

---

# QQ Super Compression 1.2.6 Stable — 2026-09-27

User-approved Preview detector promoted to formal QQ Super Compression. Both Classic and Super use aligned past/future peaks at the unchanged 26ms default; formal identity and preferences restored. Static laws, parameters and accepted audio retained. Preview VST3 removal and Plan B requested. See [bilingual release notes](RELEASE_NOTES_1.2.6.md) for measured tradeoffs and compatibility.

---

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

---

# 1.2.5 Candidate 修订 — 2026-09-27

- 按用户要求将 Classic 最低阈值改为 −90 dB，Single / Dual、推子、读数、复位和 DSP 统一；Super 保持 −inf。
- 保留宿主参数的历史存储范围；Classic 中低于下限的旧值按 −90 dB 处理。

# QQ Super Compression 1.2.5 Candidate — 2026-09-26

- Classic 最低阈值改为有限 −120 dB，消除最低端切回 Super 曲线的跳变；Super 保留 −inf。
- A/B 在相同输入、检测及延迟配置下交叉淡变完整处理结果，20 ms S 形曲线；保留直接算法切换的 10 ms 淡变。复现截图中的倒数 Ratio 与补偿增益匹配，验证无中间音量凹陷。
- MATCH / Makeup 扩展到 ±120 dB；按用户要求取消 −70 LUFS 绝对检测门限，保留 K 加权与相对响度门控，真正零信号保持无效。
- I/O LINK 首次默认 ON，记住显式 UI 操作，工程状态优先。
- Classic / Super 新实例沿用上次手动选择；首次 Classic，工程与 A/B 状态优先。
- Candidate，未批准 Stable；无桌面交付、正式备份或远程发布，分段压缩保持不动。兼容边界见 TRY_1.2.5.md。

# QQ Super Compression 1.2.4 Candidate — 2026-09-26

- 顶部新增 ALGO: CLASSIC / ALGO: SUPER。Classic 保留 1.2.3 的固定 dB Ratio，Super 恢复 1.2.2 的原 QQ 算法。
- 10 ms 线性交叉淡变，共用已对齐的检测与音频路径，支持淡变中途反向切换，不增加延迟。
- A/B 分别保存算法，可直接比较 Classic 与 Super。算法变化经同一 Crossfade；支持工程保存、自动化和撤销。
- 历史 1.2.3 / dB 状态迁移到 Classic，1.2.2 及更早状态迁移到 Super；新实例默认 Classic。
- Windows 验证版，等待实际工程确认；尚未设为 Stable，分段压缩及发布流程继续暂停。

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

Stable与Plan B由用户明确批准。本轮包含Windows验证、替换安装及正式源码备份；未执行Plan C/D或远程发布。原PDF手册保持原样，算法与参数范围以本版补充说明为准。

## English

The accepted QQ dB Compression comparison algorithm is now the official Super Compression.

- Finite Down uses fixed dB Ratio: output=threshold+(detector-threshold)/Ratio. At-10dB/5:1, steady0dB input produces-8dB output.
- Finite Up gates still lift only above the lower threshold, with the same dB slope as reciprocal Down. Their common interior matches after constant gain compensation, away from boundary transitions.
- Corresponding-inf branches retain the legacy law. Single Range, Dual handover, lookahead and10ms branch crossfades remain unchanged.
- Retains1:1000..1000:1 ranges, unity defaults, relative Ratio Link, opposite Input/Output Link, all skins andST/LR/MS.
- Retains the accepted deep-threshold/float-precision fixes,120dB Up calculation headroom and accurate deep Down audio/Mix/meters.
- Restores the formal product name, originalQscp VST3 identity and QQSuperCompression preferences; removes the separately installed QQ dB Compression test plugin.
- Existing project parameters load; finite-threshold sound changes to the new algorithm. Independent test instances must be replaced with the formal plugin.

The user explicitly approved Stable and Plan B. This run covers Windows verification, installation replacement and the formal source backup, with no Plan C/D or remote publication. Original PDFs remain unchanged; current supplemental guides describe the accepted algorithm.


--- Historical records below ---

# 1.2.3 — QQ dB Compression local comparison test

- New independent product/VST3 identity and preference file for concurrent use with original1.2.2 Stable.
- Finite-threshold fixed dB Down; reciprocal-slope Up anchored at upper boundary. Corresponding -inf thresholds retain legacy rational curves.
- Preserve future-peak lookahead, Range/gate transitions, branch fades and controls.
- Support finite thresholds below-100dB and new Up calculations through120dB.
- Actual processor and editor regression checks, bilingual listening guides and reproducible Windows source.

Historical QQ Super Compression changes follow.

# QQ Super Compression 1.2.2 Stable — 2026-09-26

## 中文

- Single Ratio 为1:1000～1000:1，Dual Up 为1:1000～1:1，Dual Down 为1:1～1000:1。默认全部1:1；取消 Infinity 档，Single 的1:1位于中央。
- 恢复 Dual Ratio LINK 的相对联动：保留 Up×Down，开启不跳值，边界共同停止；默认开启并记住上次的用户选择。
- Input 右上方新增小型 LINK，Input/Output 按等量反向 dB 联动，保留原有偏移；默认关闭，随工程保存，A/B 共用。ST 和 LR/MS 使用相应尺寸。
- 修正扩大 Ratio 范围后联动端点的控件/处理参数不同步，以及 Display 对-100 dB低电平误判为静音的问题。
- 保留 Lookahead、原动态曲线、有限 Range 连续回归、阈值碰撞、Dual 分支10 ms Crossfade、三套皮肤及原 VST3 身份。
- 继承通过的真实处理器、真实编辑器和13项源码回归结果；本次Stable提升没有重编译或修改音频代码。
- 旧工程的绝对 Ratio 数值保留；归一化 Ratio 自动化刻度已扩展，需用工程副本验证。原中英文手册保持不变，参数范围以1.2.2说明为准。

用户批准 Stable 与 Plan B。正式备份完成状态以 STABLE_1.2.2.md 指向的外部记录为准。

## English

- Single Ratio spans1:1000 to1000:1; Dual Up spans1:1000 to1:1 and Dual Down spans1:1 to1000:1. Every Ratio defaults to unity, centred in Single. Infinity mode is removed.
- Restored relative Dual Ratio LINK preserves Up×Down, causes no jump on enable, shares travel limits, defaults ON and remembers the last user choice.
- A compact Input/Output LINK above/right of Input applies equal, opposite dB changes while preserving the existing offset. It defaults OFF, saves with projects and is shared across A/B, with ST/LR/MS sizing.
- Fixed linked-endpoint knob/processor desynchronization after the expanded range, and Display treating -100 dB detector levels as silence.
- Retained Lookahead, original dynamics curves, continuous finite-Range transitions, threshold collision rules, Dual10 ms branch fades, all themes and the original VST3 identity.
- Reused the passing actual-processor/editor checks and13 source regressions. Stable promotion does not rebuild or change production code.
- Existing absolute Ratios restore unchanged; normalized Ratio automation uses the expanded scale. Test existing automation in a project copy. Original manuals are retained;1.2.2 guides supersede their Ratio limits.

The user approved Stable and Plan B. The external completion record linked from STABLE_1.2.2.md is the backup authority.


--- Earlier records below are historical ---

# 1.2.0 Rev4 Stable — 2026-09-10

- **Range 边界修复（Rev4）**：有限 Range 内侧使用连续过渡，让动态增益在到达 Range 前回到 0 dB，修复原先边界硬跳产生的爆裂声。有限上压门槛内侧也加入连续过渡；Range OFF 保留原有向下处理曲线。这是随检测电平变化的静态增益曲线，没有新增 Attack / Release、检测窗口或延迟。
- **Dual 独立 ON/OFF（Rev4）**：Up / Down Ratio 旁各有小开关，支持 ST / LR / MS。可独立试听上压或下压，保留 Ratio、阈值和 LINK；关闭一支不会扩大另一支的作用范围。每次开关使用 10 ms Crossfade，快速反向切换从当前淡变位置继续。Display 随对应开关更新。开关默认 ON，可自动化，并随工程和 A/B 保存；旧工程缺失的开关默认为 ON。

- **Range boundary fix (Rev4):** a continuous transition inside finite Range brings dynamic gain back to 0 dB before the boundary, removing the previous hard gain step responsible for crackling. A finite upward gate also has an inner transition. Range OFF keeps the original downward curve. These are static detector-level gain curves, with no added Attack / Release, detector window or latency.
- **Independent Dual ON/OFF (Rev4):** compact switches beside Up / Down Ratio work in ST / LR / MS. Audition either branch while retaining Ratios, thresholds and LINK; disabling one does not expand the other branch's interval. Each toggle uses a 10 ms crossfade, and rapid reversals continue from the current fade position. The Display follows the branch switches. They default ON, are automatable, save with projects and A/B, and default ON when missing from older projects.

User approved Stable and B → C → D. The two completed 23-page manuals are retained unchanged. Production schema: 14.

## Earlier checkpoints (historical)

# 1.2.0 — Single / Upward / Dual — 2026-09-10

Stable Rev3 / schema 13. 中文与英文新版手册各 23 页，作为跨平台 Release 附件。
Both new 23-page manuals are included in the cross-platform Release.
完整双语变更、范围、默认值、LINK 与升级兼容性说明见 [RELEASE_NOTES_1.2.0.md](RELEASE_NOTES_1.2.0.md)。
See the linked bilingual notes for the complete change and compatibility details.

## 历史开发记录 / Historical development checkpoints

The following checkpoints retain their original scope and status.
下列开发检查点保留当时范围与状态，不表示当前发布范围。

# Current Stable — 1.2.0 Rev3 (schema13)

2026-09-10：用户确认 Rev3 并设为 Stable，执行正式 Plan B。沿用已验证的
Windows 成品，插件版本保持 1.2.0，schema 13；说明书修订留待备份后进行。
User accepted Rev3 as Stable and requested formal Plan B. The verified Windows
binary is unchanged; manual revision follows this backup. See `STABLE_1.2.0.md`.

2026-09-10. Windows Release build and compiled DSP/editor/LINK/settings tests
pass. See `EXPERIMENT_1.2.0.md` for current semantics and `VERIFICATION_1.2.0.md`
for evidence. Earlier headings below are historical, not the current spec.

- UP is a lower gate; boost only UP<p<DOWN, down-only above DOWN. UP=-inf opens
  the gate. The previous Rev2 below-UP activation was wrong and is superseded.
- All15 Ratio defaults/reset targets are1. Existing stored numbers stay intact.
- Dual Ratio LINK preserves UP*DOWN, defaults ON, remembers last click for new
  instances and respects saved project state. It is independent of domain LINK.
- Three themes share94px primary dials/216px spacing. Ratio arcs start at unity;
  Single origin.5, Dual UP origin1, Dual DOWN origin0. Mode button(380,620,60,21);
  Ratio LINK ST(278,644,36,17), LR/MS(281,636,30,14).
- Historical Rev3 source, build and output were stored in separate local directories.
- A Rev2 source checkpoint was retained before Rev3.
- Build targets QQSuperCompression_VST3, QQSCVisualCheck, QQSCDynamicsCheck.
  Actual installation status/hashes: Rev3 output `Verification/INSTALLATION.json`.
  Stable promotion and Plan B are the current scope; Plan C/D, remote publication,
  macOS and multiband work remain outside this checkpoint.

## Historical revisions

# 1.2.0 Rev2 — Range OFF and threshold-relative upward processing

2026-09-10. Current Windows experiment; plug-in version remains 1.2.0 and state
schema advances to 12. This entry describes source changes. Build, verification
and installation results must come from the Rev2 verification record.

- Range has an explicit OFF endpoint and defaults to OFF, removing the upper
  cutoff for the original downward behaviour. Finite Range remains strict:
  detector levels at or above it have unity dynamic gain. Saved finite 0 dB
  values are preserved, including schema 11 states; missing Range parameters
  migrate to OFF.
- New Dual instances start at UP = -inf / DOWN = 0 dB, with no dynamic gain
  until a threshold is moved inward. Ratio defaults remain Single 8,
  Dual UP 1/8 and DOWN 8. Existing saved Dual values are retained.
- Upward gain now uses the relative distance below its anchor:
  `g = 1 / (r + (1 - r) * p/A)`. Ratio 1/8 at 10 dB below the anchor gives
  theoretical +7.92198 dB lift at any finite active anchor. This replaces
  Rev1's upward law, which weakened as the threshold was lowered.
- The 60x21 SINGLE/DUAL switch moves to `(394, 620, 60, 21)` beside Ratio.
  Full-height fader thumbs and Display lines use shared coordinates across
  ST/LR/MS, Input Gain and external-key references, including stopped playback.
  Range OFF displays a top tag without a finite upper line.
- Boundary pushing, coincident-pair unity, hard finite boundaries and the
  existing 0 ms colouring/oversampling mode remain. Display keeps its bounded
  480-point history and shared final Output curve.
- Mode rebinding during a drag now closes the old host parameter gesture;
  linked Ratio numeric commits also notify the target parameter's gesture.
- Delivery directory: historical local working directory.
  No GitHub publication, macOS delivery, Stable promotion or multiband changes.
  Rev1 hashes, timings and installation results below are historical.

--- Historical 1.2.0 Rev1 record; not Rev2 verification ---

# 1.2.0 Rev1 Single / Dual experiment — historical build and installation

2026-09-10. Candidate only; 1.1.9 remains the Stable rollback. Windows VST3 installed after two host-closure checks, with complete build/output/install SHA-256 parity. The previous installed 1.1.9 bundle is preserved and verified in the output rollback folder.

- Source: historical local working directory.
- Build: historical local working directory.
- Output: historical local working directory.
- Installed: C:\Program Files\Common Files\VST3\QQ Super Compression.vst3
- Windows binary SHA-256: 359ACBB47019DC70ED64A9321EA6C2983CA8F4E600D6CC6FFFF15DD9A67B0C09
- Actual processor audio/state tests, actual-editor fader/ratio tests, three-theme ST/MS/LR/minimum screenshots and ten inherited source/math regressions pass. See VERIFICATION_1.2.0.md and output Verification/INSTALLATION.json.
- Single Ratio 1/32..32 default8; Dual Up Ratio1/32..1 default1/8; Dual Down Ratio1..32 default8. Dual defaults UP-24dB / DOWN-12dB. Smaller68x104 dual ST ratio controls and60x21 mode switch.
- Display median Single4.218ms / Dual4.389ms at480 points, cached software rendering.
- Known experiment limits: hard interval boundary jumps (test~12.49dB), changed old normalized Ratio automation mapping, and deferred companion notification for host-driven collisions. Use a fresh instance/session for user audition. User listening/acceptance remains pending.
- No Plan B/C/D, GitHub, macOS, Stable promotion or multiband edits. Prior frozen backups were not accessed or changed. No further rebuild/install is required unless source changes.

--- Historical entries below ---

# Changelog

## 1.1.9 — 3:2 Landscape / 横向界面 — 2026-09-08

- Plan A/B/C/D 已完成；依项目规则设为 Stable。三项 Mac 构建与 AU auval 通过，8 个 Release 附件哈希及下载地址均核验。Plan A/B/C/D complete; Stable under the standing project rule. Three Mac jobs and AU auval passed; eight Release asset hashes/download links verified. No system installation or user listening acceptance claimed.

- UI: 1200×800 default, fixed 3:2 resizing (1008×672–1800×1200); wider history, unchanged knob proportions. 界面改为 3:2，Display 更宽，旋钮保持原比例。
- Behaviour: first landscape opening uses the new default; later opens remember its size separately from legacy size preferences. 三种主题和上次主题记忆保留；横向尺寸单独记忆。
- No DSP, audio parameter, state-schema, sidechain, A/B, LINK, material-renderer or history-algorithm changes. 音频处理及功能不变。
- Both approved 18-page manuals remain intact with only a short final-page UI update. 两份手册只在结尾补充界面变化。
- Windows build, source/math regressions, real-editor renders and geometry/state checks pass. User DAW listening is not claimed. 本机自动检查不代表用户听感验收。
- Previous Stable / 上一稳定版：1.1.8 Revision 2. No intermediate versions omitted / 无遗漏版本。

## 1.1.8 Revision 2 — Clearer Light pointer — STABLE

- 2026-09-07：指针加宽、提亮、向中心延长后，用户再次明确确认当前 Rev2 为 Stable 并要求 Plan B。继承已完成的 Plan A，不重编译、安装或测试。After the pointer refinement, the user explicitly confirmed Revision 2 as Stable and requested Plan B, reusing completed Plan A. Previous Stable/rollback: 1.1.7. The earlier pause below is historical.

- 2026-09-07：用户在复制源码前暂停 Stable / Plan B，要求 Light 指针灯更宽、更亮、更易辨认；本轮不执行备份或稳定晋升。User stopped Stable / Plan B before the copy and requested a wider, brighter Light pointer; this revision remains a candidate, with no Plan B backup created.
- Light 暖白亮芯宽度提高到 Revision 1 的 2.2 倍，橙边加宽，并按用户追加要求向中心延伸约 35%；外端、刻度、光晕宽度/强度不变，0% 指示线加粗但仍不发光。Light's warm-white core is 2.2x wider, with a wider orange edge, brighter core and approximately 35% more length toward the centre. Its outer endpoint, ticks and halo width/intensity remain; the zero-position index is thicker but unlit.

- Dark 旋钮使用用户确认的实际 JUCE 样件：更明确的倒角、厚度、接触阴影和局部蓝光反射。Dark knobs use the approved JUCE material renderer with a beveled cap, visible sidewall, contact shadow and local blue reflections.
- 仅底部控制区增加固定、细密、低对比度的磨砂颗粒；Display 保持纯净近黑。Only the bottom control panel receives a stationary fine-grained graphite finish; the Display stays clean and near black.
- Light 加入细刻度，指针向内延长并加强暖白亮芯；保持原旋钮材质、灯带和 0% 无光行为。Light adds fixed fine ticks and a longer, clearer warm-white pointer, retaining the approved material, arc and unlit zero state.
- 保留 Classic、布局和全部音频功能。Retains Classic, layout and all audio behavior. Previous Stable/rollback: 1.1.7.

## 1.1.7 — Light / Dark / Classic — STABLE

- 2026-09-07：用户验收并明确设为 Stable，执行 Plan B 完整源码备份。继承已完成的 Plan A，不重新编译或安装；user accepted and promoted v1.1.7 to Stable with a complete Plan B source backup, reusing the completed Plan A. Previous Stable/rollback: 1.1.6 Revision 3. No GitHub/Actions/publishing in this promotion.

- 新增纯净近黑色 Dark，保留原 Light 与 Classic；adds a clean near-black Dark theme alongside the original Light and Classic.
- Dark 使用拟真蓝色渐进灯带、金属旋钮与低亮度按钮；Dark uses tactile metal controls and progressive ice-blue lighting, with no background mottling.
- Input 银灰、Output 橙、GR 青色统一到仪表和 Display，明确区分 Input 已亮电平与未亮底槽；matching meter/Display semantics with clearly separated Input active and inactive segments.
- 右上角循环切换三主题，记住上次选择，旧主题偏好自动兼容；three-theme cycle, persisted last choice, and legacy preference migration.
- 原布局、Light/Classic 画面、音频 DSP、参数、状态及历史计算保留；existing layout, Light/Classic appearance, audio DSP, parameters/state and history processing retained. Initially delivered as a local Plan A candidate; subsequently promoted to Stable as recorded above.

## 1.1.6 Revision 3 — LIGHT Warm Depth — STABLE

- 2026-09-07：LIGHT 大面板略微加深为暖灰，加入柔和明暗，已认可的旋钮素材和布局不变；slightly deeper warm-grey backplates with soft shading, retaining accepted knobs and layout.
- Meter 增加由浓到浅的纵向渐变，GR 按向下增长方向对应；vertical rich-to-pale meter gradients, mirrored for downward-growing GR.
- Display 原有缓存曲线使用固定电平位置的同色系渐变，不新增历史重算或大面积 GR 填充；level-aligned tonal strokes reuse cached paths, without new history processing or full-area GR fills.
- Classic、DSP、参数及主题记忆不变。Plan A 已构建、交付并覆盖安装，保留 Rev2 安装回退；Classic, DSP, parameters and theme memory retained. Plan A built/delivered/installed with Rev2 installation rollback.
- 2026-09-07：用户验收并明确设为 Stable，执行 Plan B 源码备份；user accepted and explicitly promoted Revision 3 to Stable, with Plan B source backup. Previous Stable is v1.1.5; no rebuild or GitHub publishing for this promotion.

## 1.1.6 Revision 2 — Accepted Asset Knobs / Unified LIGHT Palette — CANDIDATE

- 2026-09-07：用户验收真实 JUCE 单旋钮后，将同一材质与动态合成接入暖色主题；integrates the same real asset compositor after the user accepted its interactive JUCE study.
- 发光长度随归一化数值连续增长：0% 全灭，10% 前十分之一，50% 左半弧，100% 全有效弧；continuous normalized illumination, including a genuinely unlit zero state.
- LIGHT 的 Input 灰、Output 橙、GR 青蓝在 Meter、Display、图例和相应读数间统一；the LIGHT meters, Display traces, legends and relevant readouts share gray Input, orange Output and cyan-blue GR. Fine meter segments and ivory wells follow the reference.
- 保持原有布局、Classic、主题记忆、音频、参数、侧链和 Display 算法；existing layout, Classic, theme memory, audio, parameters, sidechain and Display algorithms are retained.
- 每个控件只缓存当前绘制帧，分辨率缓存有界；unchanged controls reuse their current frame with bounded resolution caches.
- 本机 Plan A 候选，不晋升稳定、不同步仓库；local Plan A candidate, with no Stable promotion or repository publishing.

## 1.1.6 Revision 1 — Warm Material Skin — REJECTED / HISTORICAL

- 用户否决了本次早期的浅盘式代码材质。以下保留当时实现记录，不代表当前已认可方案；the early code-shaded disk was rejected by the user. The following records the earlier implementation, not the accepted current approach.

- 2026-09-07：重做 LIGHT 暖色皮肤，加入缓存金属旋钮材质、实时灯带/指针、陶瓷按钮、内嵌数值框和细分段仪表；reworked LIGHT skin with cached satin-metal knobs, live arcs/indices, ceramic buttons, inset values and fine meter segments.
- 保持布局、功能、Classic、主题记忆、DSP、参数和 Display 算法不变；layout, functions, Classic, theme memory, DSP, parameters and Display algorithms are unchanged.
- 完整离屏 JUCE 界面截图与布局/DSP 不变性回归；actual offscreen JUCE renders and layout/DSP identity checks.
- 独立本机候选，未覆盖安装或发布；separate local candidate, not installed or published. Stable remains v1.1.5.


## 1.1.5 - Fluid/Cached Dynamic Display Rendering - STABLE
- 发布日期 / Release date: 2026-09-02
- 修复 / Fixed: 深度压缩不再触发随 GR 填充面积增长的整块半透明多边形开销；deep compression no longer increases full-area translucent polygon work in proportion to the GR band.
- 流畅度 / Fluidity: Display 由 30 Hz / 240 点提高到 60 Hz / 480 点，同时保持约八秒可见历史；Display advances from 30 Hz / 240 points to 60 Hz / 480 points while retaining the approximately eight-second window.
- 性能 / Performance: 投影数组与五条域路径预分配并缓存，`paint()` 不再重建历史容器和曲线路径；projection arrays and five per-domain paths are preallocated/cached instead of rebuilt inside `paint()`.
- 绘制 / Rendering: GR 面积改为上限 160 段的稀疏阴影路径，Display 使用完整不透明底图，减少父界面联动重绘；the GR area uses a bounded sparse shade path and an opaque Display background prevents parent invalidation.
- 兼容性 / Compatibility: GR 数学、Key Gain、HPF 重放、声音 DSP、参数、A/B 与 state schema 10 不变；GR math, Key Gain, HPF replay, audio DSP, parameters, A/B, and schema 10 are unchanged.
- 验证 / Validation: Windows x64 Release、12 项回归、BS.1770、Steinberg validator、build/output/install 哈希一致性通过。
- 状态 / Status: Plan A/B/C/D 已完成；v1.1.5 为当前 Stable 与公开 Release，v1.1.2 为上一稳定回滚基线。
## 1.1.4 - Reliable/Faster HPF Display Replay - CANDIDATE
- 发布日期 / Release date: 2026-09-02
- 修复 / Fixed: 最新一次 HPF 历史请求不再因临时快照失败而静默丢失；失败最多自动重试三次。The newest HPF history request no longer disappears silently after a transient snapshot failure and retries up to three times.
- 响应 / Responsiveness: 非鼠标稳定防抖由四个 Display tick 缩短为两个；non-mouse stability debounce is reduced from four Display ticks to two.
- 性能 / Performance: 只复制可见历史加预热，并只计算当前 ST/LR/MS 所需的两套峰值；same-machine replay core typically falls by about 55%.
- UI / Feedback: 重放期间顶部轻量显示 `HPF UPDATING`，最新结果应用后清除；a subtle header status is shown while the latest replay is pending.
- 兼容性 / Compatibility: Key Gain 继续实时；音频 DSP、参数、自动化 ID、A/B 与 state schema 10 不变。Key Gain remains real-time; audio DSP, parameters, automation IDs, A/B, and schema 10 are unchanged.
- 验证 / Validation: Windows x64 Release、11 项回归、BS.1770、Steinberg validator、build/output/install 哈希一致性全部通过。
- 状态 / Status: 仅完成 Plan A，v1.1.4 为 Candidate；v1.1.2 仍是 Stable 回滚基线。

## 1.1.3 - Sidechain Display History Replay - CANDIDATE
- 发布日期 / Release date: 2026-09-02
- 新增功能 / Added: Key Gain 现在实时重投影完整可见 detector、含 Mix 的 GR 与 Output 历史；Key Gain now reprojects the complete visible detector, Mix-aware GR, and Output history in real time.
- 行为变化 / Behaviour changes: HPF 拖动时不反复重算，松开旋钮后由低优先级线程重放十秒原始 Key 历史；HPF avoids per-drag replay and rebuilds the ten-second raw-key history on a low-priority worker after release.
- 自动化 / Automation: 非鼠标的 HPF 自动化、Preset 与 A/B 变化通过短暂稳定值 debounce 触发一次历史刷新；non-mouse HPF automation, preset, and A/B changes trigger one debounced history refresh.
- 性能 / Performance: 原始 Key 环形缓存只在编辑器打开时启用，分析率上限 48 kHz；the raw-key ring is editor-only and capped at a 48 kHz analysis rate.
- 兼容性 / Compatibility: 音频 DSP、参数、A/B 与 state schema 均未改变，schema 保持 10；audio DSP, parameters, A/B, and state schema are unchanged; schema remains 10.
- 验证 / Validation: Windows x64 Release、11 项自测、独立 BS.1770 与 Steinberg validator 全部通过；Windows x64 Release, eleven self-tests, standalone BS.1770, and Steinberg validator pass.
- 状态 / Status: 仅完成 Plan A，当前仍为 Candidate；v1.1.2 保持 Stable 回滚基线。Plan A only; v1.1.3 remains Candidate and v1.1.2 remains Stable.

## 1.1.2 - Mix-aware Dynamic Display - STABLE
- 发布日期 / Release date: 2026-09-02
- 新增功能 / Added: 可重投影的 240 点 Dynamic Display 历史、EXT post-Key-Gain/post-HPF 弱化轮廓；reprojected 240-point Dynamic Display history and a soft EXT post-Key-Gain/post-HPF contour.
- Bug 修复 / Bug fixes: 产品层面 GR 统一包含 Mix，修正旧显示将 Mix 排除在 GR 之外的问题；product-facing GR now consistently includes Mix instead of reporting core-only reduction.
- 行为变化 / Behaviour changes: 删除可见 Wet pre-Makeup 曲线；Input、Ratio、Threshold、Mix、Makeup、Output Gain 会重算可见历史。音频 DSP 不变；the visible Wet pre-Makeup trace is removed and current controls reproject visible history. Audio DSP is unchanged.
- 性能优化 / Performance: 历史保持固定 240 点并在 UI 绘制侧投影，不改变音频线程分配；history remains bounded to 240 points and is projected on the UI side without changing audio-thread allocation.
- 兼容性变化 / Compatibility: 无新增参数，state schema 保持 10；no new parameters and state schema remains 10.
- 已知问题 / Known issues: Steinberg validator/pluginval 不可用；Cubase 试听、侧链路由和 UI 仍需用户人工复核；validator/pluginval is unavailable and Cubase listening, sidechain routing, and UI checks remain manual.
- 升级注意 / Upgrade notes: 工程无需迁移；仅 Display/GR 可视解释改变。Plan C 的 Windows 成品复用本机 Plan A，macOS 由 v1.1.2 标签手动触发 Actions；no project migration is required; only Display/GR interpretation changes. Plan C reuses the local Plan A Windows build and dispatches macOS Actions from tag v1.1.2.

- Replaced immutable Wet/Output display history with raw Input plus actual future-window detector history, then reprojects the entire visible window from current parameters.
- Product-facing Gain Reduction now includes Mix in the linear gain domain: 0% Mix = 0 dB effective GR; 100% Mix = full core GR.
- The right-side GR meter, two-second Hold, and Display history use the same Mix-aware effective GR. Makeup and Output Gain remain excluded from GR.
- Removed the visible Wet pre-Makeup trace. Display now shows Dry/Input, a Mix-aware GR band/boundary, and projected Output post-Mix.
- EXT mode adds a deliberately soft two-stroke contour for the actual post-Key-Gain/post-HPF future-window detector key; disabled external buses show N/A rather than a false signal.
- Input, Ratio, Threshold, Mix, Makeup, and Output Gain immediately reproject all visible history in ST/LR/MS. LIGHT and CLASSIC retain identical function and layout.
- No parameter/state-schema change; schema remains 10. Plan B completed on 2026-09-02, making v1.1.2 Stable by the project standing rule; v1.1.1 is the previous Stable rollback.
- Windows x64 Release VST3, ten Python/source/math regressions, standalone BS.1770, version metadata, and build/output/install hash parity pass. Validator remains unavailable; Cubase listening/UI checks remain user validation.


## 1.1.1 - Side Chain HPF - STABLE
- 发布日期 / Release date: 2026-09-02
- 新增功能 / Added: 默认 OFF、20-500 Hz 的检测器 HPF；default-OFF 20-500 Hz detector HPF.
- Bug 修复 / Bug fixes: 无独立旧 Bug；本版补充用户确认需要的频率选择性侧链控制；no separate legacy bug; this version adds the requested frequency-selective detector control.
- 行为变化 / Behaviour changes: EXT 顺序为 Key Gain → HPF，滤波后 Key 进入 detector、Key Level 与 SC LISTEN，主载波不受滤波；EXT order is Key Gain → HPF, feeding detector, Key Level, and SC LISTEN while leaving the carrier unfiltered.
- 性能优化 / Performance: 二阶 Butterworth 系数平滑更新，OFF 走原全频检测；smoothed second-order Butterworth coefficients with exact legacy full-band behaviour when OFF.
- 兼容性变化 / Compatibility: 新增可自动化 `keyHpfHz`，state schema 9 → 10；legacy state/A-B 自动迁移到 OFF；adds automatable `keyHpfHz`, advances schema 9 → 10, and migrates legacy state/A-B to OFF.
- 已知问题 / Known issues: validator/pluginval 不可用；宿主侧链、自动化与 SC LISTEN 仍需人工复核；validator/pluginval is unavailable and host-side sidechain, automation, and SC LISTEN checks remain manual.
- 升级注意 / Upgrade notes: 旧工程听感保持全频检测，用户需要时再主动开启 HPF；legacy projects retain full-band detection until HPF is explicitly enabled.

- Added an automatable/persistent Side Chain HPF parameter with a true OFF default and logarithmic 20-500 Hz range.
- Uses a second-order Butterworth high-pass on the selected detector key only. INT and EXT are supported; EXT order is Key Gain -> HPF.
- The post-HPF key feeds detector processing, Key Level metering, and latency-aligned SC LISTEN; the audible carrier remains untouched.
- OFF preserves the v1.1.0 full-band detector. Old projects and A/B snapshots explicitly migrate to OFF.
- HPF participates in project state, host automation, Undo/Redo, and A/B. State schema 9 -> 10.
- Side Chain popup expands leftward from 230x146 to 330x146 without moving the main 1020x820 layout; LIGHT and CLASSIC retain identical features and geometry.
- Windows x64 Release VST3, nine Python/source/math regressions, standalone BS.1770, version metadata, and build/output/install hash parity passed. Validator remains unavailable and Cubase listening/UI checks remain user validation.
- Plan A and Plan B completed on 2026-09-02. Under the project rule v1.1.1 became Stable at that time; v1.1.2 is now current Stable and v1.1.1 remains the previous Stable rollback.

## 1.1.0 — External Key / Sidechain — STABLE
- 发布日期 / Release date: 2026-09-02
- 新增功能 / Added: 可选 mono/stereo 外部侧链、INT/EXT、Key Gain、Key Level 与 SC LISTEN；optional mono/stereo external sidechain, INT/EXT selection, Key Gain, Key Level, and SC LISTEN.
- Bug 修复 / Bug fixes: 无独立旧 Bug；本版实现新的外部检测源工作流；no separate legacy bug; this version introduces the external detector-source workflow.
- 行为变化 / Behaviour changes: EXT 只替换 detector，不进入正常输出；未连接或静音 EXT 为 0 dB GR，主载波保持可听；EXT replaces only the detector, never the normal output; missing or silent EXT yields 0 dB GR while the carrier remains audible.
- 性能优化 / Performance: 保持既有 future-window Peak/Lookahead 核心和固定域路由，不增加 Attack/Release 包络器；retains the existing future-window Peak/Lookahead core and fixed domain routing without adding an Attack/Release envelope.
- 兼容性变化 / Compatibility: 新增 `keySource` 与 `keyGainDb`，state schema 8 → 9；旧工程迁移到 INT / 0 dB；adds `keySource` and `keyGainDb`, advances schema 8 → 9, and migrates legacy projects to INT / 0 dB.
- 已知问题 / Known issues: validator/pluginval 不可用；实际宿主的侧链总线路由和 SC LISTEN 需人工复核；validator/pluginval is unavailable and real host sidechain routing plus SC LISTEN require manual checks.
- 升级注意 / Upgrade notes: 旧工程默认继续使用 INT；SC LISTEN 不保存、不自动化、不进入 A/B，并在关闭面板/编辑器时复位；legacy projects default to INT. SC LISTEN is not saved, automated, or included in A/B, and resets when the panel/editor closes.

- Added an optional mono/stereo VST3 sidechain input and `SC: INT / SC: EXT` source selection.
- INT preserves the v1.0.4 post-Input-Gain main detector path; EXT changes only the detector source.
- Added dedicated automatable/persistent `Key Gain` (-24…+24 dB), included with Key Source in A/B snapshots.
- Added a theme-aware floating sidechain panel with source buttons, Key Gain, safety `SC LISTEN`, and compact key meter; LIGHT/CLASSIC geometry and features are identical.
- External mono drives ST, both LR detectors, and both M/S detectors in common; stereo external key maps to L/R and M/S normally.
- Disconnected or silent EXT produces unity gain reduction and never silences the carrier.
- External key follows the existing future-window Peak/Lookahead, Ratio, and Threshold law with no Attack/Release envelope.
- `SC LISTEN` is final-audible-only, latency-aligned, excluded from automation/project state/A-B, ignored by true Bypass, and reset OFF when the panel/editor closes or state is restored.
- State schema `8 -> 9`; old projects explicitly migrate to INT / 0 dB.
- Local JUCE 8.0.15 / MSVC Windows x64 Release VST3 build and eight Python/source/math tests plus BS.1770 passed. Steinberg validator is unavailable in this environment; Cubase audio/UI validation remains required.
- User explicitly promoted v1.1.0 to the Stable baseline on 2026-09-02; Plan A and the formal Plan B are complete. v1.0.4 is the previous Stable rollback baseline. The Cubase checklist remains manual follow-up, and no Steinberg validator/pluginval pass is claimed.

## 1.0.4 — Light / Classic UI switch — STABLE

- 用户于 2026-09-01 明确将 v1.0.4 提升为当时的稳定基线，并确认本项目完成 Plan B 的版本即记录为 Stable；当前 Stable 已推进至 v1.1.2。
- 右上角新增 `LIGHT` / `CLASSIC` 视觉主题切换；重新打开编辑器时恢复本机上次选择。
- 主题不进入宿主自动化、A/B、DSP 或工程状态；两种主题保持同一 1020x820 布局、全部控件和声音功能。
- LIGHT 保留暖色 ivory 界面；CLASSIC 恢复克制的深 charcoal / cyan 控件语言与更简洁的旧版控件绘制。
- 本机 Windows x64 VST3 Release 构建、系统安装 bundle 一致性和七项回归测试通过。当前环境未安装 Steinberg validator，因此此稳定基线明确保留 validator 验证缺口。

- The user promoted v1.0.4 to the Stable baseline on 2026-09-01 and confirmed the project rule that completing Plan B records that version as Stable; the current Stable has since advanced to v1.1.2.
- Added an upper-right `LIGHT` / `CLASSIC` visual-theme switch.
- The choice is restored from local UI settings on later editor opens; it is not part of host automation, A/B or DSP/project state.
- Light preserves the approved warm ivory UI. Classic restores the restrained dark charcoal / cyan control language and simpler legacy control drawing while preserving the current 1020x820 layout and every feature.
- Local Windows x64 VST3 Release build and installed-bundle parity passed. Steinberg validator is not installed in this Codex environment, so validator status remains an explicitly recorded validation gap for this user-promoted Stable baseline.

## 1.0.3 — Centered Domain Monitor — STABLE

- Built directly from the user-confirmed **v1.0.2 Stable** baseline.
- Enabled `FORMATS VST3 AU` so the documented Universal 2 AU target is generated; this is build configuration only, not a DSP or UI change.
- Added LR `ALL/L/R` and MS `ALL/M/S` audition Monitor; ST hides Monitor.
- L/R centered audition copies the selected channel to both outputs at `1/sqrt(2)` (-3.0103 dB).
- M centered audition uses `M=(L+R)/2` at unity; **no** additional -3.01 dB.
- S centered audition uses `S=(L-R)/2`, copied to both outputs at `1/sqrt(2)`, matching the mature QQ ChainScope Mixboard centered-Side convention.
- Monitor is final audible-only: Display/Meter/Match and the actual compression/Mix/Output Gain result remain pre-monitor.
- Monitor is project-persistent workflow state, excluded from APVTS host automation and A/B snapshots. LR and MS remember separately. State schema `7 -> 8`.
- UI adds a compact three-button Monitor row without reducing the 550 px Display/Meter row. Mode/Lookahead remain 108x23; LINK geometry is unchanged.
- Existing regression tests plus `monitor_audition_selftest.py`: PASS. Plan A passed JUCE 8.0.15 / MSVC Windows x64 Release, BS.1770, and Steinberg validation. Plan D passed Actions for Windows x64 VST3, macOS Apple Silicon VST3, macOS Intel VST3, and Universal 2 AU; the AU passed `auval`.

## 1.0.2 — Complete Relative LINK — STABLE

- User explicitly promoted v1.0.2 to the new Stable baseline on 2026-08-30.
- v1.0.1 remains the previous Stable rollback point.
- Based directly on the user-confirmed **v1.0.1 Stable** baseline (Plan A/B/C/D completed).
- LINK now covers all four LR/MS paired controls: **Ratio / Threshold / Makeup / Mix**.
- Mix uses the same relative-delta rule as the existing pairs; e.g. `100% / 70%` moved by `-10` becomes `90% / 60%`.
- Direct numeric entry now participates in LINK for Ratio, Threshold, Makeup and Mix instead of bypassing the linked gesture logic.
- Typed values use the same shared-boundary clamp as dragging/Shift fine adjustment: when either member reaches a legal limit, both stop while preserving the current offset.
- Threshold OFF retains the established `-inf` semantics: one-OFF/one-finite pairs do not invent a finite dB offset; both-OFF may be brought out together.
- No DSP, detector, Ratio law, Lookahead, Oversampling, PDC, Display, Meter, LR/MS signal routing, state schema or A/B sound-snapshot behaviour is changed.
- Version metadata updated to `1.0.2`; user subsequently built/verified and promoted this revision to Stable.

## 1.0.1 — Transparent Core / Independent Domains / Display-First — STABLE

- User promoted the Display `0…-90 dB` v1.0.1 revision to the new Stable baseline on 2026-08-30 and reported Plan A/B/C/D completed.
- The historical Candidate revision notes below are preserved unchanged.

## 1.0.1 — Transparent Core / Independent Domains / Display-First — Candidate

### Candidate revision — Mode / Lookahead alignment polish

- User-approved compact technical-column layout: the Mode cycle button and Lookahead ComboBox now use the **same 108 x 23 design-pixel rectangle** and exactly the same horizontal alignment.
- LINK is no longer allowed to steal width from Mode. It is a separate **34 x 23** auxiliary button placed 6 px to the right of Mode and remains hidden in ST / visible in LR-MS.
- Mode remains a click-to-cycle button (`ST -> MS -> LR -> ST`); Lookahead remains a ComboBox because it has many choices.
- Oversampling uses the same primary-control alignment as Lookahead when visible at 0 ms.
- No DSP, parameter, Threshold, Mix, Display, Meter, LINK semantics, PDC or project-state behaviour changed.
- Static geometry checks and existing Python self-tests pass; real JUCE/MSVC/Cubase visual verification is still required.

### Candidate revision — Mode/LINK visibility + independent LR/MS Mix

- Fixed the user-reported missing Mode control: Mode/LINK bounds are now owned only by `resized()`; timer-driven `updateModeUi()` no longer mutates button geometry. Mode remains visible in ST/LR/MS and LINK appears in LR/MS.
- LR/MS Mix is now independent per domain: L/R Mix in LR, M/S Mix in MS; ST retains the legacy shared `mix` parameter.
- Equal M/S Mix values are mathematically equivalent to the previous shared post-decode Mix. Unequal values are applied in the M/S domain before decoding, preserving true component independence.
- New LR/MS Mix parameters are appended for project compatibility; older states and A/B snapshots migrate the legacy shared Mix into all missing domain Mix values.
- LINK remains exactly the agreed three-parameter workflow link (Ratio / Threshold / Makeup); Mix is not silently added to LINK.
- LR/MS Threshold faders are now stacked top/bottom to mirror the stacked L/R or M/S Dynamic Displays instead of appearing side-by-side.
- Display dimensions and transparent future-window DSP are unchanged from the prior 1.0.1 Candidate.
- Added `independent_mix_selftest.py`; math/source tests pass. Real JUCE/MSVC/Cubase verification is still required.

- Rejected and removed the v1.0.0 Direct/Analytic/Hilbert DSP path after user testing showed no practical value, measurable harmonic colour and very high ASIO Guard/CPU cost.
- Restored the cleaner v0.9.4/v0.9.7 future-window peak / Lookahead core and the established 0 ms-only `1x / 8x / 16x` Oversampling logic.
- Threshold remains a lower boundary around the old QQ law; Threshold OFF executes the exact pre-Threshold equation.
- Retains v1.0.0 domain features: ST one Ratio/Threshold/Makeup; LR and MS independent paired Ratio/Threshold/Makeup values.
- ST now derives linked gain from the stronger current L/R window peak using the ST Ratio/Threshold, so hidden LR Ratio values cannot affect ST.
- Retains one relative LINK button for Ratio/Threshold/Makeup. LINK preserves offsets and stops both members at pair boundaries.
- Restored Oversampling UI at 0 ms; moved LINK to the Mode row so both controls can coexist without overlap.
- Enlarged design root from `1020x670` to `1020x820`; visual row is now `550` design px and lower controls `140` px.
- Narrowed Meter/Threshold side strips slightly to give the Dynamic Display more horizontal room.
- Active Threshold lines now show their numeric dB values directly inside the Display.
- Added `transparent_core_selftest.py`; archived the rejected v1.0.0 analytic self-test under `tests/archive/`.
- Static/math tests pass; real JUCE VST3 build/Cubase/PluginDoctor validation is still required.

## 1.0.0 — Analytic Sample Mapping / Independent LR-MS Domains / Relative Link — REJECTED EXPERIMENT

- Promotes the analytic-amplitude/sample-reconstruction experiment to the user-requested `1.0.0` Candidate version number.
- Lookahead remains a pure post-map delay and no longer determines an analysis window.
- ST keeps one Ratio / Threshold / Makeup.
- LR adds independent L/R Ratio and Threshold alongside the existing independent Makeups.
- MS adds independent M/S Ratio and Threshold alongside the existing independent Makeups.
- Added one LR/MS `LINK` state covering Ratio, Threshold and Makeup pairs. LINK preserves the existing numerical difference; it never equalises values.
- Linked movement stops both controls when either member reaches its legal boundary.
- Threshold OFF is treated conceptually as `-inf`; if only one side starts OFF, that side remains OFF for that gesture.
- LR/MS Dynamic Display now uses two stacked domain histories, each with its own Dry/Input, Wet pre-Makeup, Output post-Mix and Threshold line.
- Keeps the enlarged 405 px visual row / compact 145 px lower control row.
- Added migration: old common Ratio/Threshold values copy into missing LR/MS domain parameters.
- Added `domain_link_selftest.py`; corrected harmonic-projection leakage in the analytic self-test.
- Known Candidate limitation: the 4095-tap Hilbert prototype is expensive and is less accurate at 20–50 Hz than at 100 Hz+.
- Not yet a Stable release; requires JUCE/MSVC build, Cubase and PluginDoctor verification.

## 0.9.4 — Editable Numeric Text Contrast — Candidate

- Fixed unreadable white text when double-clicking a slider value for direct numeric entry on the light ivory UI.
- Explicitly styled JUCE `Label` edit-state and internal `TextEditor` colours: dark warm text, ivory background, warm focus/caret, and a readable warm selection highlight.
- No layout, parameter, DSP, Input/Output Gain, Display, meter, state, A/B, Undo/Redo, Lookahead, Oversampling, PDC, LUFS Match or GR Hold behaviour changed.
- Candidate only; requires Codex/MSVC build and a quick DAW direct-entry visual check.

## 0.9.3 — UI Rollback / Features Retained — Candidate

- Rejected the v0.9.2 runtime bitmap-filmstrip knob appearance after real user/DAW visual testing.
- Restored the v0.9.1 `UTF8LookAndFeel` vector rotary rendering and warm-light/material style.
- Removed the active `QQSCAssets` BinaryData target/link from CMake; rejected PNG assets remain preserved as development history only.
- Kept v0.9.2 Input Gain / Output Gain parameters, signal flow, Display semantics, meters, A/B, project state and Undo/Redo unchanged.
- Kept Input/Output as smaller secondary trims in the bottom control row; Ratio / Makeup / Mix remain the visual focus.
- No Ratio / Lookahead / Oversampling / PDC / LUFS Match / GR Hold DSP redesign.
- Candidate only; requires Codex build and user visual/functional verification.

## 0.1.10 — 0 ms-Only 1x/8x/16x Oversampling / Design Documentation — Candidate

- Finalised Oversampling around the user-verified need instead of exposing a generic quality menu at every Lookahead.
- Lookahead `0 ms` now alone exposes Oversampling; the UI is a single button cycling `1x -> 8x -> 16x -> 1x`. The remembered default is `8x`.
- Lookahead `10 / 26 / 40 / 80 / 100 ms` forces the DSP to `1x` and hides the Oversampling control while preserving the user's last 0 ms choice.
- Removed user-facing `2x` and `4x` choices intentionally: user PluginDoctor tests found aliasing remained severe at both factors, while the added latency of `8x`/`16x` was small enough that the intermediate factors had little practical value.
- Added a `16x` maximum-quality linear-phase FIR Oversampling path with integer latency compensation.
- Host PDC / compensated Dry / Bypass continue to use `Lookahead + active FIR latency`; non-zero Lookahead returns to Lookahead-only latency because Oversampling is effectively 1x.
- New/legacy state default for the remembered 0 ms choice is `8x`. v0.1.9 migration maps old `1x -> 1x` and old `2x/4x/8x -> 8x`. A/B snapshots and Undo/Redo keep the remembered 0 ms choice.
- Replaced the v0.1.9 Oversampling ComboBox with a compact click-to-cycle button that is hidden outside 0 ms.
- Added `PRODUCT_DESIGN_NOTES.md` explaining the core product motivation: compress dynamics while avoiding unwanted conventional Attack/Release reshaping of transients/onsets on sources such as guitar, vocals, piano and bass.
- Added `OVERSAMPLING_DESIGN_NOTES.md` documenting the measurement history and why 2x/4x are deliberately absent.
- Ratio law, strict Integrated LUFS Match, six Lookahead presets, ST/MS/LR topology, Makeup/Mix, two-second GR Hold, uniform UI scaling and internal product identity remain unchanged.

## 0.1.9 — FIR Oversampling / PDC Alignment / Warning Cleanup — Candidate

- Added user-selectable `OVERSAMPLING`: `1x / 2x / 4x / 8x`, default `1x`.
- `2x / 4x / 8x` use maximum-quality linear-phase half-band FIR oversampling with integer latency compensation; `1x` uses the dummy path.
- Oversampling encloses the future-window detector, Ratio smoothing and Ratio gain application. Lookahead keeps the same six millisecond meanings; internal Lookahead samples are the rounded base Lookahead samples multiplied by the selected factor.
- Downsampling carries simultaneous ST linked L/R, LR independent L/R and MS M/S pre-Makeup Wet streams so strict Integrated LUFS Match can continue accumulating every mode domain in parallel.
- Host latency/PDC now reports `Lookahead + Oversampling FIR latency`; the host-rate Dry/Mix/Bypass path uses the same combined delay.
- Oversampling is stored in project state and A/B snapshots and participates in normal APVTS Undo/Redo. New instances and 0.1.8-or-earlier states with no Oversampling field explicitly migrate to `1x`; no last-used Oversampling preference is introduced.
- Added `OVERSAMPLING` ComboBox beneath Lookahead without redesigning the rest of the 1020x670 uniform UI. Panel metadata now reports `v0.1.9`.
- Renamed the local editor constrainer pointer to `editorBoundsConstrainer`, addressing the non-fatal `constrainer` name-shadow warning reported from the 0.1.8 build.
- Ratio law, six Lookahead presets, 0 ms flavour semantics, strict LUFS Match definition, Makeup/Mix topology, two-second GR Hold and product/internal identity remain otherwise unchanged.

## 0.1.8 — GR Hold Readability / Uniform 1:1 UI Scaling — Candidate

- Removed the redundant `H` prefix from the Gain Reduction Hold numeric readout and increased the Hold value font from 7.5 px to 8.5 px; the white Hold marker and 2-second Hold algorithm are unchanged.
- Moved all plug-in widgets into one fixed 1020x670 design-space root and uniformly scale that root for user resizing, so fonts, meters, knobs, strokes and spacing all grow/shrink by the same X/Y factor.
- Added a fixed 1020:670 editor aspect-ratio constrainer. Width and height can no longer be stretched independently.
- Existing saved non-proportional editor sizes are migrated to the largest uniform scale that fits within the previously saved rectangle; subsequent saved sizes remain proportional.
- Kept 0.1.7 GR Hold timing, 0.1.6 strict LUFS Match, future-window peak compression, Ratio, Lookahead/PDC, ST/MS/LR, A/B, Makeup, Mix and all audio DSP unchanged.
- CMake/JUCE version bumped to `0.1.8`; Project/parameter/state schema is unchanged.

## 0.1.7 — Auto GR Peak Hold / Version Tag / Warning Cleanup — Candidate

- Added a meter-only 2 second automatic Gain Reduction Peak Hold for both displayed GR channels/components.
- A deeper GR peak updates the Hold immediately and restarts the 2 s timer; after 2 s without a deeper peak the Hold automatically refreshes to current GR.
- ST linked mode holds the linked GR; LR holds L/R independently; MS holds M/S independently.
- Added a small horizontal Hold marker plus a subdued `H` value without changing current GR meter movement/orientation.
- Mode or Lookahead changes clear stale GR Hold values.
- Renamed the local host playhead pointer from `playHead` to `hostPlayHead` to remove the non-fatal name-shadow warning reported by the user's 0.1.6 build.
- Added a small, low-contrast panel version label sourced from `JucePlugin_VersionString`.
- Strict LUFS Match, future-window peak Ratio DSP, Lookahead presets/PDC, A/B, Makeup, Mix and processing modes are unchanged.
- Candidate only; requires Codex build and Cubase visual/timing verification.

## 0.1.6 — Strict Integrated LUFS Match — Candidate

- Replaced the old integrated power/RMS-equivalent Match with ITU-R BS.1770 / EBU R128 style Integrated LUFS measurement.
- Added K-weighting, 400 ms gating blocks, 75% overlap (100 ms hop), -70 LUFS absolute gate and -10 LU relative gate.
- ST Match measures stereo Dry vs linked Wet and writes one common Makeup.
- LR and MS Match use independent mono gated-LUFS calculations and write only channels/components with valid gated data.
- Measurement remains Dry vs compressed Wet pre-Makeup/pre-Mix.
- Future-window peak Ratio core, Lookahead presets, A/B, Mix, UI and processing modes are unchanged.
- Candidate only; requires Codex build and Cubase/independent loudness-meter validation.


## 0.1.5 — Fixed Lookahead Presets / Last-Choice Memory — Candidate

- Replaced the 0.1.4 arbitrary Lookahead TextEditor with a ComboBox containing exactly 0 / 10 / 26 / 40 / 80 / 100 ms.
- Kept the 0.1.4 future-window peak detector and Ratio law unchanged.
- Preserved the existing `lookaheadMs` parameter ID; old arbitrary 0.1.4 candidate values are snapped to the nearest approved preset on state restore.
- Existing instances restore their project-saved preset; new instances default to the user's last manually selected preset.
- Added per-user `lastLookaheadMs` persistence in the existing QQSuperCompression settings file.
- First-run fallback is 26 ms.
- Recorded actual user PluginDoctor findings that motivated the six presets, including 0 ms as a retained distortion/flavour option.
- Bypass/PDC still use the exact selected Lookahead delay.
- Strict LUFS Match remains pending and is intentionally not mixed into this focused change.

## 0.1.4 — Variable Lookahead Peak Experiment — Candidate

- Replaced the active fixed 20 ms rolling RMS detector after user PluginDoctor tests showed Attack/Release-like timing and residual harmonic distortion.
- Added editable `LOOKAHEAD (ms)` text field, 0.0–100.0 ms, default 5.0 ms.
- Lookahead now controls both the future analysis window and the real audio-path latency/PDC.
- Added an allocation-free sliding future-window peak detector for L/R/M/S domains.
- Bypass remains on the same delayed path and reports the same latency as active processing.
- Lookahead is stored in project state and included in A/B snapshots.
- Retained Ratio law, Makeup topology, Mix, ST/MS/LR modes, meters, Dynamic Display, window-size memory and workflow features.
- Match intentionally remains the old RMS/power prototype; strict LUFS Match is a separately confirmed pending requirement and was not mixed into this experiment.
- 0 ms is intentionally retained as a distortion-prone instantaneous comparison point.

## 0.1.3 — Workflow / A-B / Match / Independent Makeup — Candidate

- Removed the subtitle text under the plug-in title.
- Slimmed the dual-meter panel while keeping its height unchanged.
- Added persistent per-user editor size memory.
- Added A/B comparison plus A→B and B→A copy actions.
- Added JUCE UndoManager integration and Ctrl/Cmd+Z / Ctrl/Cmd+Shift+Z handling.
- Added Shift-drag fine adjustment and Alt+left-click default reset to all rotary controls.
- Replaced checkbox-style Bypass appearance with a TextButton.
- Replaced Mode ComboBox with one cycle button: ST -> MS -> LR -> ST.
- Kept ST shared Makeup; added independent L/R Makeup in LR and independent M/S Makeup in MS.
- Added integrated energy Match based on Dry vs Wet pre-Makeup over host playback; LR/MS are calculated independently per domain channel.
- Kept 0.1.2 Ratio engine, 20 ms detector, zero latency, Dynamic Display trace meanings and dual meters unchanged.

## 0.1.2 — ST/MS/LR Modes / Zero Latency / Dual Meters — Candidate

- Removed 10 ms Stable latency mode; plugin is fixed 0-sample latency.
- Removed Makeup Gate and all related UI/processing.
- Added ST / MS / LR processing modes.
- Added dual-channel Input, Output and Gain Reduction meters.
- MS mode meters show M/S instead of L/R.
- Gain Reduction meter orientation changed to 0 dB at top, growing downward.
- Dynamic Display retains Dry, Wet pre-Makeup and final post-Mix Output; Gate line removed.
- Kept 0.1.1 threshold-free level-domain Ratio engine.

## 0.1.1 — Ratio Engine Fix / Meter & UI Pass — Candidate

- Replaced rejected sample-domain waveshaper Ratio with level-domain gain control.
- Added three meter groups.
- Improved Dynamic Display layout.
- Added stronger UTF-8/CJK handling.

## 0.1.0 — Prototype — Test

- Initial Ratio / Makeup / Makeup Gate / Mix / 0-or-10-ms prototype.
- Ratio implementation was rejected because it behaved as a waveshaper, increased level with Ratio and produced severe distortion.

## 0.9.1 — Lighting & Material Refinement Candidate

- Keeps the accepted v0.9.0 layout and all v0.1.10 DSP behaviour unchanged.
- Strengthens the knob "lamp under glass" system so it is visible in a real DAW: wider soft halo, medium bloom, crisp lit arc, bright endpoint lamp, warm panel spill and reflected light inside the knob body.
- Strengthens active-button back-lighting while keeping the same warm/cyan semantic colour roles.
- Adds low-contrast panel material depth using a slight vertical ivory gradient, fine highlight rim and softer border/shadow; geometry is unchanged.
- Fixes the user-reported non-fatal MSVC C4459 naming warning by renaming conflicting `lookaheadMs` argument/local identifiers only.
- Version metadata -> `0.9.1`. Still Candidate/Test; v1.0.0 remains reserved for explicit final user confirmation.

## 0.9.0 — Warm Transparent UI Candidate

- Version line intentionally jumps from the 0.1.x development series to **0.9.0** as the pre-release UI/polish stage. v1.0.0 is reserved for the user's final release confirmation.
- Replaces the dark prototype visual language with a warm ivory / sand chassis, dark warm-grey text, apricot/orange primary accents and cyan technical/Wet accents.
- Adds a custom scalable JUCE vector LookAndFeel for soft "lamp under glass" knob/button illumination using layered low-alpha strokes rather than bitmap blur assets.
- Keeps Dynamic Display behaviour/geometry unchanged; only its background, grid, text and Dry/Wet/Output palette are updated.
- Updates the meter panel to the same light/warm material language while preserving all meter and 2-second GR Hold semantics.
- Mode presentation is the user-approved compact single cycle button (`ST -> MS -> LR -> ST`), not a dropdown.
- Adds `UI_DESIGN_NOTES.md` so future Codex/AI can understand why the UI is intentionally clean, transparent and warm, and why the earlier dark cyberpunk concept was rejected.
- **DSP unchanged from 0.1.10:** Ratio, Lookahead, 0 ms Oversampling, PDC, LUFS Match, A/B, Makeup/Mix, ST/MS/LR and GR Hold are not redesigned in this version.

## v0.9.2 — Asset Knobs / Input & Output Gain (Candidate) — 2026-08-27

- Rotary rendering architecture changed from generated vector glow to an embedded 128-frame transparent PNG filmstrip.
- 128 frames map to normalized 0..127 visual states while APVTS parameters remain continuous/high precision.
- Numeric values/units remain editable JUCE text and are not baked into knob images.
- Progressive lamp rule: lit arc accumulates from minimum toward the current pointer; frame 127 lights the full usable arc; pointer remains visible at both endpoints.
- Added Input Gain and Output Gain trims (candidate range -24..+24 dB, default 0 dB), appended after existing parameters for legacy parameter-order safety.
- Input Gain is pre-detector/pre-compression and changes compression behaviour, but Dynamic Display Dry/Input remains the original pre-Input-Gain reference.
- Output Gain is final post-Mix trim and is included in Dynamic Display Output and Output meters.
- True Bypass keeps combined PDC but bypasses Input/Output Gain and the processing path.
- Input/Output Gain are included in project state, A/B snapshots and Undo/Redo; pre-0.9.2 states migrate both trims to 0 dB.
- Existing DSP Ratio/Lookahead/Oversampling/LUFS Match/GR Hold design is otherwise unchanged.
- Button visuals remain vector/JUCE in this candidate pending separate asset approval.

## v0.9.7 — Threshold Rebuild from v0.9.4 — Candidate / Test

- Rebased the Threshold work directly on v0.9.4; v0.9.5 and v0.9.6 are rejected experimental branches and are not the new baseline.
- Keeps the v0.9.4 future-window peak detector unchanged.
- Threshold OFF maps to the exact legacy `gain = 1 / (1 + (Ratio - 1) * level)` law.
- Enabled Threshold only adds a lower operating boundary and re-anchors the same Ratio curve to unity at that boundary.
- No Attack, Release, knee, detector segmentation, hidden smoothing or monitor-output crossfade was added.
- Threshold resolution is 0.01 dB and the vertical Threshold control now has real Shift fine drag plus existing Alt reset/Undo/direct entry workflow.
- Threshold is included in project state and A/B; pre-Threshold projects migrate to OFF.
- Enlarged the Display/Meter row vertically and compacted the lower control row.


## v1.0.1 — Display 0…-90 dB Scale Polish — Candidate Revision 4

- Dynamic Display vertical view changed from the old +6…-120 dB presentation to a focused `0…-90 dB` working range.
- Grid labels are now `0 / -15 / -30 / -45 / -60 / -75 / -90 dB`.
- Values outside the visible range are only clamped for drawing; DSP, meters, loudness, Threshold state and processing are unchanged.
- This change is specifically for Threshold workflow: useful programme dynamics occupy more of the available height instead of leaving a large visually empty <-90 dB area.
