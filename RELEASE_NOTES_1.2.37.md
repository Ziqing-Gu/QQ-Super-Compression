# QQ Super Compression 1.2.37 Candidate

此版本针对停止播放后的整体静音开销，尚待用户在 Cubase 原工程中验证，不属于 Stable 发布。Plan B 未执行，Plan C/D 保持取消。

- 静音充分排空后按音频块快速返回，同时跳过主压缩、Ceiling、逐样本增益/混合/延迟更新和真峰值过采样分析。
- 保留 FULL 历史记录、TP/GR 保持时间、播放/极小输入/参数/外部侧链的当前块恢复和插件延迟。
- 只有当前选用的侧链会阻止休眠；INT 模式下未选用的侧链不再造成无意义的持续运算。
- 不设置听觉门限、不截断低音量输入。保留原有尾音排空与恢复等待。

本机 48 kHz / 256 样本、FULL、窗口关闭，10 实例静音每块总耗时：Normal 1x 从 1081.67 降至 55.2238 微秒，Limiter 16x 从 1399.91 降至 81.7578 微秒。以上为独立测试耗时，不是 Cubase ASIO-Guard 百分比。实际 VST3 共 30 个状态/配置组合通过；完整回归通过，新增静音/唤醒对照最大绝对差值 1.49012e-8。

This candidate reduces the complete stopped-silence path, including native-rate processing and multiple instances. It is not a Stable release and still requires verification in the user's Cubase session. Plan B was not executed and Plan C/D remain cancelled.

- After tails and recovery settle, the processor returns through a block-level idle path while retaining FULL history, TP/GR hold timing and reported latency.
- Input, playback, automation and the selected sidechain wake the current block. An unused sidechain no longer prevents internal-key instances from sleeping.
- No level-based noise gate is introduced; very quiet input remains active. Existing tail-drain and recovery timing remain intact.
- Local 48 kHz / 256-sample, FULL, closed-editor ten-instance timing: Normal 1x 1081.67 to 55.2238 microseconds per chain block; Limiter 16x 1399.91 to 81.7578. These are standalone timings, not Cubase meter percentages. Thirty actual VST3 cases and the full review regression passed; the added wake comparison has maximum absolute error 1.49012e-8.

Evidence: Verification/1.2.37-Windows/validation-report.json. Approved 1.2.36 manuals remain unchanged.
