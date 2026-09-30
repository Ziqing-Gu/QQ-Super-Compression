# 1.2.8 — Limiter Mode Continuity Candidate

日期：2026-09-28。状态：源码候选，待 Windows / Cubase 验证。

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

## Validation status (English)

Implemented on the supplied verified 1.2.7 source, not an older repository checkout. Full project source is delivered. Core-function isolation tests pass; the original baseline fails the same threshold-switch case as expected. Full JUCE/Windows/Cubase validation remains pending. No 1.2.8 binary, installation, release or Plan B completion is claimed.
