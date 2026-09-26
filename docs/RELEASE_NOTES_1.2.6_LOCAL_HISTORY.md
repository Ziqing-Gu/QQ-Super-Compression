# QQ Super Compression 1.2.6 Stable — 2026-09-27

用户已确认接受 1.2.6 Preview 的声音与实测调制残差取舍，并要求作为正式最新版替换 QQ Super Compression、移除 Preview VST3、执行 Plan B。

## 本版行为

- Classic 和 Super 共用双侧峰值检测。对输出位置 t，电平取过去与未来各 N 样本窗口绝对峰值的较小值，两个窗口均含当前样本。
- 稳定弱音后即将出现强音时，未来峰值不再单独决定弱音的增益。截图同类 4341.1 Hz、−20 / 0 / −20 dB 阶跃测试，Classic 原本约 10.5 dB 的提前凹陷被消除。
- 默认 Lookahead 仍为 26 ms，无额外延迟，无新增 Attack / Release 控件。26 / 40 / 100 ms 变更时重建两侧历史；Display 历史检测回放与音频路径同步更新。
- 两种算法各自的 Ratio / Threshold / Range 曲线、Single / Dual、上压 / 下压、ST / LR / MS、A/B 和分支 Crossfade 保持已确认 Preview 的行为。
- 恢复正式名称 QQ Super Compression、Qscp 插件身份与 QQSuperCompression 用户偏好文件。正式旧工程继续识别插件；参数 ID、顺序和默认值保留。

## 验证与已接受取舍

接受版本已通过 64 组实际 VST3 阶跃测试，弱音段前后最大误差约 0.00224 dB；44.1 / 48 / 96 kHz、17 / 256 / 1024 样本块保持一致。20–6000 Hz 的 9 个稳态频点，两种算法均未出现明显新增 H2–H7。

调幅残差相对于已知包络的目标输出测量，不等同于稳态 THD。400 Hz / 3 Hz / ±10 dB、12.4:1 时，Classic 谐波邻域残差由 −47.88 到 −46.05 dB，Super 由 −53.84 到 −51.94 dB。50 Hz / 11 Hz / ±20 dB 压力测试，Classic 下压由 −17.37 到 −13.29 dB（增加 4.09 dB）；同组 Super 下压改善约 1.18 dB。用户已知悉并接受。没有任意信号零失真、所有局部输出均高于阈值的承诺。

正式提升要求实际 VST3 输出与已确认 Preview 逐样本一致，并验证旧正式身份、参数、工程状态；重新运行 DSP、A/B、Range、MATCH 与实际编辑器回归。完整证据保存在输出目录 `Verification`，仅 `PLAN_B_COMPLETION.json` 的 COMPLETE 表示本阶段已完成。

已有工程会使用新的检测时序，因此动态信号的声音可能与 1.2.5 有差别，这是本版预期变化。Preview 的独立宿主身份不会自动迁移；使用过 Preview 的工程需换成正式插件并核对设置。旧版源代码与备份保留。

## English

The user-approved 1.2.6 Preview audio is promoted to the formal QQ Super Compression product. Classic and Super both use the minimum of the past and future peak windows aligned to the output sample. The default lookahead remains 26 ms, with no added latency or attack/release controls. This removes the pre-dip in the tested low-to-high plateau transitions; static transfer laws and accepted crossfades remain unchanged.

The formal Qscp identity, product name and QQSuperCompression preferences are restored. Parameter IDs, order and defaults are preserved. Existing formal projects adopt the new detector timing. Projects using the separate Preview identity require replacement with the formal plugin and a settings check.

The accepted candidate passed 64 actual VST3 step cases and steady-tone checks. Some AM residuals increase: the largest measured AM regression was 4.09 dB in the Classic downward 50 Hz / 11 Hz / ±20 dB stress case. The user accepted this tradeoff. This residual metric is not steady-tone THD, and the release does not promise zero distortion for arbitrary signals or a per-sample threshold floor.

Promotion validates bit-identical actual audio against the accepted Preview, formal identity/state compatibility, native DSP and editor regressions. Plan B preserves full source and exact JUCE 8.0.15 dependencies. Existing manuals remain unchanged; these notes document the newer behavior. This stage does not publish to GitHub, build macOS artifacts or create a desktop package.
