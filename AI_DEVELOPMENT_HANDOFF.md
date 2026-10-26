# Current delivery — QQ Super Compression 1.4.20 Stable

User-selected Stable on 2026-10-27. Validated Windows and completed frozen Plan B are reused; never access or refresh the backup. Chinese and English manuals are now 1.4.20, 49 pages each. Read docs/BEHAVIOR_CONTRACTS.md before changing the product. Current behavior includes display-only Adjust, Shift-wheel synchronization, -inf to +24 dB thresholds and optional Ceiling with sample-peak or true-peak limiting. Match uses original input and pre-Makeup dynamic Wet. Preserve unconditional Output LINK and all other user contracts.

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

--- Historical records ---

# Current delivery — QQ Super Compression 1.4.17 Stable

User-designated Stable on 2026-10-18. Windows and Plan B are completed prerequisites and reused. Never access or refresh the frozen snapshot. Plan C/D authorized: publish current source, build three Mac artifacts, deliver four platforms and publish the Release. Version history since 1.4.14 covers 1.4.15–1.4.17. Both 1.4.14 manuals are intentionally reused unchanged; do not edit or relabel them. Read docs/BEHAVIOR_CONTRACTS.md before product edits.

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

--- Historical records ---

# Current delivery — QQ Super Compression 1.4.14 Stable

User-designated Stable on 2026-10-16. Windows uses the verified 1.4.14 binary; bilingual 1.4.14 manuals are finalized. The user authorized public source, three Mac outputs and Release publication. Version coverage since public 1.4.10 includes 1.4.11–1.4.14. Read docs/BEHAVIOR_CONTRACTS.md before product edits. Completed Windows and backup prerequisites must be reused; frozen backups must not be accessed.

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

--- Historical records ---

# Current delivery: QQ Super Compression 1.4.10 — 2026-10-16

The Windows binary is reused from the completed local 1.4.10 delivery. The user's Plan B/C/D request authorizes a new frozen source snapshot, public source synchronization, cross-platform delivery and Release publication. The last user-selected Stable remains 1.4.1; this request does not promote 1.4.10 to Stable.

README and CHANGELOG cover every actual version after public 1.4.1 through 1.4.10. Read docs/BEHAVIOR_CONTRACTS.md before product changes. The bilingual 1.4.9 manuals remain applicable and keep their original edition number. Do not rebuild or access frozen backups to repeat completed prerequisites.

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

--- Historical records ---

# Current release: QQ Super Compression 1.4.1 Stable

User-approved on 2026-10-14. The accepted Windows product is reused; Mac release builds use the same production inputs. README and CHANGELOG cover every actual version after 1.3.6: 1.3.7, 1.4.0, 1.4.1. Matching Chinese and English manuals have 49 pages.

Read AGENTS.md and docs/BEHAVIOR_CONTRACTS.md before changes. Future delivered product modifications require the next numeric version; Stable requires explicit user choice. Reuse completed Plan stages and never reopen frozen backups. Historical Candidate/Test labels below describe actual old deliveries and are not the current workflow.

--- Historical records ---

# Current: 1.2.8 Plan C correction — 2026-09-28

Desktop delivery is now verified: eight files, 43,518,833 bytes, comprising Windows x64 VST3, Apple Silicon VST3, Intel VST3, Universal 2 AU, bilingual installation guides and two clearly labelled historical 1.2.7 manuals. Apple Silicon job 108719043229 passed in run 36354377015 at f5891559f24d54abe272d37d2941cdb1e068ba89. Intel job 108717571154 passed in run 36353856702 at 9ca6e6ff284dc133d7c7ecd3137861c6fc314093; the run's aggregate failure belongs to its earlier ARM attempt and is not reported as a successful run. AU job 108714126279 passed at a281cd9700b57d29378820b8fdb1fd9080e72767. All three successful jobs have identical production inputs; the 36 production source/resource/build files match the accepted Plan A workspace. The ARM Ceiling test now accounts for sub-step floating-point rounding and requires reset/redo to restore the exact stored default. The transient matrix-selection failure in run 36354310433 was corrected before the successful ARM build. Windows was reused without rebuilding. Plan D remains deferred.

Product: QQ Super Compression, not QQ Super Multiband Compression. Public repository: Ziqing-Gu/QQ-Super-Compression. Input: the user's 1.2.8 Limiter Mode Continuity source, based on 1.2.7 Strict Output Link. Plan A is complete, with Windows binary SHA-256 9A5E39848AB2691AFF7F70AAF3781D6FDBE7004B90D62C3CF57B9A0327F8B7E3. Plan B was already confirmed complete by the user; do not inspect, refresh or change frozen backups. This instruction supersedes historical checkpoint advice below about refreshing backups.

The governing Plan definitions changed on 2026-09-01: Plan C includes source synchronization, native Apple Silicon/Intel VST3, Universal 2 AU and the actual desktop user package. Plan D means the formal GitHub Release and is deferred. Previous claims that source push alone completed Plan C were incorrect. AU passed in macOS run 36352181159 attempt 2, from a281cd9700b57d29378820b8fdb1fd9080e72767. The VST3 checks exposed two portability issues: comparison against the requested decimal rather than the stored float on arm64, and Ctrl instead of Command for undo on macOS. Commit 9ca6e6ff284dc133d7c7ecd3137861c6fc314093 uses exact pre/post stored-value equality and the platform command modifier; production source is unchanged. Run 36353856702 rebuilds the two VST3s and reuses the verified AU. Windows reuses Plan A; no Windows Actions build is required.

The first Mac run used the historical QQSCDynamicsCheck and failed on its obsolete 26ms total-latency assumption. Attempts to adapt old tests were fully reverted. The current workflow uses the supplied 1.2.8 Limiter, Unity, Dual algorithm and Continuity suites, plus architecture/version/signature checks and AU validation. Do not claim the obsolete suite passed. Historical tag v1.2.8 remains at 365ee5c; the exact Mac build commit above is authoritative for this package, with identical production inputs.

Only the verified, physically present desktop package establishes Plan C completion. Keep internal evidence separate from the user package. The existing 35-page bilingual 1.2.7 manuals may be supplied only under their original version as historical references; they are not complete 1.2.8 manuals. DAW listening and user acceptance are not inferred from automation.

---

# Historical: 1.2.6 Plan C/D release checkpoint — 2026-09-27

User explicitly requested Plan C then D for QQ Super Compression (not multiband), with the approved new Chinese and English manuals. Release source: D:/Codex/Workspaces/QQSuperCompression-1.2.6-PlanCD. Output/evidence: D:/Codex/Outputs/QQ Super Compression/1.2.6. Public repository: Ziqing-Gu/QQ-Super-Compression.

Reuse the validated installed Windows Stable 1.2.6 bundle: B7AB486AE66BB57997FA88CF44255C886602F913ACAAE3FC26B856CDFD885493. Production Source/Assets/CMake are byte-identical to Stable. Both approved 28-page PDFs are integrated without alteration, with their source builders/assets. Only docs and workflow labels differ from Stable. Source/version and native cross-platform tests must match before publication. Historical workflows and binary identities remain.

Create a NEW complete source + exact JUCE 8.0.15 snapshot under the existing D:/备份文件/Vibe Coding/QQ Super Compression/源代码 hierarchy. The external Verification/PLAN_B_COMPLETION.json is authoritative; COMPLETE freezes that snapshot. Never reopen or refresh old completed snapshots. Later source changes require another new snapshot, not mutation.

Final package: actual desktop directory QQ Super Compression 1.2.6, root bilingual installation guides and new PDF manuals, Win/ and Mac/ ZIPs. Keep proof files out of it. Publish the exact same eight files on v1.2.6 and verify asset digests/unauthenticated availability. Verification/RELEASE_COMPLETION.json records final success. This checkpoint does not claim pending remote jobs, backup or packaging have already completed. No multiband modification or new Windows install/rebuild is required.

---

# Current: 1.2.6 Stable — 2026-09-27

Follow [STABLE_1.2.6.md](STABLE_1.2.6.md) and RELEASE_NOTES_1.2.6.md. The user accepted the Preview tradeoff and requested formal replacement, Preview VST3 removal and Plan B. Both algorithms use the accepted 26ms aligned detector. Restore the formal Qscp identity and QQSuperCompression preferences, preserve accepted audio, verify old formal state compatibility, and complete the formal source/dependency snapshot. Completion authority: output Verification/PLAN_B_COMPLETION.json. Older instructions below apply only to their dated baselines.

---

# Historical: 1.2.5 Stable — 2026-09-27

Follow [STABLE_1.2.5.md](STABLE_1.2.5.md). User approved Classic90 as the unchanged Stable baseline and Plan B. Subsequent lookahead work must be isolated. No desktop, multiband or publication work is authorized in this stage. The existing installed binary is the Stable binary; verify its hash before any proposed replacement.

---

# Historical candidate checkpoint

# Current: 1.2.5 audition candidate — 2026-09-26

This checkout is the 1.2.5 candidate, not the historical 1.2.3 release below. Follow [CANDIDATE_CHECKPOINT_1.2.5.md](CANDIDATE_CHECKPOINT_1.2.5.md) and [TRY_1.2.5.md](TRY_1.2.5.md). Stable, publication and multiband work are paused pending user audition. No desktop output unless the user requests Plan C. The final MATCH design removes the absolute detection gate; do not restore the earlier fallback.

# Historical: 1.2.3 Plan C/D release checkpoint — 2026-09-26

User authorized Plan C/D after approving the accepted 1.2.3 Stable algorithm. Production code and assets remain byte-identical to the accepted Stable workspace. Original PDF manuals are unchanged. Current release work updates documentation and Mac workflows, including native DSP regression, macOS 11 minimum and explicit ad-hoc signing.

- Release source: `D:\Codex\Workspaces\QQSuperCompression-1.2.3-PlanCD`.
- New formal source snapshot: `D:\备份文件\Vibe Coding\QQ Super Compression\源代码\QQ Super Compression 1.2.3-PlanB-Release-20260926-205600`.
- External completion authority: `D:\Codex\Outputs\QQ Super Compression\1.2.3\Verification\PlanB-Mac-Validation\PLAN_B_COMPLETION.json`.
- Public source / release: https://github.com/Ziqing-Gu/QQ-Super-Compression/tree/v1.2.3
- Platform test and publication completion authority: `D:\Codex\Outputs\QQ Super Compression\1.2.3\Verification\RELEASE_COMPLETION.json`.

Only COMPLETE in those external records certifies completion. Never reopen, rehash or modify a completed snapshot. The earlier Stable Plan B remains frozen; release documentation requires this new full source + exact dependency snapshot. GitHub publication and platform packaging are verified externally after this source checkpoint. Use current 1.2.3 guides and release checklist; older sections below record their historical scopes.

# Mac native validation follow-up

The first arm64 job passed all fixed-dB, deep-threshold and reciprocal audio checks, then exposed an exact-float assertion in a historical parameter endpoint test. The test now permits less than 0.0001 dB (100 times below its 0.01 dB control step), logs the endpoint, and independently requires finite Range to remain enabled. Production code and the accepted Windows binary are unchanged. A new complete Plan B includes this test correction before final publication.

---

# QQ Super Compression 1.2.3 Stable

User approval:2026-09-26. The user accepted the QQ dB Compression algorithm and requested official Super Compression replacement, removal of the test installation, Stable designation and Plan B.

Version1.2.3, state schema17, JUCE8.0.15. Product QQ Super Compression; pluginQscp; bundle com.qingaudio.qqsupercompression. Original VST3 CIDs ABCDEF019182FAEB51696E6751736370 and ABCDEF011234ABCD51696E6751736370. Preference application QQSuperCompression. No legacy algorithm rollback is introduced for finite old-project thresholds.

- Active source: `D:\Codex\Workspaces\QQSuperCompression-1.2.3-Stable`.
- Build: `D:\Codex\Temp\QQSuperCompression-1.2.3-Stable`.
- Verified local output: `D:\Codex\Outputs\QQ Super Compression 1.2.3 Stable`.
- Formal Plan B snapshot: `D:\备份文件\Vibe Coding\QQ Super Compression\源代码\QQ Super Compression 1.2.3-PlanB-Stable-20260926-200700`.
- External completion authority: `D:\Codex\Outputs\QQ Super Compression 1.2.3 Stable\Verification\PlanB\PLAN_B_COMPLETION.json`.

Only status COMPLETE in the external record certifies verified Plan B completion. The full source, assets, documents, workflows, build scripts, exactJUCE8.0.15 source/licenses, accepted Windows VST3 and evidence are included. Verify all files/bytes/hashes, then freeze. Consult the external record without reopening, rehashing, overwriting, deleting or building in a completed frozen snapshot. Old frozen snapshots are not accessed. Future work belongs in an active workspace and a new snapshot.

DSP, thresholds, Display mathematics and numerical fixes match the user-approved comparison build. Only formal branding/identity/preferences/state variant metadata are changed in production sources. Windows VST3 and processor/editor checks are rebuilt for that identity. Approved original bilingual PDF manuals are unchanged; current supplemental guides describe the new algorithm.

Scope is Stable plus Plan A/B. No GitHub publication, Plan C/D, macOS build or desktop repack is claimed.

## Implementation pointers

Finite Down gain=pow(T/p,1-1/R). Finite Up gain=pow(A/p,1-r) followed by the retained lower gate blend. A is Single finite Range/0dB or Dual Down threshold. -inf maps to0linear and uses the original rational branch. Single finite Range returns to unity internally; Dual ends Up at DOWN before Down takes over. Existing future-window peak and latency are unchanged.

Use tests/db_dynamics_test.cpp via QQSCDynamicsCheck, and QQSCVisualCheck for current expectations. The legacy included test main is historical helper code and is not the current curve oracle. Lower than-100dB thresholds remain finite until the-120dB sentinel. Weighted Mix/crossfade sums avoid cancellation at deep Down. Display and meter calculations match the audio.
