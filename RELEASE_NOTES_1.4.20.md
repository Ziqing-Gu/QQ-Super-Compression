# QQ Super Compression 1.4.20 Stable

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
