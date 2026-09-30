# 1.2.40 Candidate: stopped raw-input diagnostics

The 1.2.39 Cubase report confirms stopped transport, 0% exact-zero blocks, 0% sleep and a nonzero-input idle gate at 16x/16x. It does not establish the input amplitude or source.

- Measure finite raw main-input and selected external-key block peaks only while host transport is known and stopped; report min/max using scientific notation and dBFS.
- Include key selection, external bus channels, stopped measurement count and nonfinite input sample count. Ignore unselected sidechain samples and exclude playback from these ranges.
- Retain closed-editor snapshots, right-click FULL/ECO access, explicit Copy report/Refresh and per-instance/default preferences.
- No sample replacement, silence threshold or sleep-policy change. Audible DSP is unchanged. This candidate gathers evidence; the stopped-silence load issue remains unresolved.
- Targeted input tests, diagnostics/preferences regressions, three-size component renders and source architecture audit pass. No new Cubase acceptance or broad audio benchmark is claimed. Plan B/C/D remain stopped.

停止播放，关闭插件界面至少 10 秒，再打开并右键 FULL/ECO，点击 Copy report。新增的 Stopped raw main peak 与 selected EXT peak 数值用于判断非零信号来源及幅度。本版补充测量，尚未修复静音负载。
