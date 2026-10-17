# QQ Super Compression 1.4.17 Stable

Changes since 1.4.14 / 自 1.4.14 起的更新。

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
