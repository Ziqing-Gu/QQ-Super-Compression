# 1.2.39 Candidate

- Add an editor-owned TooltipWindow. Keep the FULL/ECO tooltip text constant so GUI refreshes cannot restart its hover timer.
- Right-click FULL/ECO opens an editor-owned Performance / Idle diagnostics panel. Right-click does not toggle the mode. The snapshot is selectable, scrollable and copyable; Refresh updates it. Closed-editor counters remain available after reopening.
- Explicit left-click FULL/ECO saves `lastPerformanceEco` in the existing user settings, after reloading to preserve other instances' changes. Each newly constructed processor reads this default even before opening an editor.
- Existing instances never poll or follow that default. Project/preset instance state takes precedence; restore, A/B and editor reopen do not save a default. Missing old-project state retains the prior FULL migration rule.
- Audio callback, detector, oversampling and Ceiling processing are unchanged from 1.2.38. User confirms ECO improvement but stopped-silence load remains unresolved. No release plans resumed.

## 使用

停止播放，关窗静置至少 10 秒，重新打开插件，**右键 FULL/ECO**。点击 **Copy report**，将文字发回以检查 `closed` 区间的 `sleep` 与 `Last idle gate`。面板不自动刷新；需要更新时点 Refresh。

要记住新实例的默认选择，请左键选择一次所需的 FULL/ECO。此后新建实例沿用该选择；已有实例及工程恢复各自保留原状态。

诊断入口、右键不切换模式、新实例默认值、工程恢复优先级、实例隔离和不同尺寸快照的验证记录位于 `Verification/1.2.39-Windows`。Cubase 安装后的界面与静音原因仍需用户验证。
