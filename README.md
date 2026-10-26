# ⚠️ 禁止商业使用 / NO COMMERCIAL USE

## Qing Audio 非商业源码共享许可证 1.0

### 本项目源码公开，但不属于 OSI 认可的开源软件

> **禁止任何商业使用。** 仅允许个人、学习、教育、研究、评估、爱好及其他非商业用途。发布原版、二进制版或修改版时，必须同时免费公开完整对应源代码，保留作者、版权和许可证声明，醒目标明原项目名称、作者、来源链接、修改者、修改日期及修改内容，并使整个修改版继续采用同一许可证。完整条款见 [LICENSE](LICENSE)。
>
> **NO COMMERCIAL USE.** Use is permitted only for personal, educational, research, evaluation, hobby, charitable, and other non-commercial purposes. Any distributed original, binary, or modified version must provide the complete corresponding source without charge, preserve authorship, copyright, and license notices, prominently identify the original project, author, source URL, modifier, date, and changes, and license the entire modified work under the same terms. See [LICENSE](LICENSE).
>
> 许可证政策变更与后续 AI 维护说明见 [LICENSE_POLICY_CHANGE.md](LICENSE_POLICY_CHANGE.md)。 / See [LICENSE_POLICY_CHANGE.md](LICENSE_POLICY_CHANGE.md) for the policy record and future AI maintenance instructions.

# QQ Super Compression 1.4.20 Stable

Qing Audio 动态处理插件，提供 Compressor、Clipper、Limiter，Classic/Super、单压/双压和内外侧链。Comp/ST 同时提供 L/R Link 与 M/S Link。Lookahead 为 0 / 25.3 / 50.6 / 76 / 100 ms，Window 在固定延迟内调节检测时长。TRANSPARENT 默认关闭，Window 或 Lookahead 为 0 时灰显。OS 为 1x/4x/8x/16x，默认 1x。

Qing Audio dynamics plug-in with Compressor, Clipper, Limiter, Classic/Super, Single/Dual and internal/external sidechains. Comp/ST provides both L/R and M/S Link. Lookahead presets are 0 / 25.3 / 50.6 / 76 / 100 ms; Window changes detection length at fixed latency. TRANSPARENT defaults off and is disabled at zero Window or Lookahead. OS offers 1x/4x/8x/16x, default 1x.

Display：框选放大，滚轮上下缩放，Shift＋滚轮上下平移，Ctrl＋滚轮左右缩放，右键恢复全图。FREEZE 点亮后停播才冻结；播放时跟随最新音频。Clipper 的柱条实时，数字和标记线保持最大压缩量。

Display: drag a rectangle to zoom, wheel to zoom vertically, Shift+wheel to pan vertically, Ctrl+wheel to zoom horizontally, and right-click to reset. FREEZE holds history only after stopping; playback follows the live edge. Clipper bars remain live while numbers and marker lines hold maximum reduction.

Adjust 只上下移动波形、刻度和阈值线，不改变声音；Shift＋滚轮直接调整同一个 Adjust。Limiter 的 CEILING 默认开启，可关闭；TP 关闭时使用采样峰值 Limiter，两种状态都支持 Auto/Tight/Smooth。Match 以原始输入对比动态 Wet，排除最终 Ceiling / Output / 监听。

Adjust moves waveforms, scale and threshold lines without changing audio; Shift+wheel edits the same value. Limiter CEILING defaults on and can be disabled; TP-off uses sample-peak limiting, with Auto/Tight/Smooth in both states. Match compares original input with dynamic Wet, excluding final Ceiling / Output / monitoring.

配套手册 / Included manuals: [中文 1.4.20](docs/manuals/QQ-Super-Compression-1.4.20-User-Manual-Chinese.pdf) · [English 1.4.20](docs/manuals/QQ-Super-Compression-1.4.20-User-Manual-English.pdf)。各 49 页，配色和版式沿用原版，内容与本版操作对应。 / 49 pages each, retaining the established design and updated for this version.

本次覆盖 1.4.17 之后的所有真实版本 1.4.18、1.4.19、1.4.20。 / Covers every actual version since 1.4.17: 1.4.18, 1.4.19 and 1.4.20.

## 下载与安装 / Download and installation

1.4.20 跨平台发布准备中，下方仍为已发布的 1.4.17 入口。 / Version 1.4.20 is being prepared; the links below still refer to published 1.4.17.

**下载 1.4.17 / Download 1.4.17:** [最新 Release / Latest Release](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/latest) · [固定版本页面 / Version page](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/tag/v1.4.17) · [历史版本 / Release history](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases)

按系统、宿主架构和格式选择。Mac VST3 只安装一套匹配宿主架构的包；AU 是独立格式。 / Choose by OS, host architecture and format. Install one matching Mac VST3 architecture; AU is independent.

- Windows 10/11 x64 · VST3: [QQ.Super.Compression.1.4.17.Windows.x64.VST3.zip](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.4.17/QQ.Super.Compression.1.4.17.Windows.x64.VST3.zip)
- macOS 11+ · Apple Silicon arm64 · VST3: [QQ.Super.Compression.1.4.17.macOS.Apple.Silicon.VST3.zip](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.4.17/QQ.Super.Compression.1.4.17.macOS.Apple.Silicon.VST3.zip)
- macOS 11+ · Intel / Rosetta x86_64 · VST3: [QQ.Super.Compression.1.4.17.macOS.Intel.x86_64.VST3.zip](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.4.17/QQ.Super.Compression.1.4.17.macOS.Intel.x86_64.VST3.zip)
- macOS 11+ · Universal 2 · AU: [QQ.Super.Compression.1.4.17.macOS.Universal.2.AU.zip](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.4.17/QQ.Super.Compression.1.4.17.macOS.Universal.2.AU.zip)
- 中文安装说明 / Chinese installation guide: [QQ.Super.Compression.1.4.17.Installation.Guide.Chinese.txt](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.4.17/QQ.Super.Compression.1.4.17.Installation.Guide.Chinese.txt)
- English installation guide / 英文安装说明: [QQ.Super.Compression.1.4.17.Installation.Guide.English.txt](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.4.17/QQ.Super.Compression.1.4.17.Installation.Guide.English.txt)
- 中文手册 1.4.14（49 页）/ Chinese manual 1.4.14 (49 pages): [QQ.Super.Compression.1.4.14.User.Manual.Chinese.pdf](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.4.17/QQ.Super.Compression.1.4.14.User.Manual.Chinese.pdf)
- English manual 1.4.14 (49 pages) / 英文手册 1.4.14（49 页）: [QQ.Super.Compression.1.4.14.User.Manual.English.pdf](https://github.com/Ziqing-Gu/QQ-Super-Compression/releases/download/v1.4.17/QQ.Super.Compression.1.4.14.User.Manual.English.pdf)

两本手册原样沿用 1.4.14（各 49 页），保留真实版号；本次 ECO 修复见更新记录。 / Both 49-page manuals are reused unchanged as edition 1.4.14. See the version history for the ECO fixes.

升级前另存工程并退出 DAW；Mac 包使用 ad-hoc 签名，未经过 Apple 公证。完整步骤和安全处理见双语安装说明。不提供 Linux 成品。 / Save a session copy and quit DAWs before upgrading. Mac bundles are ad-hoc signed and not notarized. See the bilingual guides for installation and security steps. No Linux build is provided.

**注意 / Note:** GitHub 自动生成的 `Source code (zip)` 和 `Source code (tar.gz)` 是源码快照，不是可安装插件。 / GitHub-generated source archives are source snapshots, not installable plug-ins.


## 当前稳定版 / Current stable version

**1.4.20 Stable (2026-10-27)**，由用户明确指定。Windows 使用已验证成品；Mac 从对应源码构建。 / Explicitly user-designated. Windows reuses the verified binary; Mac is built from corresponding source.

## 版本记录 / Version history

### 1.4.20 — 2026-10-27 — Stable

- 中文：Threshold、UP、DOWN 和有限 Range 上限扩展至 +24 dB，保留 -inf、Range OFF 和原有 -90 / 0 dB 默认值。Shift＋滚轮直接调整显示用 Adjust，旋钮同步，波形、刻度和阈值线一起移动，保持缩放与停播时间位置。新版中英文手册各 49 页。此版本由用户指定为 Stable。
- English: Extends Threshold, UP, DOWN and finite Range to +24 dB while preserving -inf, Range OFF and existing -90 / 0 dB defaults. Shift+wheel edits display-only Adjust, updating its knob and moving waveforms, scale and threshold lines together without changing zoom or the stopped time position. Includes matching 49-page Chinese and English manuals. This version was explicitly selected as Stable by the user.
- 兼容 / Compatibility: 普通保存值保留原 dB；旧宿主阈值 / Range 自动化须重新检查。Stored parameter values retain their dB values; review older host threshold / Range automation after the range expansion. TP / recovery DSP is unchanged from 1.4.19. No new quantified CPU claim.

### 1.4.19 — 2026-10-27 — Delivered / 已交付

- 中文：Adjust 替代 Input Gain，只平移 Display。按用户确认，旧工程 Input 增益也停用，Input / Output LINK 移除；Clipper / Limiter Output LINK 保留。全部模式和算法支持 -inf 阈值，原有默认值不变。Match 统一比较原始输入与动态压缩后、Makeup 前的 Wet，排除 Adjust、Output、Ceiling 和耳机监听。
- English: Replaces Input Gain with display-only Adjust. By user choice, legacy Input gain is also disabled in saved sessions and Input / Output LINK is retired; Clipper / Limiter Output LINK remains. All modes and algorithms support -inf thresholds without changing defaults. Match compares original input with dynamically processed Wet before Makeup, excluding Adjust, Output, Ceiling and headphone monitoring.
- 中文：Limiter 增加默认开启的 CEILING 开关。关闭时移除最终峰值保护及其额外延迟。非 TP 改为采样峰值 Limiter，TP 开关两种状态都支持 Auto / Tight / Smooth；加强原生 1x TP 极端信号保护。Clipper 不显示 LUFS-I，Match 仍可用。
- English: Adds a default-on CEILING switch in Limiter; disabling it removes final peak protection and its extra latency. TP-off now uses sample-peak limiting, with Auto / Tight / Smooth available in either TP state. Strengthens native 1x true-peak protection for extreme signals. Clipper hides LUFS-I while retaining Match.
- 兼容 / Compatibility: 非零旧 Input 和旧 TP-off 设置可能改变声音，升级前另存工程并重新试听。Non-zero legacy Input and old TP-off settings may sound different; save a session copy and audition after upgrading. Changing Ceiling state can cause host delay-compensation adjustment.

### 1.4.18 — 2026-10-26 — Delivered / 已交付

- 中文：修复 Clipper、Limiter 和 Compressor Range OFF 在超过 0 dB 时压缩量不足，同步音频、Display 和历史重算。有限 Range、上压边界和 Output LINK 保持各自规则。
- English: Fixes insufficient downward compression above 0 dB in Clipper, Limiter and Compressor with Range OFF, including audio, Display and history recalculation. Finite Range, upward boundaries and Output LINK retain their rules.
- 兼容 / Compatibility: 受影响的高电平素材会获得正确的下压量；保存工程副本后比较。Affected high-level material receives the corrected reduction; compare using a saved session copy. No separate performance feature or new control was added.

版本覆盖 / Version coverage: previous public release 1.4.17; included 1.4.18, 1.4.19, 1.4.20; missing versions: none / 遗漏：无。

升级提示 / Upgrade: Legacy Input gain is retired; review old sessions and automation. Adjust changes only the view. Match does not match the final Ceiling/headphone path. macOS bundles are ad-hoc signed, not notarized. No claim of interactive validation in every DAW. / 旧 Input 增益取消；检查旧工程和自动化。Adjust 只改变图表，Match 不测算 Ceiling / 耳机的最终结果。Mac 使用 ad-hoc 签名，未公证；不声称全部 DAW 人工验证。


### 1.4.17 — 2026-10-18 — Stable

- 中文：ECO 关闭 UI 后重新打开时，无论 FREEZE 是否点亮，都清空旧 Display 并从新音频重新绘制；修复 QQ Host 保留编辑器对象时仍显示旧波形的问题。保留缩放、界面持续打开时的停播冻结和 FULL 后台分析；缺失数据的播放轮次仍不能 Match。更新中文悬停，声音和参数规则不变。由用户指定 Stable；20 组原生窗口重开、12 组隐藏门控及完整行为约定通过，音频逐采样差异为 0。
- English: In ECO, reopening the UI clears the old Display and starts fresh regardless of FREEZE, including QQ Host's retained-editor case. Zoom, stop-time freezing while the UI stays open and FULL background analysis remain; an incomplete playback pass remains ineligible for Match. Updates Chinese hover help without changing audio or parameter rules. User-designated Stable; 20 native-window reopen cases, 12 hidden-gating cases and the full contracts passed, with zero sample difference.

### 1.4.16 — 2026-10-18 — Stable

- 中文：ECO 暂停分析后，在 FREEZE 关闭时重开 UI 会清空旧历史、重算缓存和过期结果，保留缩放，从新采集的音频开始显示。保持不完整播放不能 Match 的规则。用户确认 QQ Host 问题解决并指定 Stable；20 组重开及原有回归通过，声音差异为 0。FREEZE 点亮时的旧图保留问题由 1.4.17 修复。
- English: After ECO suspends analysis, reopening with FREEZE off clears old history, replay caches and stale results while retaining zoom and drawing newly captured audio. Incomplete passes remain ineligible for Match. The user confirmed the QQ Host fix and selected Stable; 20 reopen cases and existing regressions passed with zero audio difference. Version 1.4.17 addresses retained history when FREEZE is on.

### 1.4.15 — 2026-10-17 — Delivered / 已交付

- 中文：识别宿主仅隐藏原生外层窗口、仍保留插件编辑器的情况。ECO 在这段时间停止 Display、普通表计、LUFS 和 Match 分析；漏算的本轮播放不能用于 Match，下一轮完整的 UI 可见播放恢复。12 组 Windows 原生隐藏测试通过，声音差异为 0。重开时旧图保留的问题在后续版本继续修复；用户曾回退 1.4.14 做对比，1.4.15 未被指定 Stable。
- English: Detects a host hiding its native parent window while retaining the editor. ECO suspends Display, ordinary meters, LUFS and Match analysis during that interval. The incomplete playback pass cannot be matched; a complete UI-visible pass restores eligibility. Twelve Windows native-hidden tests passed with zero audio difference. Later versions address stale history on reopening. The user temporarily restored 1.4.14 for comparison; 1.4.15 was not designated Stable.

这三版均保留插件身份、工程参数和音频算法，未修改 QQ Host。升级前另存工程并退出宿主。Windows 原生窗口回归不能代替 macOS 的交互式宿主验证；不声称完成所有 DAW 的人工测试。

All three versions preserve plug-in identity, session parameters and audio algorithms; QQ Host itself was not modified. Save a session copy and quit hosts before upgrading. Windows native-window fixtures do not establish interactive macOS host behavior; no all-DAW manual testing is claimed.


### 1.4.14 — 2026-10-16 — Stable

- 中文：精简 Display 中文悬停，只保留框选、滚轮缩放、Shift 平移、Ctrl 横向缩放、右键恢复与 FREEZE 的常用操作。更新中英文 49 页手册，截图与本版一致。由用户指定为 Stable；声音及参数行为不变。
- English: Shortens the Chinese Display tooltip to the basic selection, wheel zoom, Shift pan, Ctrl horizontal zoom, right-click reset and FREEZE operations. Updates both 49-page manuals and screenshots. User-designated Stable; audio and parameter behavior are unchanged.

### 1.4.13 — 2026-10-16 — Stable

- 中文：框选放大更容易操作，可从 Display 周边开始拖动，超出的部分按绘图区边缘计算。由用户指定为 Stable；原有缩放、平移、FREEZE 与实时重算保持。
- English: Makes rectangle zoom easier to use: dragging can start around the Display and out-of-plot portions use the plot edges. User-designated Stable; existing zoom, pan, FREEZE and live recalculation remain.

### 1.4.12 — 2026-10-16

- 中文：新增 Shift＋滚轮上下平移 Display，保持放大比例与时间位置；同步中文悬停。Ctrl 横向缩放、播放跟随及实时响应不变。
- English: Adds Shift+wheel vertical panning while preserving zoom and time position, with updated Chinese help. Ctrl horizontal zoom, live following and realtime response remain.

### 1.4.11 — 2026-10-16

- 中文：Display 顶部也支持滚轮缩放，纵向以 0 dB 为上沿，Ctrl 横向缩放。Clipper 柱条显示实时压缩量，数字和标记线保持最大值，双击或重新播放清零。
- English: Enables wheel zoom over the Display header, with 0 dB at the top for vertical zoom and Ctrl for horizontal zoom. Clipper bars show current reduction; numbers and marker lines hold the maximum until double-click or playback restart.


### 1.4.10 — 2026-10-16

- 中文：修复 Lookahead 的“25.3 ms”被截成省略号。只调整文字内边距和箭头占位，字号、控件尺寸、声音与参数不变；540 组真实控件适配检查及行为约定检查通过。沿用适用的 1.4.9 双语手册。
- English: Fixes ellipsis clipping of “25.3 ms” in Lookahead. Only text padding and arrow space change; font size, field dimensions, audio and parameters remain unchanged. All 540 real-control readout fixtures and behavior contracts passed. The applicable bilingual 1.4.9 manuals are included.

### 1.4.9 — 2026-10-16

- 中文：横向缩放改为 Ctrl＋滚轮。停播时以鼠标为中心，播放时跟随最新音频；停播查看偏移不写入工程或 A/B。保留框选、细步幅纵向滚轮、Freeze 与实时参数重算。18 组停播夹具、27 组跟随及音频不变性检查通过；双语手册各 49 页、57 张截图更新。
- English: Changes horizontal zoom to Ctrl+wheel. It anchors at the pointer while stopped and follows the newest audio during playback; stopped navigation offsets are not saved in projects or A/B. Rectangle zoom, fine vertical wheel, Freeze and live parameter replay remain. Eighteen stopped fixtures, 27 live-follow cases and audio-invariance checks passed; both 49-page manuals and 57 screenshots were updated.

### 1.4.8 — 2026-10-16

- 中文：新增 FREEZE，点亮后仅在停播时冻结 Display；首次默认打开，新实例记住上次手动选择。工程保留自身状态，A/B 不改变它；录音、离线及未知播放状态保持实时。18 组冻结/模式/ECO/FULL 与 Window/HPF 重算检查通过，音频差值为 0。
- English: Adds FREEZE, which freezes the Display only after playback stops. It initially defaults on and new instances remember the last manual choice. Projects retain their own setting and A/B does not change it; recording, offline rendering and unknown transport states remain live. Eighteen freeze/mode/ECO/FULL fixtures and Window/HPF replay checks passed, with zero audio difference.

### 1.4.7 — 2026-10-16

- 中文：恢复左键框选放大，与细步幅滚轮并存，支持反向及连续框选。右键恢复完整视图；播放持续跟随最新音频，不恢复旧历史偏移。中文悬停说明同步更新；27 组混合缩放跟随、18 组交互和 12 组音频不变性检查通过。
- English: Restores left-drag rectangle zoom alongside fine wheel zoom, including reverse and repeated selections. Right-click restores the full view; playback stays at the newest audio without restoring old offsets. Chinese hover help was updated. Twenty-seven mixed-zoom follow cases, 18 interaction cases and 12 audio-invariance cases passed.

### 1.4.6 — 2026-10-16

- 中文：纵向及当时 Shift 横向滚轮的缩放灵敏度降为原来的四分之一，便于小幅调整；不增加响应延时。Display 悬停说明同步修改。保留范围、鼠标电平锚点与实时重算；行为约定、18 组交互和 12 组音频不变性检查通过。横向修饰键后于 1.4.9 改为 Ctrl。
- English: Reduces vertical and then-Shift horizontal wheel sensitivity to one quarter for smaller steps, without adding response lag. Display hover help was updated. Limits, pointer-level anchoring and live replay remain; contracts, 18 interaction cases and 12 audio-invariance cases passed. Version 1.4.9 later changes the horizontal modifier to Ctrl.

### 1.4.5 — 2026-10-16

- 中文：Display 纵轴 dB 刻度统一保留一位小数，避免缩放后出现冗长数字。仅改变文字精度，不改变实际范围、阈值或音频精度；离屏预览、行为约定与音频不变性检查通过。
- English: Formats Display dB-axis labels to one decimal place to avoid long numbers after zooming. This changes text formatting only, not actual ranges, thresholds or audio precision. Offscreen previews, behavior contracts and audio-invariance checks passed.

### 1.4.4 — 2026-10-16

- 中文：SAFE 改名 TRANSPARENT，中文说明强调瞬态、细节和减少噼啪声；参数 ID、算法、默认关闭及零 ms 禁用规则不变。加入鼠标电平为中心的纵向滚轮、Shift 横向滚轮与右键复位，当时暂时取消框选。18 组缩放、12 组音频不变性和中文提示检查通过；框选后于 1.4.7 恢复。
- English: Renames SAFE to TRANSPARENT and explains transients, detail and click reduction in Chinese help; parameter IDs, algorithm, default-off and zero-ms disabling remain. Adds pointer-centred vertical wheel zoom, Shift horizontal zoom and right-click reset, temporarily replacing rectangle zoom. Eighteen zoom cases, 12 audio-invariance cases and Chinese tooltip checks passed; rectangle zoom returns in 1.4.7.

### 1.4.3 — 2026-10-16

- 中文：用户试听后将最大 Lookahead 从试验用 105 ms 改为 100 ms，最终五档为 0 / 25.3 / 50.6 / 76 / 100 ms。旧 105 ms 设置恢复到 100 ms 并保留 Window 比例；Display 滞后修复保留。真实插件加载、状态迁移、36 组延迟与 15 组脉冲检查通过。
- English: After listening, the maximum Lookahead changes from the experimental 105 ms to 100 ms. Final presets are 0 / 25.3 / 50.6 / 76 / 100 ms. Saved 105 ms settings load at 100 ms while preserving Window proportion; the Display lag fix remains. Real plug-in loading, state migration, 36 latency fixtures and 15 pulse fixtures passed.

### 1.4.2 — 2026-10-16

- 中文：修复框选后历史时间偏移累积，导致 Display 长时间落后声音的问题。缩放保留比例，播放时右端跟随最新音频；旧工程的历史偏移清零。此版真实交付的 Lookahead 为 0 / 25.3 / 50.6 / 76 / 105 ms，供用户比较高延迟；旧档位就近迁移，保留参数身份、独立模式及 A/B。27 组跟随、33 组状态迁移和实际拖动实时更新检查通过；一小时为模拟时间线，不是一小时 DAW 实测。
- English: Fixes accumulated historical offsets after rectangle zoom that could make the Display lag far behind audio. Zoom keeps its scale while playback follows the live edge, and old project offsets are cleared. This delivered version used 0 / 25.3 / 50.6 / 76 / 105 ms for high-latency comparison. Old presets map to the nearest values while parameter identity, separate modes and A/B remain. Twenty-seven follow cases, 33 migration cases and real drag replay checks passed; the one-hour case is a simulated timeline, not a one-hour DAW test.

所有以上版本均为实际交付的数字版本；未由用户另行指定为 Stable。最后用户选定的 Stable 仍为 1.4.1。Windows 自动检查与离屏渲染不等于所有宿主中的人工试听；Mac 验证等级以本次 Release 说明为准。

All versions above were delivered numeric versions, without a new user-selected Stable designation. The last user-selected Stable remains 1.4.1. Windows automated/offscreen checks do not imply manual listening in every host; see this Release for the Mac validation level.


### 1.4.1 - Stable, 2026-10-14

- 中文：ST/LR/MS 模式按钮统一为紧凑尺寸；LR/MS 的 LINK 紧邻右侧，切换顺序为 ST → LR → MS，保留原参数编号及自动化含义。放大阈值标签，将 LUFS 表缩小移到 Display 左下角。
- English: Uses compact ST/LR/MS buttons with LINK immediately beside LR/MS, cycles ST → LR → MS and preserves saved parameter IDs. Enlarges threshold labels and moves the compact LUFS meter to the lower-left of the Display.
- 中文：Super 与 Classic 阈值范围统一为 -90 至 0 dB，取消 Super -inf。原 -inf 默认/复位改为 -90 dB，原 0 dB 默认保留。旧工程、预置和 A/B 低于 -90 dB 的阈值恢复为 -90 dB，Range OFF 保留；合法自动化位置和无条件 Output LINK 不变。
- English: Super and Classic thresholds share -90 to 0 dB; Super -inf is removed. Former -inf defaults/resets become -90 dB; existing 0 dB defaults remain. Lower legacy project/preset/A/B thresholds load as -90 dB. Range OFF, legal automation positions and unconditional Output LINK remain.
- 中文：配套中英文手册各 49 页，尽早解释 ST 的两个 Link，分开介绍 Lookahead/Window 与 SAFE；明确默认值、Alt/Option 复位与旧工程兼容。经用户指定为 Stable。
- English: Matching 49-page manuals explain both ST Links early, separate Lookahead/Window from SAFE, and distinguish defaults, Alt/Option resets and legacy recall. Stable status was explicitly selected by the user.

### 1.4.0 - Stable, 2026-10-14

- 中文：整理 Compressor/Clipper/Limiter 三种模式。Clipper 只下压，Ratio 10:1-1000:1，无 Ceiling，L/R Link 默认/Alt 为 0%，Output LINK 默认关闭。Comp/ST 的 L/R Link、M/S Link 同时提供，默认/Alt 均为 100%；两者都为 0% 时两种独立处理各占一半。
- English: Consolidates Compressor/Clipper/Limiter. Clipper is downward-only, 10:1-1000:1, without Ceiling; L/R Link defaults/resets to 0% and Output LINK starts off. Comp/ST provides both L/R and M/S Links, each default/reset 100%; at both 0%, independent L/R and M/S processing is mixed equally.
- 中文：修正 Clipper 最低 Ratio 灯带、Window 调回 0 的 Display 一致性；Clipper 的每声道 GR 保持最大值，双击或重新播放清零。修复 Limiter Output LINK 在 Ratio 1:1 或下压关闭时失效，联动仅由开关决定，双压 UP 保持独立。
- English: Fixes the Clipper minimum-Ratio arc and Display consistency when Window returns to zero. Per-channel Clipper GR holds its maximum until double-click or new playback. Limiter Output LINK remains active at Ratio 1:1 or with DOWN off; only its switch controls linking, and Dual UP remains independent.

### 1.3.7 - Historical development delivery, 2026-10-14

- 中文：首次交付三种处理模式、Comp/ST 双 Link、零 Window/零 Lookahead 下禁用 SAFE，以及 Display 框选放大和右键恢复。缩放保持滚动，改进 Ceiling 对齐和绘制流畅性，保留参数调整时的实时历史响应。此版本当时以 Candidate 名称交付，未设为 Stable；历史标识保留，功能后续并入 1.4.0。
- English: First delivered the three processing modes, two Comp/ST Links, SAFE disabling at zero Window/Lookahead and rectangle Display zoom with right-click reset. Zoom keeps scrolling; Ceiling alignment/rendering improve while live parameter replay remains. Historically delivered as Candidate, not Stable; this historical label is retained and the work was consolidated into 1.4.0.

### 1.3.6 — Stable, 2026-10-12

- 中文：所有悬停说明改为中文，并补齐 Input、Output、Makeup、Mix、Key Gain、模式切换和 A/B 等控件及数值栏的说明。统一 UTF-8 与中西文字体回退，覆盖 Windows 和 macOS；Alt/Option 复位说明保持一致。
- English: Converts hover help to Chinese and fills missing tips for Input, Output, Makeup, Mix, Key Gain, mode and A/B controls, including numeric readouts. Uses explicit UTF-8 and CJK font fallback on Windows and macOS, with consistent Alt/Option reset guidance.
- 中文：外部侧链曲线独立显示经过 HPF 和 Key Gain 后的信号，与主音频对齐，不再随 Window、SAFE 或 OS 改变。内部侧链的输出预览与原始曲线使用相同输入基准，修正低谷被填平、看似压缩后反而抬升的问题；未改变 Classic/Super 压缩规则。
- English: EXT history displays aligned post-HPF/Key-Gain audio independently of Window, SAFE and OS. INT output projections share the captured input reference, correcting filled valleys and apparent gain increases without changing the Classic/Super compression law.
- 中文：拖动 Window 和侧链 HPF 时可更新历史曲线；后台重算按耗时限制刷新频率，松开后完成最终刷新。高倍 OS 下显示刷新可能较慢。新实例普通模式和 Limiter 均默认 1x OS；已保存的工程、预设、A/B 和模式选择继续恢复。
- English: Window and sidechain HPF drags refresh history with bounded, adaptively paced background replay and a final refresh on release; higher OS may refresh more slowly. New Normal and Limiter instances default to 1x OS while saved project, preset, A/B and mode settings are retained.
- 中文：更新配套中英文用户手册，各 44 页。经用户确认设为 Stable；复用已验证并安装的 Windows 成品，macOS 从相同对应源码构建。
- English: Includes updated 44-page Chinese and English manuals. Promoted to Stable by user approval, reusing the verified installed Windows product and building macOS from the corresponding source.

### 1.3.5 — Stable, 2026-10-11

- 中文：Display 新增 Scale 按钮，在 -30 / -60 / -90 dB 之间切换，普通模式与 Limiter 共用。图表、阈值推子和拖动精度同步刻度，保留实际阈值与声音；超出视野的阈值保留真实读数并以星号提示。取消 Limiter 固定 -45 dB 限制，继续移除其 Range。Scale 随工程保存，A/B 和 Limiter 切换保持一致。更新中英文 44 页说明书。修正复用 Windows 构建目录时版号资源未更新的问题。用户已确认并指定为 Stable；双语手册配套发行。
- English: Adds a shared Display Scale button for -30 / -60 / -90 dB views in Normal and Limiter. Graphs, threshold faders and drag sensitivity follow the view without rewriting actual thresholds or changing audio; out-of-view readouts retain their value and show an asterisk. Removes the fixed Limiter -45 dB restriction while keeping Limiter Range disabled. Scale persists with the project and stays consistent across A/B and Limiter changes. Updates both 44-page manuals and fixes Windows resource-version refresh in reused build directories. Promoted to Stable by explicit user instruction, with matching bilingual manuals.

### 1.3.4 — Stable, 2026-10-11

- 中文：Limiter 的 Display 与 Single / UP / DOWN 阈值统一为 -45 至 0 dB；移除 Limiter 的 Range 控件及其处理作用，普通压缩保持原行为。按用户要求保留切换 Limiter 时原有的延迟变化。中文手册重新整理 Lookahead / Window、独立 SAFE 章节、Range 的功能与操作顺序、噼啪声提示，以及上压和下压的对称关系。用户已确认并指定为 Stable；配套 44 页英文手册已完成。
- English: Limiter Display and Single / UP / DOWN thresholds now span -45 to 0 dB. Removes Limiter Range controls and processing while preserving normal compression behavior. Existing Limiter-switch latency behavior is retained at the user's request. The approved Chinese manual clarifies Lookahead / Window, SAFE, Range operation and crackles, and the upward/downward compression relationship. Promoted to Stable by explicit user instruction; matching 44-page English manual is included.

### 1.3.3 — Stable, 2026-10-11

- 中文：统一 Window 毫秒控件；SAFE 移至右下，0 ms 禁用。Limiter 采用完整 ST 界面、单张 Display、一组共用阈值和 Ratio/Makeup/Mix/ON，内部保留可调关联的左右独立检测；移除参数 LINK、双压 Ratio LINK 和声道 Monitor（Limiter 固定完整立体声输出），保留 0–100% 音频 L/R Link（覆盖动态及 Ceiling）。普通 LR/MS 参数 LINK 同步对应 Dual ON。MATCH 支持播放中累计匹配，点击不清空、不重复叠加补偿；停止保留，下次播放重置。ECO 缺失分析时不提供不完整播放的 MATCH。详见新版双语用户手册。
- English: Window in ms; relocated SAFE disabled at zero Lookahead. Limiter has a full ST presentation with a single history panel and common boundaries/Ratio/Makeup/Mix/enables, while retaining independent L/R detection with adjustable gain coupling, no parameter LINK, UP/DOWN Ratio LINK or channel Monitor (always stereo output), and a numeric 0–100% audio link through dynamics and Ceiling. Normal LR/MS parameter LINK pairs matching Dual enables. MATCH retains the playback capture across clicks without re-adding correction; stop retains it and restart resets it. ECO skips are not treated as a complete capture. Promoted to Stable after user acceptance.

### 1.3.2 — Stable, 2026-10-09

- 中文：检测切换统一为 SAFE 按钮，位于 Limiter 右侧。新实例默认关闭；点亮为黄色 Safe，关闭为 Normal。Normal 显示 Distort 百分比，Safe 显示 Window 毫秒；切换保持实际窗口长度。更新配套中英文 43 页手册。DSP 沿用已验证的 1.3.1 后续方案。
- English: A fixed SAFE button sits to the right of Limiter, off by default in new instances. Lit yellow means Safe; unlit means Normal. Normal shows Distort percent, Safe shows Window in milliseconds; switching retains the actual window. Includes matching 43-page Chinese and English manuals. DSP retains the verified final 1.3.1 design.

### 1.3.1 — Development series consolidated into 1.3.2

- 中文：在固定 Lookahead 基础延迟内加入可调检测窗口。Normal 的 Distort 0% 对应全窗口，100% 对应零窗口；Safe 的 Window 为 0 至当前 Lookahead。手动切换 Lookahead 和 Alt 单击恢复全窗口，工程及 A/B 保留已存设置。过采样统一为 1x/4x/8x/16x，覆盖所有 Lookahead 的动态与 Ceiling；保留 1x 音频下的独立 TP 重建检测。移除 10 ms 档，旧 10/5 ms 状态迁移至 26 ms，最终档位为 0/26/40/80/100 ms。Limiter 上压阈值不再参与 Output/Makeup 联动，Super 最低 -inf、Classic 最低 -90 dB；下压和 Makeup 联动保留。修正 0 ms、Lookahead 切换及 A/B 的 Display 对齐。
- English: Adds an adjustable detector window within the fixed Lookahead base delay. Normal Distort maps 0% to the full window and 100% to zero; Safe Window spans zero to the current Lookahead. Manual Lookahead changes and Alt-click restore the full window; projects and A/B retain saved settings. Unified 1x/4x/8x/16x audio oversampling covers dynamics and Ceiling at every Lookahead, including independent TP reconstruction at native 1x audio. Removes the 10 ms preset; old 10/5 ms states migrate to 26 ms. Final presets are 0/26/40/80/100 ms. Limiter UP Threshold no longer links to Output/Makeup; its Super floor is -inf and Classic floor is -90 dB. DOWN and Makeup linking remain. Corrects Display alignment at 0 ms and across Lookahead/A/B changes.

### 1.3.0 — Detector investigation and comparison candidate

- 中文：针对带 Delay/混响素材的持续噼啪声，引入前后窗口峰值取较小值与取较大值的对比方案，并排查 1:1 Display 参考对齐。较大峰值方案以提前压及峰后压为代价，帮助抑制快速增益变化；相关候选方案在 1.3.1 中进一步整理。本系列未采用隐藏 Attack/Release 作为动态修复。
- English: Investigates persistent crackles on delay/reverb material with a comparison between the smaller and larger of past/future window peaks, alongside unity-ratio Display reference alignment. The larger-peak approach trades pre/post-attenuation for reduced rapid gain changes; the candidate designs are consolidated in 1.3.1. No hidden Attack/Release stage was introduced as the dynamics fix.

### 1.2.42 — Stable, 2026-10-01

- 中文：新增 FULL 停播极低残留保守休眠，保留实时监听与播放/录音/离线处理；无输出渐变及延迟变化。Windows 自动测试通过，Cubase ASIO-Guard 数值未量化。
- English: Added conservative FULL low-residual sleep on known Stop while retaining live monitoring and playback/recording/offline processing. No output fade or latency change. Windows automated tests passed; Cubase ASIO-Guard was not quantified.

### 1.2.41 — Stable, 2026-10-01

- 中文：ECO 宿主明确停播即停止处理，恢复时清除旧缓存；FULL 保留实时监听。优化 DSP 环形索引，Windows 自动检查通过。
- English: ECO suspends processing on a known host Stop and clears stale state on resume; FULL retains live monitoring. DSP ring-index optimization and Windows automated checks passed.

### 1.2.40 — Diagnostic candidate

- 中文：补充宿主停播输入峰值及侧链来源诊断，未改变音频休眠规则。
- English: Added stopped-input peak and sidechain-source diagnostics without changing sleep behavior.

### 1.2.39 — Diagnostic candidate

- 中文：修复性能诊断提示入口及新实例 FULL/ECO 选择记忆；不宣称解决停播负载。
- English: Repaired performance diagnostics entry and last-choice recall for new FULL/ECO instances; no stopped-load fix was claimed.

### 1.2.38 — Performance candidate

- 中文：优化 0 ms 检测器及 ECO 关窗分析，增加停播计时诊断；后续由 1.2.39 修复诊断入口。
- English: Optimized 0 ms detection and closed-editor ECO analysis and added stopped-time diagnostics; the diagnostics entry was repaired in 1.2.39.

### 1.2.37 — Performance candidate

- 中文：尝试降低停播负载；用户 Cubase 测试未观察到改善，因此没有提升为 Stable。
- English: Attempted to reduce stopped load; the user's Cubase test did not show improvement, so this was not promoted to Stable.

### 1.2.36 — Previous candidate

- 中文：加入独立 Core/Ceiling 过采样倍率及 Limiter 相关更新；早期 Stable 提升随后撤回。
- English: Added independent Core/Ceiling oversampling options and Limiter updates; an earlier Stable promotion was subsequently withdrawn.

更早的详细变更记录保留于各版 `RELEASE_NOTES_*.md`。 / Earlier detailed records remain in the versioned `RELEASE_NOTES_*.md` files.

## 安装与说明书 / Installation and manuals

Windows 10/11 x64 提供 VST3；macOS 11+ 提供 Apple Silicon VST3、Intel x86_64 VST3 与 Universal 2 AU。按宿主实际架构选一个 macOS VST3 包，AU 按宿主需要安装。升级前关闭宿主并备份工程及旧 bundle。

Windows 10/11 x64 uses VST3. macOS 11+ has Apple Silicon VST3, Intel x86_64 VST3 and Universal 2 AU. Select one macOS VST3 architecture for the host. Close the host and back up the session and old bundle before upgrading.

- [中文说明书，44 页](docs/manuals/QQ-Super-Compression-1.3.6-User-Manual-Chinese.pdf) / [English manual, 44 pages](docs/manuals/QQ-Super-Compression-1.3.6-User-Manual-English.pdf)
- [中文安装指南](docs/QQ-Super-Compression-1.3.6-INSTALL-CN.txt) / [English installation guide](docs/QQ-Super-Compression-1.3.6-INSTALL-EN.txt)
- [当前文档状态 / Current document status](docs/CURRENT_DOCUMENT_STATUS.md)

## 使用要点 / Basic use

Limiter 通常从 Ratio 20:1–1000:1 开始，保持 Output LINK，缓慢降低阈值。0 ms Lookahead 可配合 4x / 8x / 16x 总过采样试听响段与静段的动态对比。下压调好后，可适度加入 1:1–1:1.2 上压，并留意底鼓与军鼓延音。

For Limiter, start with Ratio 20:1–1000:1, keep Output LINK on and lower Threshold gradually. Try 0 ms Lookahead with 4x / 8x / 16x overall oversampling for loud/quiet contrast. After downward compression, optional gentle 1:1–1:1.2 upward compression needs care with kick/snare sustain.

## 编译 / Build

JUCE 固定为 8.0.15；CMake 3.22+。Windows 使用 VS 2022 C++ 工具与 Windows SDK，macOS 使用 Xcode Command Line Tools。macOS 最低部署版本为 11.0。详见 [复现说明 / reproduction](REPRODUCE_1.4.1.md)。Windows Actions 仅手动触发，本次交付复用已验证 Windows 成品。

JUCE is pinned to 8.0.15. Use CMake 3.22+, VS 2022 C++ tools and Windows SDK on Windows, or Xcode Command Line Tools on macOS. The macOS deployment target is 11.0. See [reproduction instructions](REPRODUCE_1.4.1.md). Windows Actions remains manual; this release reuses its verified Windows build.

## 已知限制 / Known limits

Safe 的较长 Window 会提前压及峰后压；较短窗口可能增加染色或噼啪声。统一过采样减少混叠但不保证所有素材无失真。旧工程升级后请核对 OS 和延迟补偿。

Longer Safe windows can cause pre/post-attenuation. Shorter windows can increase coloration or crackles. Unified oversampling reduces aliasing but does not guarantee distortion-free processing. Check OS and host latency compensation after migrating old sessions.

FULL may stay awake at extreme gain or with active input/selected sidechain. ECO cuts tails when the host reports Stop. Plugin CPU timings cannot be translated to an ASIO-Guard percentage; DAW scheduling and buffer conditions matter. macOS artifacts are ad-hoc signed and may require trusted-source quarantine handling as described in the installation guides. Commercial use is prohibited; redistributed binaries require free complete corresponding source and the required notices.

## 1.3.6 历史升级说明 / Historical upgrade notes

Limiter 旧 ST 设置迁移为 L/R Link 100%，旧 LR/MS 为 0%；旧 L（MS 为 M）参数成为共用值。Limiter 不再使用 Range。Display Scale 不会改变阈值；Classic 最低 -90 dB，Super 最低 -inf。如果旧 1.3.4 已将阈值存为 -45 dB，本版无法恢复更早的数值。旧双组参数工程可能改变声音，请另存工程并试听。

Older Limiter ST migrates to L/R Link 100%; LR/MS to 0%, with former L (M in MS) settings shared. Limiter Range is inactive. Display Scale does not change thresholds: Classic reaches -90 dB, Super -inf. If 1.3.4 already saved a clamped -45 dB value, earlier values cannot be reconstructed. Older split-channel sessions may sound different; save a session copy and audition it.

Window 只改变检测时长，不改变选定延迟；SAFE 可能衰减强音前后的声音。0 ms、较低 Window（尤其低于 10 ms）和 Limiter 推荐比较过采样。切换 Limiter 仍可能因 Ceiling 额外延迟触发宿主补偿。

Window changes detection duration at fixed latency. SAFE may attenuate audio around loud events. Compare OS at 0 ms, short Window (especially below 10 ms), and in Limiter. Limiter switching can still trigger host compensation because Ceiling adds latency.

当前构建说明 / Current build instructions: [REPRODUCE_1.4.1.md](REPRODUCE_1.4.1.md).

新实例 OS 默认 1x，由用户按声音和 CPU 选择；已保存工程不会被强制改为 1x。悬停帮助现为中文，英文操作说明见随附英文手册。

New instances start at 1x OS; choose the factor to suit the sound and CPU budget. Existing saved projects are not forced to 1x. Hover help is now Chinese; the included English manual explains all controls in English.
