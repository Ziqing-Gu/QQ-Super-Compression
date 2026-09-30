# QQ Super Compression 1.2.8 — 修复与验证说明

日期：2026-09-28。状态：**源码候选，尚未完成 Windows VST3 构建与 Cubase 验证。**

## 基线

本次仅以用户确认的 `QQSuperCompression-1.2.7-Strict-Output-Link.zip` 为起点，保留其中已完成的 Dual Up/Down 独立算法等功能，没有从旧仓库或旧备份回退开发。

本次重新计算并确认输入源码 ZIP 的 SHA-256：

```text
D065AFB3F0618D79CCC81807610CB44EB1CA1A5C1CCC55573542DA9D054ECCFF
```

已确认成品的 1.2.7 VST3 主程序 SHA-256，沿用前一步成品/源码比对记录：

```text
B9E073E7283AB4C4288FB646FE516866CFE8EC04E27142F3C12EA9CAA78D5704
```

原始 ZIP、RAR 和用户本机文件未修改。本次新文件只生成在独立的 1.2.8 候选目录。

## 用户规则及实现

**正常压缩：** Single / Dual 的阈值仍然分开保存与恢复，互不覆盖。本次没有把正常压缩模式的阈值合并。

**Limiter：** Single → Dual 时，将当前 Single Threshold 接续到 Dual DOWN Threshold；Dual → Single 时，将当前 DOWN Threshold 接续到 Single Threshold。不得因选择另一模式而召回它以前的旧向下阈值。

Makeup、Mix、Output Gain 在现有 Limiter 参数库中已经为 Single / Dual 共用。本次保持这组值原样，不改写、不归零、不根据另一模式重新补偿，不根据 Output 正负反推阈值。Limiter Link、声道 Link、Ratio Link 均不作为执行该连续性规则的条件。

ST、L、R、M、S 分别接续各自当前值，保留声道差异。Single Range 和 Dual UP gate 如与接续的向下阈值发生碰撞，沿用既有的边界推移/相等规则；Range OFF 保留。Single/Dual 的 Ratio 与算法设置没有合并。

为兼容 1.2.7 工程，旧参数 ID 和序列化字段仍保留；“保持一致”在此落实为 Limiter 每次切换均使用当前向下阈值，不再恢复目标模式的旧向下阈值。Normal / Limiter 切换、A/B、完整工程恢复和 Undo/Redo 是独立的状态恢复操作，不按一次新的 Single/Dual 切换重复搬运参数。

## 代码改动范围

| 文件 | 修改 |
|---|---|
| `CMakeLists.txt` | 版本改为 1.2.8，登记新增头文件。 |
| `Source/LimiterModeContinuity.h` | 不依赖 JUCE 的阈值接续与碰撞规则。 |
| `Source/PluginProcessor.h` | 新增接续入口及已发布的 Limiter Single/Dual 模式状态。 |
| `Source/PluginProcessor.cpp` | 在 Limiter mode 参数回调中接续目标阈值；先更新边界，再发布音频读取的新模式；识别状态恢复/Undo；快照记录已发布的模式。 |
| `Source/LimiterMode.cpp` | GUI 模式切换入口：整理旧编辑、开启同一 Undo 事务、更新并同步目标参数；音频读取使用已发布的模式。 |
| `Source/PluginEditor.cpp` | 仅 Limiter 按钮路径调用新入口；正常模式保留原切换路径；切换后更新 Link 编辑参照。 |

原 `Parameters.h` 和其余 22 个未涉改 Source 文件逐字节保持一致，包括压缩算法、ABTransfer、输出联动编辑代码和计量模块。`createParameterLayout`、`processBlock`、`applySnapshot`、`setStateInformation`、`getStateInformation`、原 `timerCallback` 的函数内容在归一化换行后相同。参数 ID/顺序、插件身份及 state schema 22 保留。详情见 `Verification/1.2.8/source-invariants.log` 和 `protected-source-hashes.json`。

这些源码一致性检查不等同于完整插件的音频零残差验证。

## 本轮实际执行的验证

隔离测试使用**从当前工程提取的实际 C++ 函数体**，并用简化的 APVTS、参数转换、消息线程和 UndoManager 替身承载。编译器为 GCC 14.2.0，C++17，启用 AddressSanitizer 和 UndefinedBehaviorSanitizer。它不是完整 JUCE 插件，也没有运行真实的 DAW。

| 项目 | 结果 |
|---|---|
| 1.2.7 原代码负向对照 | 成功复现问题：Single → Dual 后 DOWN 为 0 dB，而应接续的当前阈值为 -12 dB。测试按预期以失败码 1 退出。 |
| 1.2.8 核心函数场景 | 864 组通过，覆盖两种参数回调/原始值更新时间顺序，Link 开/关，ST/LR/MS，算法，Output -10/0/+18 dB，Mix 0/37/100，以及双向和反复切换。 |
| 正常压缩独立记忆 | 100 次 Single / Dual 切换后，各自阈值保持不变。 |
| 边界规则检查 | 252,021 次接续计算通过，覆盖 -120～0 dB 的 0.01 dB 网格、碰撞、Range OFF 和往返保持。 |
| 无编辑器的工作线程回调 | 隔离环境通过：无需等待计时器即可读取接续后的有效阈值，回调未递归写入宿主参数。 |
| 恢复/撤销抑制分支 | 替身环境通过。只验证分支行为，未验证真实 JUCE Undo 或工程序列化。 |
| 内存/未定义行为检查 | 上述隔离测试未报告 ASan/UBSan 错误。 |
| 参数布局与未涉改源码 | 通过，记录见对应日志。 |

日志：`Verification/1.2.8/isolated-current.log`、`isolated-baseline.log`。

复跑隔离检查：

```bash
python tests/limiter_mode_continuity_isolated_test.py --sanitize
```

将 `--source` 指向原始 1.2.7 工程可复跑负向对照，该对照应在阈值接续检查失败。

## 尚未完成的验证

实际尝试用 CMake 配置完整工程，但当前环境没有本次构建使用的 JUCE SDK，配置在“JUCE was not found”处停止。记录见 `Verification/1.2.8/full-build-probe.log`。

**以下均尚未完成，不能视为通过：** 完整 JUCE/MSVC 编译、Windows VST3 构建、真实编辑器控件与手势、真实 Undo/Redo、A/B 与即时保存/恢复回归、实际宿主自动化、音频渲染/爆音检查、Cubase 使用验证、macOS 构建。

已补充完整 JUCE 回归代码 `tests/limiter_mode_continuity_checks.inc`，挂到现有 `QQSCLimiterCheck` 目标中，但本轮没有运行该目标。其覆盖真实 APVTS、编辑器按钮、双向切换、正常模式独立记忆、Undo/Redo、A/B、保存/恢复及边界碰撞。

## Windows 构建入口

使用原 1.2.7 构建所用 JUCE 8.0.15 和 Visual Studio 2022 C++ Build Tools。包中提供 `BUILD_WINDOWS.cmd` / `BUILD_WINDOWS.ps1`，源码定位以脚本自身目录为准，不再指向旧 1.2.7 工作区。

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\BUILD_WINDOWS.ps1 -JucePath "D:\path\to\JUCE-8.0.15"
```

默认构建当前 VST3 与 QQSCLimiterCheck，并执行新的 `continuity` 用例。辅助脚本本身也未在 Windows 实机验证。它不自动安装或发布插件，也不执行 Plan B。

编译成功后，新 VST3 预期位于：

```text
build-1.2.8-windows\QQSuperCompression_artefacts\Release\VST3\QQ Super Compression.vst3
```

真实新增回归的手动命令：

```powershell
.\build-1.2.8-windows\QQSCLimiterCheck_artefacts\Release\QQSCLimiterCheck.exe .\build-1.2.8-windows\verification\continuity continuity
```

还应运行原有回归：`QQSCLimiterCheck <目录> quick`、QQSCUnityCheck、QQSCLoudnessCheck，以及同参数设置下的旧/新 VST3 对照。真实按钮切换与正在播放时的宿主自动化需要 Cubase 实测。

确认 Windows 编译和测试后再替换插件；替换前退出所有宿主，保留用户已经确认的 1.2.7 成品作为回滚版本。

## 包内历史与交付状态

旧版 README、构建日志、成品哈希、安装记录和旧路径脚本均保留于 `docs/history/1.2.7-verified-baseline/`，不作为 1.2.8 已构建或已安装的凭据。历史 PDF 未修改，不是新修订的 1.2.8 说明书。

交付为完整项目源码、既有资源和文档，加上本次新测试/说明/构建入口。与输入工作区 ZIP 一样，本包不含 JUCE SDK。**没有附带冒充 1.2.8 的旧 VST3；没有安装、上传 GitHub、正式发布或执行 Plan B。**

## API 核对参考

实现时核对了 JUCE 官方的 APVTS Listener（回调应使用传入的新值）和 UndoManager（事务分组及 isPerformingUndoRedo）接口。参考只用于 API 语义核对，不替代本项目验证：

- https://docs.juce.com/master/structjuce_1_1AudioProcessorValueTreeState_1_1Listener.html
- https://docs.juce.com/master/classjuce_1_1UndoManager.html
