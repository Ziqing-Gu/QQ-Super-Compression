# QQ Super Compression Preview 1.2.6

这是独立验证版，不是 Stable。正式 QQ Super Compression 1.2.5 Stable 保留不动。

## 这次改动

- 同时作用于 Classic 和 Super，包括 Single / Dual、Up / Down、ST / LR / MS。
- 两种算法各自的 Ratio、Threshold、Range 曲线保持不变，改的是决定动态增益的电平检测。
- 对实际输出时刻 t，检测电平取 `min(过去 N 个样本窗口的绝对峰值, 未来 N 个样本窗口的绝对峰值)`，两个窗口均含当前样本。
- 未来强音不能仅靠未来窗口提前压低前面的弱音；稳定正弦仍通过峰值保持取得稳定增益。
- 默认预读仍为 26 ms；过去窗口只使用已有历史，不增加延迟，也没有新增 Attack / Release 控件。
- 历史 Display 回放也使用相同的双侧检测。更改 Lookahead 时，用 2N 样本历史重建检测，避免丢失过去窗口。
- 独立 VST3 名称、插件 ID 和用户偏好文件，便于和 Stable 同时加载。

## 在 PluginDoctor 对比

分别加载 `QQ Super Compression` 和 `QQ Super Compression Preview`，两边设置一致：

1. Single，Classic，Threshold −12 dB，Ratio 8:1，Range OFF。
2. Input / Makeup / Output = 0 dB，Mix 100%，Lookahead 26 ms，内部检测，Key HPF OFF。
3. Dynamics 信号：4341.1 Hz，1 秒 −20 dB、2 秒 0 dB、1 秒 −20 dB。
4. 检查 1 秒跳变前的凹陷；再将两边都切为 Super 重复对比。
5. 也可使用 Threshold −40.96 dB、Ratio 12.4:1，或上压 Ratio 1:8，进行进一步试听。

测试版在旧 DAW 工程中不会自动替代 Stable。请按独立插件名称加载。

## 已验证

实际加载两份 VST3 的离线宿主测试，不是只测数学模拟：

- 64 组阶跃用例：Classic / Super、上压与下压、50 / 400 / 1000 / 4341.1 Hz、多相位、8 / 12.4 / 1000:1 等比例。
- 上述截图同类 Classic 用例，Stable 的弱音在跳变前多衰减约 10.5 dB；Preview 测得 0 dB 额外衰减。
- 所有阶跃用例的弱音段最大前后误差约 0.00224 dB；强音段没有超过测试曲线的预期峰值上限。
- 44.1 / 48 / 96 kHz，17 / 256 / 1024 样本块；音频块大小不改变结果。26 ms 延迟保持。
- 20–6000 Hz 的 9 个稳态正弦频点，两种算法的 H2–H7（仅计 Nyquist 以下）最差约 −150.1 dBc；这只是所测条件，不代表任意信号零失真。
- 原 DSP 回归通过，包括静态 Ratio 曲线、Single / Dual、ST / LR / MS、Range 连续边界、分支 Crossfade、216 组 A/B、深压缩 MATCH 和 0 ms 过采样。
- 新增检测独立逐窗口参考验证、环形缓冲回绕 / Reset，以及 26 / 40 / 100 ms 切换时的历史重建检查。
- 实际编辑器离屏回归通过；标题明确标识 Preview。

## 需要试听确认的取舍

本版针对提前凹陷，不承诺把每个瞬时采样点或每段任意音频的输出都维持在阈值以上。

36 组实际 VST3 调幅 / 双音测试表明：动态包络跟随更接近当前时刻，但部分信号的非期望频谱残差有所增加。

- 400 Hz 载波 / 3 Hz 调幅 / ±10 dB，Classic 12.4:1：谐波邻域残差从 −47.88 到 −46.05 dBc；Super 从 −53.84 到 −51.94 dBc。
- 50 Hz / 11 Hz / ±20 dB 压力测试，Classic 下压残差从 −17.37 到 −13.29 dBc，这是本轮最明显的调幅退步；同组 Super 下压则改善约 1.18 dB。
- 双音测试中最明显的非输入频谱能量增加约 1.03 dB（Classic，400 + 401 Hz）。

这些调幅 / 双音指标不是稳态 THD。完整结果在交付目录的 `Verification`，原始浮点测试音频保留于 `D:\Codex\Temp\QQSC-1.2.6-Preview-results`。需要用真实音乐确认这种取舍是否可接受，再决定下一步。

## 重现构建

只使用本目录的 `build-preview.cmd`，使用 JUCE 8.0.15 和 Visual Studio 2022 x64。

- 源码：`D:\Codex\Workspaces\QQSuperCompression-1.2.6-Preview`
- 构建：`D:\Codex\Temp\QQSC-1.2.6-Preview`
- 输出：`D:\Codex\Outputs\QQ Super Compression\1.2.6 Preview`

复制自 Stable 的旧版本记录和旧构建 / 发布脚本仅作为历史参考，不应用来发布本 Preview。未执行 Plan B / C / D，没有桌面副本，没有更新说明书或多段压缩。
