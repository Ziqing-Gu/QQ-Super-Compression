# QQ Super Compression 1.2.9 — 修复与验证说明

日期：2026-09-28。状态：**完整源码候选，未生成新的 Windows VST3，未执行完整 JUCE/Cubase 验证。**

## 基线与交付范围

基线是本会话用户确认、随后标为 Stable 并完成 Plan B 的 1.2.8。当前工作仅使用本会话上传的完整源码 ZIP，没有读取用户本机的 Plan B 目录，也没有修改上传 ZIP、已安装插件或 GitHub 仓库。

输入 `QQSuperCompression-1.2.8-Limiter-Mode-Continuity-Source.zip` 的 SHA-256：

```text
A8A7C812647E96186FB3D3E0A0F7C52919F59E7D83D963B3C955EBAB5F121655
```

本轮改动：首次进入 Limiter 不再复制普通参数；Normal Makeup ±30 dB / Limiter ±120 dB；恢复 Ratio、Mix 的完整参考 Output 联动。旧的压缩曲线补偿规则保留，没有改成绝对正负配对。

新实例第一次进入 Limiter 为其独立默认设置（Makeup 0 dB，Mix 100%，Output 0 dB）。之后恢复各自设置。**已经使用并保存的 Limiter 参数不会因升级被清零**；验证首次默认请使用新实例。旧版未使用参数库的 −120 dB Single 阈值占位符迁移为 0 dB。

Limiter Single/Dual 沿用 1.2.8 稳定行为：当前阈值接续，Makeup/Mix/Output 不改变，不受 Link 开关影响。正常模式继续分开记忆 Single/Dual 阈值。

## 联动实现

参考计算沿用实际 Single/Dual 静态曲线，并计入湿声 Makeup 与线性干湿混合：

```text
reference[d] = (1 − Mix[d]) + Mix[d] × MakeupLinear[d] × CurveGain[d](检测电平=1)
Output_new = Output_anchor + Reference_anchor_dB − Reference_new_dB
```

LR 用较大的通道参考值，MS 沿用两个参考界的保守和。此计算不读取正在播放的音乐响度，也不承诺 LUFS 或所有电平不变。UP Ratio 单独变化、不影响参考点时补偿为 0；Ratio Link 带动 DOWN 时按更新后的完整曲线计算。多参数联动的中间回调不单独补偿。手动设置的 Output 偏移保留。

显式 GUI 编辑加入 Output 伴随手势；源参数、联动参数、Output 的自动化轨道应一起录制/回放。单独宿主写值、工程恢复或 Undo/Redo 不触发新补偿。参数量化和 Output ±120 dB 边界可能限制等式的精确保持。纯干声、无阈值响应时，反向调 Output 不会伪造阈值变化。

## 实际执行的检查

所有“通过”仅对应下表注明的环境。隔离测试提取当前源文件中的实际 C++ 函数体，使用 GCC 14.2.0、C++17、AddressSanitizer 与 UndefinedBehaviorSanitizer。APVTS、Slider、Undo、参数归一化/通知与部分边界适配器为简化替身；它们不等同真实 JUCE 或宿主。

| 检查 | 结果与范围 |
|---|---|
| 首次进入 / 重入参数库 | 4 组通过；包含隐藏 Limiter 已有值，实际 `enterLimiterMode()` 不改写两套设置。 |
| 完整参考编辑 | 4,896 次通过；实际编辑包装、Ratio/声道互联、补偿函数；覆盖 Single/Dual、ST/LR/MS、Classic/Super、Link/Domain Link/Ratio Link、Mix 0/37/100 起点。 |
| 参考计算独立数值对照 | 1,200 组通过；固定检测电平使用独立 double 计算对照，涵盖 Makeup ±120 dB、Mix 端点/接近端点、4 个阈值。 |
| 额外联动回归 | 数字提交的 Dual 成组更新、上下限/互反 Ratio 约束、36 次混合参考反向 Output 编辑、200 次 Mix 往返、门限相等、有限 Range、已讨论的 24.30 dB 示例、Output 上下限通过。 |
| 状态/自动化回调保护 | 替身环境中被动更新、恢复标志、Undo 标志不会二次补偿；伴随手势计数平衡。不是实测 JUCE Undo 或真实自动化回放。 |
| 1.2.8 连续性 | 原隔离测试通过：864 组模式切换、两种 APVTS 回调顺序、100 次普通模式切换独立记忆、252,021 次边界策略检查，以及工作线程发布/恢复抑制。 |
| 负向对照：旧入口 | 将原 1.2.8 `enterLimiterMode()` 单独换回当前测试支架，按预期失败：首次进入复制参数。不是重新构建整套 1.2.8 插件。 |
| 负向对照：旧参考 | 将原 1.2.8 的 wet-only Link 参考包装单独换回，按预期失败：参考与 Output 不守恒。 |
| 源码不变量 | 参数 ID 定义、压缩引擎、ABTransfer、LUFS 核心、1.2.8 连续性核心未改；`processBlock` / `processBlockInternal` 等函数内容未改；CMake 除版本外相同；结构括号检查通过。 |
| 完整 CMake 配置 | **未完成。** 实际配置在 “JUCE was not found” 处终止，故不能视为完整 C++/JUCE 编译通过。 |

隔离回归未报告 ASan/UBSan 错误。源码相同检查不等同音频零残差验证。

原始日志：`Verification/1.2.9/isolated-full-link.log`、`isolated-continuity.log`、`negative-old-entry.log`、`negative-old-wet-reference.log`、`source-invariants.log`、`full-build-probe.log`。负向日志中的 FAIL 是预期结果，不是本版正向测试失败。

## 尚未完成的验证

**完整 JUCE/MSVC 编译、Windows VST3、真实编辑器/文本/滚轮/键盘、真实 Undo/Redo、A/B 与工程序列化、Cubase 自动化/回放/渲染、音频峰值/爆音与听感、macOS 构建均未在本次环境执行。**

真实 JUCE 回归已经写入 `tests/independent_banks_full_link_checks.inc`，接到 `QQSCLimiterCheck ... revision129`。它测试首次独立/重入、真实参数范围、数值输入/显示、旧状态与 A/B 范围迁移、GUI 成组补偿、数字 Dual Undo/Redo、键盘与被动写值。`continuity` 保留上一版回归，`dual` 保留双压独立算法与音频检查。本轮没有把这些未执行的测试标成通过。

历史 `strictOutputLinkChecks`、旧 `run` / `ratioMixChecks` 等“首次克隆、Ratio/Mix 不联动”断言不再符合用户本次要求，保留源码作为历史规格，不再从默认验收路径运行。

## Makeup 范围兼容性

Normal 五域宿主参数确实改为 ±30 dB，不只是缩小 UI。工程内物理 dB 与 A/B Snapshot：±30 dB 内保留，超出部分钳到边界；Limiter 原范围与已保存值不变。状态 schema 升为 23，参数 ID/排列、插件身份不变。

**DAW 自己保存的 0～1 归一化自动化可能因新范围产生不同 dB 映射，插件无法替宿主改写外部曲线。**升级前保留 1.2.8 与工程副本，特别核对已经写过普通 Makeup 自动化的轨道。只读取工程内物理参数成功，不代表这些外部自动化已经迁移。

## 本机复跑与构建

从解压后的源码根目录执行：

```text
python tests/independent_banks_full_link_isolated_test.py --sanitize
python tests/limiter_mode_continuity_isolated_test.py --sanitize
python tests/revision129_source_audit.py --baseline <1.2.8源码ZIP或目录>
```

隔离测试需要可用的 GCC/Clang；不是 Windows 安装步骤。Windows VST3 使用已有 JUCE 8.0.15 与 VS2022：

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\BUILD_WINDOWS.ps1 -JucePath "D:\path\to\JUCE-8.0.15"
```

也可双击 `BUILD_WINDOWS.cmd`，脚本使用自己的目录作为源码路径；默认新建 `build-1.2.9-windows`，编译 VST3 与测试，运行 `revision129`、`continuity`、`dual`。构建成功后的预期成品：

```text
build-1.2.9-windows\QQSuperCompression_artefacts\Release\VST3\QQ Super Compression.vst3
```

脚本本身本次也未在 Windows 执行。它不自动覆盖插件，不做 Plan B、源码同步或发布。关闭 Cubase/其他宿主后才手动替换，保留 1.2.8 Stable 回滚版本。

## 变更文件与记录

产品源码改动集中在 `DynamicsLimits.h`、`PluginProcessor.h/.cpp`、`LimiterMode.cpp`、`PluginEditor.h/.cpp`、`LimiterEditor.cpp`，以及 CMake 版本。补充新测试、更新测试调度与构建脚本，旧源码/构建元数据另存历史目录。完整差异见 `Verification/1.2.9/source-changes.diff`。

1.2.8 的 Stable / Plan B 为用户后续确认，历史交付文件仍按原样保留，不伪造本机验证日志。本次未附旧 VST3 冒充 1.2.9。PDF 说明书仍是历史文件，没有把它们声称为本版新说明书。

## API 核查

核对的是 JUCE 官方 8.0.15 的 Slider、ParameterAttachment 与 APVTS Attachment 源码：原生值回调中附件先更新参数；数字输入的解析发生在原生提交之前；SliderAttachment 重绑会重设范围和转换器。这些接口核查用于实现，不替代本机集成验证。

- https://github.com/juce-framework/JUCE/blob/8.0.15/modules/juce_gui_basics/widgets/juce_Slider.cpp
- https://github.com/juce-framework/JUCE/blob/8.0.15/modules/juce_audio_processors/utilities/juce_ParameterAttachments.cpp
- https://github.com/juce-framework/JUCE/blob/8.0.15/modules/juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.cpp
