# 1.2.8 — Limiter Mode Continuity

日期：2026-09-28。状态：Plan A Windows 构建、安装及自动回归通过；DAW 听感与工作流仍由用户验证。

## 修复

- Limiter Single → Dual：Dual 的 DOWN Threshold 接续当前 Single Threshold。
- Limiter Dual → Single：Single Threshold 接续当前 Dual DOWN Threshold。
- Makeup、Mix、Output Gain 保持当前值；不依据 Output 的正负或大小反推阈值，不重新校准 Output。
- 无论 Limiter Link、声道 Link 或 Ratio Link 是否开启，均执行上述切换规则。
- ST、L、R、M、S 分别处理，不影响各域原有差异。
- 新模式向音频线程发布前，先更新目标模式的阈值；GUI 切换在重新绑定控件之前同步参数，并纳入同一次 Undo 事务。

## 保持不变

正常压缩模式的 Single / Dual 独立阈值记忆；Normal / Limiter 两组设置；A/B 与工程恢复；现有参数 ID、顺序、state schema 22；Single/Dual 各自的 Ratio 与算法设置；DSP、Lookahead、Ceiling、True Peak、LUFS 和现有输出联动算法。

保留两套旧参数 ID 兼容工程。这里的“保持一致”指每次 Limiter Single/Dual 切换都接续当前向下阈值，绝不因模式切换召回旧阈值；不是把正常模式、A/B、各声道或工程中的所有阈值合并。

若接续后的阈值与 UP gate / Range 发生交叉，沿用原有的边界推移与相等规则；不额外发明动态处理策略。

## English changes

- In Limiter mode, switching Single to Dual carries the current Single Threshold into DOWN Threshold; switching back carries the current DOWN Threshold into Single Threshold.
- ST, L, R, M and S carry their own values independently of Limiter, channel and Ratio links. Makeup, Mix, Output Gain, ratios and algorithm choices are preserved.
- Normal compression retains independent Single / Dual threshold memories. A/B, project restore and Undo/Redo preserve their state semantics. Existing parameter IDs/order, plug-in identity and state schema 22 are retained.
- Boundary collisions retain the existing UP gate / Range rules. This update does not redesign the audio algorithms.

## 发布阶段 / Publication stage

当前仅执行 Plan C 源码同步；Plan B 已由用户确认完成。Plan D、跨平台成品交付和正式 GitHub Release 暂缓。历史 1.2.7 中英文手册为功能参考，不是匹配 1.2.8 的新版完整手册。

This stage covers Plan C source synchronization only; the user has already confirmed Plan B complete. Plan D, cross-platform package delivery and the formal GitHub Release are deferred. The historical bilingual 1.2.7 manuals are feature references, not newly revised complete 1.2.8 manuals.

## 验证 / Validation

Implemented on the supplied verified 1.2.7 source, not an older repository checkout. The full JUCE Windows build and real editor/APVTS continuity regression pass. Full Limiter, Unity and loudness suites pass. Real VST3 comparison against the installed 1.2.7 covers 120 normal-mode and 24 Limiter cases with zero residual; identity, parameter count/order and latency are unchanged. The installed and delivered Windows bundles match the build hashes. Cubase listening/workflow validation remains user-owned and is not claimed as completed by automation.
