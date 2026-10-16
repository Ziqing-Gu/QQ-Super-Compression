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

# Current: QQ Super Compression 1.3.2 Stable

Current public documentation: RELEASE_NOTES_1.3.2.md, REPRODUCE_1.3.2.md, docs/CURRENT_DOCUMENT_STATUS.md. Chinese and English manuals are complete. Current workflow checks target SAFE, Distort/Window, unified OS, Display, transport and FULL residual sleep; historical tests are not all current acceptance profiles. The previously completed source backup is frozen and is not revisited for publication.
