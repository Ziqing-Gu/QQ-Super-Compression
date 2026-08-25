# AI 持续开发交接规范

> 本文件用于让不同 AI、不同开发者在接手同一个软件项目时，能够快速理解项目当前状态、历史决策、已解决问题、已知风险和后续开发方向。
>
> **本文件必须长期保留在项目源代码根目录中，并随着项目持续更新。**
>
> 无论由哪一个 AI 继续开发，只要修改了代码、版本、架构、功能、构建方式或重要行为，就必须同步补充本文件中的开发记录，不能只修改代码而不留下过程。

---

## 1. 本文件的目的

这个项目可能会由不同 AI 在不同时间继续开发。

为了避免出现以下问题：

- 新 AI 不知道之前已经做过什么；
- 重复实现已经完成的功能；
- 把已经解决的问题重新引入；
- 不知道某个设计为什么这样做；
- 不知道哪个版本最稳定；
- 出现问题时不知道应该回滚到哪里；
- 修改代码后没有留下说明，导致下一次接手困难；

因此，项目必须始终保留一份**连续、累积、可追溯的开发记录**。

---

## 2. 所有接手项目的 AI，开始开发前必须做的事

在修改任何代码之前，必须先完成以下步骤：

1. 阅读本文件全文。
2. 阅读项目根目录中的 README、CHANGELOG、PATCH NOTE、HANDOFF、BUILD 或其他开发说明文件。
3. 确认当前：
   - 最新版本；
   - 稳定基线版本；
   - 当前开发候选版本；
   - 已完成的功能；
   - 已知问题；
   - 尚未完成的功能；
   - 构建方式；
   - 测试方式；
   - 回滚方案。
4. 不要仅凭代码表面行为猜测历史设计意图。
5. 如果历史记录与当前代码存在冲突，应先指出冲突，再决定如何处理。
6. 如果用户已经明确指定某个版本为稳定基线，应优先基于该稳定基线继续开发，除非用户明确要求更换基线。

---

## 3. 开发过程中必须遵守的原则

### 3.1 不要破坏已经验证稳定的功能

新增或修改功能时，应尽量做到：

- 修改范围最小化；
- 避免无关重构；
- 避免为了修一个问题而重写大块已经稳定的代码；
- 不要随意改变已有用户工作流；
- 不要在用户没有要求时改变参数含义、默认值、界面逻辑或工程兼容性。

如果必须进行结构性修改，应在开发记录中明确写明：

- 为什么必须改；
- 改了什么；
- 可能影响哪些旧功能；
- 如何验证没有产生回归。

### 3.2 区分“稳定版本”和“开发候选版本”

- **Stable / 稳定基线**
  - 已经过用户实际测试并确认；
  - 后续开发和回滚优先以此为基础。

- **Candidate / 开发候选**
  - 新修改版本；
  - 尚未经过用户完整验证；
  - 在用户确认前，不应自动替代稳定基线。

只有用户明确确认后，才能把新的候选版本升级为稳定基线。

### 3.3 不要删除历史记录

开发记录必须采用**累积追加**方式。

禁止：

- 为了“整理”而删除旧版本记录；
- 用新结论覆盖旧过程；
- 删除曾经失败的方案；
- 删除已经解决的问题记录；
- 只保留当前版本而抹掉之前历史。

失败过程同样重要，因为它可以避免未来再次走同样的弯路。

---

## 4. 每次修改代码后，AI 必须完成的事情

只要修改了任何具有实际意义的代码，就必须在交付前更新开发记录。

至少要记录以下内容：

### 4.1 版本信息

```text
版本号：
日期：
开发状态：Stable / Candidate / Test
基于版本：
```

### 4.2 本次开发目标

说明：

- 用户提出了什么问题；
- 为什么需要修改；
- 预期解决什么。

不要只写“修复 Bug”，应写清楚 Bug 的实际表现和应用场景。

### 4.3 问题原因

如果已经定位原因，应记录：

- 根因是什么；
- 为什么旧版本会出现这个问题；
- 是算法问题、状态管理问题、UI 问题、宿主兼容问题、线程问题、延迟问题，还是其他原因。

如果原因尚未完全确定，应明确写：

> 当前属于推测 / 尚未完全确认

不要把推测写成事实。

### 4.4 本次具体修改

记录：

- 修改了哪些模块；
- 新增了哪些状态、参数、类、函数或数据结构；
- 删除了哪些旧逻辑；
- 哪些行为保持不变。

如果修改涉及重要架构，应写清楚数据流或信号流。

### 4.5 为什么这样修改

不仅要记录“做了什么”，还要记录“为什么这样做”。

例如：

- 为什么不用另一个方案；
- 为什么保留旧结构；
- 为什么采用某种算法；
- 为什么某个功能必须放在某个处理阶段。

### 4.6 验证情况

必须区分：

#### 已实际验证

例如：

- Windows + 某 DAW 实测通过；
- macOS 编译通过；
- VST3 / AU 扫描正常；
- 用户 AB 测试确认。

#### 仅静态检查

例如：

- 源码静态检查通过；
- 单元测试通过；
- ZIP 完整性检查通过；
- 当前环境无法实际编译。

**绝对不要把“静态检查通过”写成“插件已经实际验证通过”。**

### 4.7 已知问题 / 未解决问题

如果仍然存在问题，必须留下。

### 4.8 回滚方案

必须记录：

- 如果新版本失败，应回滚到哪个版本；
- 哪个版本是当前稳定基线；
- 哪些文件或结构不应回退。

---

## 5. 推荐的版本开发记录格式

以后每次开发完成，都在本文件末尾追加一段：

```markdown
---

## vX.X.X — 版本名称

**日期：** YYYY-MM-DD  
**状态：** Candidate / Stable / Test  
**基于：** vX.X.X

### 用户需求

描述本次用户提出的问题或功能需求。

### 问题表现

描述实际出现了什么。

### 根因

说明定位到的原因。

### 修改内容

- 修改 1
- 修改 2
- 修改 3

### 保持不变

- 不影响……
- 没有修改……
- 继续沿用……

### 验证

- [x] 静态检查
- [x] 编译
- [ ] DAW 实测
- [ ] 用户确认

### 已知问题

如果没有：

> 暂无新增已知问题。

### 回滚

如果本版本失败，优先回滚到：

> vX.X.X — 稳定基线

### 后续建议

记录下一位 AI 最需要知道的事项。
```

---

## 6. 推荐的项目文档结构

```text
Project Root/
│
├─ README.md
├─ AI_DEVELOPMENT_HANDOFF.md
├─ CHANGELOG.md
├─ BUILD.md
├─ Source/
├─ Assets/
└─ ...
```

其中：

- `README.md`：面向普通用户或开发者，说明软件是什么、怎么使用、怎么构建。
- `AI_DEVELOPMENT_HANDOFF.md`：记录设计意图、开发历史、稳定基线、失败经验、AI 接手规则、下一步注意事项。
- `CHANGELOG.md`：简洁记录各版本变化。

本文件应比 CHANGELOG 更详细，尤其要保留：

> **为什么改、怎么改、哪些方案失败过、哪些行为不能破坏。**

---

## 7. AI 接手时的行为要求

### 必须做

- 先读历史，再改代码；
- 尊重当前稳定基线；
- 理解现有架构后再修改；
- 每次修改后补充开发记录；
- 保留旧记录；
- 清楚区分事实、推测和未验证内容；
- 清楚区分静态检查、编译成功和用户实际验证；
- 如果改变重要设计，应记录原因；
- 如果发现旧文档已经过时，应修正文档，但不要抹去历史。

### 禁止做

- 不读历史直接大规模重构；
- 删除旧开发记录；
- 把候选版本擅自标记为稳定版本；
- 未经用户确认改变核心工作流；
- 只交付代码，不更新交接记录；
- 把“理论上可行”写成“已经验证”；
- 因为看不懂旧代码就直接重写整个项目。

---

## 8. 对未来 AI 的特别提醒

> **每一次更新代码后，都必须留下本次开发经过。**

这里的“开发经过”不仅是最终结果，还包括：

- 用户为什么提出这个修改；
- 出现了什么问题；
- 如何定位；
- 尝试过什么方案；
- 哪些方案失败；
- 最终为什么选择现在的实现；
- 哪些功能已经确认稳定；
- 哪些仍未验证；
- 如果失败应该回滚到哪里。

这样做的目的不是写形式化文档，而是为了让下一位 AI 能够真正接着当前进度继续开发，而不是重新猜一遍整个项目。

---

## 9. 最重要的长期规则

1. **源码可以变化，但开发历史不能丢。**
2. **每个新版本都必须知道自己基于哪个稳定版本。**
3. **用户确认之前，新版本只是候选版本。**
4. **任何 AI 修改代码后，都必须更新开发记录。**
5. **记录“为什么这样做”，比只记录“改了什么”更重要。**
6. **失败方案也要保留，因为它们属于项目知识的一部分。**
7. **如果无法完成真实编译或 DAW 实测，必须明确说明，不能假装已经验证。**

---

# 当前项目开发状态

> 这一部分由当前项目维护者填写，并在每次重要版本更新后同步修改。

```text
项目名称：QQ Super Compression
当前稳定版本：暂无（尚未由用户明确指定 Stable）
当前候选版本：0.1.8 — GR Hold Readability / Uniform 1:1 UI Scaling
主要平台：Windows / macOS
插件/程序格式：VST3
主要开发环境：JUCE 8 / CMake / C++17
主要测试环境：Windows + Cubase / PluginDoctor（由用户/Codex 实际构建与验证）

当前已完成：
- 0.1.3 UI/工作流：Meter 面板瘦身、窗口尺寸记忆、A/B、A→B/B→A、Shift 微调、Alt 默认、Undo/Redo、按钮式 Bypass、循环 Mode、LR/MS 独立 Makeup。
- Dynamic Display：Dry/Input、Wet pre-Makeup、最终 post-Mix Output。
- ST / MS / LR 三模式；三组双通道 Input / Output / Gain Reduction Meter；MS 显示 M/S；GR 从顶部向下。
- 0.1.4：新增 LOOKAHEAD (ms) 可编辑 TextEditor，范围 0.0–100.0 ms，默认 5.0 ms。
- 0.1.4：Lookahead 同时决定未来分析窗口长度和真实音频路径延迟/PDC。
- 0.1.4：移除活跃 DSP 中固定 20 ms rolling RMS detector，改为 L/R/M/S 四域未来窗口 sliding peak 分析；使用预分配单调最大值队列，稳态 audio thread 不分配内存。
- 0.1.4：Bypass 继续走相同 Lookahead delay，并保持与 Active 完全相同的 latency report。
- 0.1.4：Lookahead 纳入工程状态和 A/B snapshot。
- 0.1.4 用户实际 PluginDoctor 结果：5 ms≈99.6 Hz、10 ms≈49.8 Hz、20 ms≈24.9 Hz；26 ms 为约 20 Hz 分水岭；40 ms 对 20 Hz 更低失真，80 ms 明显更干净；0 ms 虽有谐波但用户要求作为口味保留。
- 0.1.5：Lookahead UI 收敛为 0 / 10 / 26 / 40 / 80 / 100 ms 六档 ComboBox。
- 0.1.5：已有工程/实例恢复自己的保存值；新实例默认使用用户最后一次手动选择，首次无偏好时 fallback=26 ms。
- 0.1.6：Match 已从 integrated power/RMS-equivalent 完整替换为 BS.1770 / EBU R128 gated Integrated LUFS：K-weighting、400 ms block、75% overlap、-70 LUFS absolute gate、-10 LU relative gate。
- 0.1.6：ST 按 stereo Integrated LUFS；LR 与 MS 分别对 L/R、M/S 做独立 mono gated-LUFS 并只写入有效通道/分量的 Makeup。
- 用户后续实际确认 0.1.6 LUFS Match 没问题；但没有明确指定 0.1.6 为 Stable。
- 0.1.7：Gain Reduction Meter 新增固定 2 秒自动 Peak Hold；新更深峰重启计时，到期自动刷新；ST linked、LR 独立 L/R、MS 独立 M/S。
- 0.1.7：面板新增小而低调的版本号（来自 `JucePlugin_VersionString`）；修复用户 0.1.6 编译报告的非致命 `playHead` 名称遮蔽 warning。
- 0.1.8：Hold 第二行去掉冗余 `H` 并把字体 7.5→8.5；Hold marker/2 s timing 不改。
- 0.1.8：Editor 改为固定 1020x670 design root + 单一 uniform scale，并锁定 1020:670 aspect；旧非比例窗口保存值迁移成比例尺寸。
- Ratio law 继续保持 0.1.1 方向：gain = 1 / (1 + (Ratio - 1) * level)；0.1.8 不修改 Ratio 或 detector。
- UTF-8/CJK 跨平台规则继续保留。

当前已知问题：
- 用户 PluginDoctor 已确认 0.1.1–0.1.3 固定 20 ms rolling RMS 核心会显示类似 Attack/Release 的时间响应，并产生可见谐波，因此该 detector 已被判定不符合最终目标；历史必须保留，不能再当作最终核心。
- 0.1.4 已由用户实际构建并在 PluginDoctor 做了多频率/多 Lookahead 谐波测试；future-window peak 的频率/窗口关系已获得用户实测证据。0.1.5 的六档预设/记忆逻辑继续被 0.1.8 继承，需随当前版本做回归验证。
- Lookahead=0 ms 会退化为 one-sample instantaneous level，预期可能重新出现 waveshaping/谐波；保留它只是为了对比，不应误认为 0 ms 是最终无失真模式。
- 用户已实测：5 ms≈99.6 Hz、10 ms≈49.8 Hz、20 ms≈24.9 Hz 出现明显分水岭；26 ms 把分水岭推至约 20 Hz，40/80 ms 对 20 Hz 继续更干净。该结果必须保留为 0.1.4 的实际 PluginDoctor 验证。
- 运行中修改 Lookahead 会改变真实插件 latency，宿主可能发生一次 PDC realignment；这不是稳态 Attack/Release，但需 Cubase 实测。
- 0.1.6 已按用户明确要求将 Match 改为严格 Integrated LUFS；旧 0.1.3–0.1.5 RMS/power Match 仅保留在历史记录中，不得恢复为最终 Match。
- 用户已实际确认 0.1.6 LUFS Match 没问题；仍保留“未指定 Stable”的状态纪律。
- 0.1.7/0.1.8 GR Hold 尚需 Codex/Cubase 验证 2 秒刷新时序与 UI marker；Hold 只按 audio callback samples 推进，宿主完全停止 callback 时不会靠墙钟继续倒计时。
- 0.1.8 的统一 UI transform / fixed aspect ratio 尚需在 Cubase 验证 resize、鼠标 hit-test、旧非比例 saved size 迁移与不同系统 DPI。
- Ctrl/Cmd+Z 是否被宿主优先截获仍需实际 VST3/Cubase 键盘焦点验证。
- 尚无用户明确确认的 Stable 版本。

当前正在开发：
- 0.1.8 Candidate：基于 0.1.7，仅修 Hold 数值可读性与 Editor 真正 1:1 等比缩放；全部 audio DSP 不变。

当前回滚基线：
- 没有 Stable 回滚基线。
- 若 0.1.8 UI scaling 失败，优先回到 0.1.7 Candidate，只重做 Editor scaling；若 0.1.7 Hold 本身失败，再回到 0.1.6 Candidate 只重做 Hold/UI。用户已确认 0.1.6 LUFS Match 没问题，不回退 LUFS Match。
- 0.1.3 rolling RMS 仍不得作为最终算法回滚点。
- 永远不要恢复 0.1.0 已否决的 sample-domain abs(x)^(1/Ratio) waveshaper。

下一步建议：
- Codex 编译 0.1.8 时确认旧 `playHead` shadow warning 仍未回归，不趁机重构 future-window peak / LUFS Match。
- Cubase 验证 GR Hold：更深峰立即更新并重启 2 s、到期自动刷新，ST/LR/MS 域行为正确；确认 Hold 不影响声音。
- 验证面板 `v0.1.8` 足够小且低调；GR Hold 第二行无 `H` 且更易读。
- 拖动窗口验证 1020:670 比例锁定和完整 UI 统一缩放；控件 mouse hit-test 必须与视觉位置一致。
- 六档 Lookahead/PDC 与 0.1.6 LUFS Match 只做回归检查。
- 用户明确确认后，才把某个版本写成 Stable。
```

---

# 开发历史

> 从这里开始持续追加版本历史。
>
> **不要删除旧条目。**


---

## v0.1.0 — Prototype

**日期：** 2026-08-25  
**状态：** Test  
**基于：** 初始版本

### 用户需求

建立 QQ Super Compression 第一版 VST3 原型：Ratio、手动 Makeup、Makeup Gate、Mix、动态电平显示和 0/10 ms 延迟选择；同时注意 PDC/Bypass 和 macOS 中文乱码。

### 问题表现

首次实测发现只要 Ratio 开始工作就出现明显失真，而且 Ratio 越大音量反而越高。

### 根因

0.1.0 使用 `y = sign(x) * abs(x)^(1/Ratio)` 直接处理每个采样。对 0..1 的采样，指数小于 1 会把数值推向 1，因此 Ratio 增大反而增大电平；同时这是典型 waveshaper，会产生强谐波失真。

### 修改/结论

- 0.1.0 Ratio 方案被否决。
- 该失败方案必须长期保留记录，禁止未来重新引入。

### 验证

- [x] 用户 DAW 听感确认该 Ratio 方案失败。
- [ ] 不作为稳定版本。

### 回滚

无。初始失败原型。

---

## v0.1.1 — Ratio Engine Fix / Meter & UI Pass

**日期：** 2026-08-25  
**状态：** Candidate / Test  
**基于：** v0.1.0

### 用户需求

修复 Ratio 越大越响和严重失真；增加 Input / Output / Gain Reduction Meter；处理 UI 文字重叠和中文乱码。

### 根因

根因已确认是 0.1.0 sample-domain waveshaper Ratio，而不是过采样不足。

### 修改内容

- 改为固定 20 ms rolling RMS level-domain detector。
- Ratio 通过 detector-derived gain 乘到音频，不再直接 waveshape sample。
- 增加 Input / Output / Gain Reduction 三组 Meter。
- 改进 Dynamic Display 布局。
- 加强 UTF-8/CJK 字符串和字体路径。

### 保持不变

- 无传统 Threshold。
- 无用户 Attack/Release。
- 手动 Makeup / Mix。
- 0/10 ms latency 和 Makeup Gate 当时继续保留用于测试。

### 验证

- [x] 用户实际听感认为新压缩概念效果非常好。
- [x] 用户确认 10 ms latency 与 Makeup Gate 在实际使用中似乎没有意义，要求下一版删除。
- [ ] 用户未把 0.1.1 明确指定为 Stable。

### 回滚

可作为 0.1.2 失败时的直接前一 Candidate 对照，但不是 Stable。

---

## v0.1.2 — ST/MS/LR Modes / Zero Latency / Dual Meter Rework

**日期：** 2026-08-25  
**状态：** Candidate / Test  
**基于：** v0.1.1

### 用户需求

- 删除 10 ms latency，只保留 0 latency。
- 删除 Makeup Gate。
- Gain Reduction 从顶部 0 dB 向下显示。
- Input / Output / Gain Reduction 全部改成双通道。
- 新增 ST / MS / LR。
- MS 模式 Meter 从 L/R 自动改显示 M/S。
- Dynamic Display 黄色 Output 保持为 Makeup + Mix 后最终输出。

### 问题表现

0.1.1 的 10 ms Stable 和 Makeup Gate 经用户实际试用没有体现出足够价值；Meter 方向和单通道显示不符合新需求。

### 根因

这不是算法 Bug，而是经过实际试用后做出的产品设计简化和功能扩展。

### 修改内容

- 删除 latencyMode、StereoDelay、MakeupGate 及其 UI/状态。
- 插件固定 `setLatencySamples(0)`。
- 保留 0.1.1 Ratio law 和 20 ms rolling detector。
- 新增 processingMode = ST / MS / LR。
- ST：L/R detector 各自运行，较大 GR 控制共同 gain，保持 stereo image。
- LR：L/R 独立压缩。
- MS：`M=(L+R)*0.5`, `S=(L-R)*0.5`，M/S 独立压缩后精确解码。
- L/R/M/S 四套 detector 始终保持 warm，降低模式切换冷启动风险。
- 三组 Meter 全部双通道；MS 显示 M/S。
- GR 由顶部向下增长。
- Dynamic Display 删除 Gate 线，但保留 Dry / Wet pre-Makeup / post-Mix Output。
- 继续保留 UTF-8/CJK 开发规则。

### 保持不变

- Ratio 核心公式不变。
- 20 ms detector 不变。
- 无传统 Threshold。
- 无用户 Attack/Release。
- Makeup 手动。
- Mix 位于最终处理阶段。

### 为什么这样修改

删除两个未体现实际价值的实验功能可以降低 UI、状态和 PDC 复杂度；三模式则扩展声道域控制能力。ST 使用 strongest-channel linking 是为了避免独立 L/R Gain Reduction 导致声像移动。MS 使用 0.5 encode 是为了让纯 Mid / pure Side 的 detector level 与 L/R 数值保持更接近。

### 验证

- [x] 源码静态检查。
- [x] CMake 静态配置检查可正常执行到预期的 JUCE-not-found 防护分支；在该分支之前未发现 CMake 语法错误。
- [x] CMake 活跃 target 中已移除 MakeupGate / StereoDelay。
- [x] CMake 配置静态检查：`QQSC_FETCH_JUCE=OFF` 可正常执行到预期 JUCE-not-found 防护分支，在此之前未发现 CMake 语法错误。
- [x] `StaticCompressionEngine` 使用最小 JUCE stub 完成独立 C++17 编译/运行检查；这只验证该头文件语法和队列基本运行，不等于完整插件编译。
- [ ] 当前 AI 环境真实 JUCE 编译。
- [ ] VST3 扫描。
- [ ] Cubase DAW 实测。
- [ ] 用户确认。

### 已知问题

- ST strongest-channel linking 相比 0.1.1 平均 linked detector 可能更强，需要试听。
- 模式切换没有额外 crossfade；虽然 detector 持续 warm，但仍需实测 click/pop。
- M/S 模式归一化的主观一致性待验证。

### 回滚

当前没有用户确认的 Stable 版本。若 0.1.2 失败，优先回到 0.1.1 Candidate 对照；绝对不要恢复 0.1.0 waveshaper Ratio。

### 后续建议

优先让 Codex 构建 0.1.2 VST3，并在 Cubase 验证 ST/LR/MS、Meter、0 latency、模式切换和 Dynamic Display。


---

## v0.1.3 — Workflow / A-B / Match / Independent Makeup

**日期：** 2026-08-25  
**状态：** Candidate / Test  
**基于：** v0.1.2

### 用户需求

用户在 0.1.2 已能加载运行的基础上要求继续完善工作流和 UI：

- 右侧 Meter 面板宽度明显缩窄，高度不变，为左侧 Dynamic Display 腾出更多空间；
- 删除标题下方 `Threshold-free level-domain compression / no Attack or Release / zero latency` 副标题；
- 记住用户上一次调整的插件窗口大小，下一次打开继续使用；
- 新增 A/B 对比及 A→B、B→A；
- 所有旋钮支持 Shift+拖动微调、Alt+左键恢复默认、Ctrl+Z 撤销、Ctrl+Shift+Z 重做；
- Bypass 改为普通按钮；
- Mode 改为单按钮点击循环；
- LR 模式允许 L/R 分别调 Makeup，MS 模式允许 M/S 分别调 Makeup；
- 新增 Match：后台统计从播放期间的压缩前后总体电平，点击后自动写入 Makeup；LR 和 MS 必须分别计算/调整两路。

### 问题表现 / 设计原因

这次不是 Ratio 算法 Bug，而是 0.1.2 在实际 UI 使用后的工作流增强。0.1.2 右侧 Meter 面板占据过多水平空间，顶部副标题视觉冗余；单一 Makeup 也无法充分发挥 LR/MS 独立处理模式。用户还希望将常用对比、复制、撤销和自动补偿流程直接集成进插件。

### 根因

- Meter 区宽度由 `visualRow.getWidth()/3` 再限制到 270–320 px，导致 0.1.2 视觉上过宽。
- Editor 每次构造固定 `setSize(1020, 670)`，没有持久 UI size preference。
- 0.1.2 只有一个 `makeupGainDb` 参数，因此 LR/MS 虽然压缩独立，Makeup 仍共享。
- APVTS 之前使用 `nullptr` UndoManager，且 UI 没有统一键盘/鼠标 modifier 工作流。
- 没有 A/B snapshot 和播放期 integrated level accumulator。

### 修改内容

- Meter 面板宽度改为约 190–220 px 动态范围，高度仍为 350 px；Dynamic Display 获得更多横向区域。
- 完全移除标题下副标题组件和显示文本，并把顶部 header 从 84 px 收紧到 70 px。
- 新增 per-user `juce::PropertiesFile`，保存 `editorWidth/editorHeight`；下一次打开 editor（包括新实例）恢复上次尺寸。
- 新增 A/B snapshot：保存 Ratio、ST/L/R/M/S Makeup、Mix、Mode；Bypass 不属于 A/B。
- 新增 A、B、A→B、B→A 按钮；A/B snapshot 随项目状态保存。
- APVTS 接入 `juce::UndoManager`；开启 `EDITOR_WANTS_KEYBOARD_FOCUS`；插件仅消费 Ctrl/Cmd+Z 与 Ctrl/Cmd+Shift+Z。
- 新增 FineKnob：Shift+drag 提高鼠标拖动灵敏度分辨率；Alt+左键直接恢复该参数默认值。 Alt-reset 仍完整 begin/end Slider/APVTS gesture，确保 Undo/宿主参数手势路径不会因快捷重置被绕过。
- Bypass 使用 TextButton + ButtonAttachment，不再使用 ToggleButton checkbox 风格。
- Mode 使用 TextButton，单击按 ST→MS→LR→ST 循环。
- 保留旧 `makeupGainDb` 作为 ST/common Makeup，新增 `makeupGainLDb`, `makeupGainRDb`, `makeupGainMDb`, `makeupGainSDb`，降低 0.1.2 Candidate 工程参数迁移风险。
- LR：压缩后对 L/R 分别应用 Makeup；MS：对压缩后的 M/S 分别应用 Makeup，再 decode 回 L/R；ST：继续使用一个共同 Makeup。
- 新增播放期 Match accumulator，L/R/M/S/ST 五类结果同步累计；检测源为 Dry 与 compressed Wet pre-Makeup，Mix/Makeup 不参与测量。
- Match：ST 写一个 common Makeup；LR 写 L/R；MS 写 M/S。
- Match 当前公式使用累计 power ratio（RMS-equivalent integrated level），不是 LUFS；结果限制在 Makeup 参数 ±36 dB 范围。
- Dynamic Display 三条线定义保持不变：Dry/Input、Wet pre-Makeup、final Output post-Makeup/post-Mix。
- Ratio law、20 ms rolling detector、ST/LR/MS compression topology、0 latency 均未修改。

### 为什么这样修改

- Meter 只需要保证 L/R 或 M/S 两条柱体清晰，不需要占据接近三分之一主界面宽度；缩窄后对核心 Dynamic Display 更有价值。
- UI size 属于用户个人界面偏好，不应污染音频工程参数，因此采用 per-user PropertiesFile，而不是把窗口尺寸作为 DAW automation parameter。
- LR/MS 已经是独立动态域，独立 Makeup 与其信号结构一致；ST 保持 linked/common Makeup，避免破坏 stereo image。
- Match 放在 Wet pre-Makeup 对比 Dry，才能直接计算“为了恢复压缩前总体电平需要多少 Makeup”，并且不受当前 Makeup/Mix 干扰。
- A/B 排除 Bypass，因为 Bypass 是全局试听状态，不应在切换两个参数方案时意外改变。

### 保持不变

- 不修改 0.1.2 Ratio 核心和 20 ms detector。
- 不增加传统 compression Threshold。
- 不增加用户 Attack/Release。
- 插件继续固定 0 sample latency。
- Mix 仍在最终 stage。
- 黄色 Output 仍是 Makeup + Mix 后最终输出。
- Meter 仍为 Input / Output / Gain Reduction 双通道；MS 自动显示 M/S；GR 仍从顶部向下。
- UTF-8、MSVC `/utf-8`、PingFang SC / Microsoft YaHei 规则继续保留。

### 验证

- [x] 基于 0.1.2 源码和项目交接文档完成修改。
- [x] 源码文本静态检查：旧 subtitle / Mode ComboBox / 单 Makeup active path 已移除或替换。
- [x] UTF-8 全工程文本解码检查通过。
- [x] CMake 版本与 keyboard focus 配置已更新；`QQSC_FETCH_JUCE=OFF` 配置流程可正常运行到预期的 JUCE-not-found 防护分支，在此之前未发现 CMake 语法错误。
- [x] 文档、CHANGELOG、BUILD brief、handoff 已同步更新。
- [ ] 当前 AI 环境真实 JUCE 编译：未完成；容器无法解析 github.com，且本机无 JUCE checkout。
- [ ] Codex/Windows Release 编译。
- [ ] VST3 扫描和加载。
- [ ] Cubase A/B、Undo/Redo、Match、LR/MS Makeup、窗口尺寸记忆实测。
- [ ] 用户确认。

### 已知问题 / 未验证风险

- Match 使用 integrated power/RMS-equivalent 而非 LUFS；如果用户后续要求严格感知响度/BS.1770，需要另行确认算法后再改。
- 如果宿主停止播放后完全不再调用 audio callback，processor 无法在“停止瞬间”收到一次明确 callback。当前新轮次重置结合 play-state transition、transport sample discontinuity，以及 Match 后的 reset request；必须在 Cubase 验证连续 Stop/Play 的实际行为。
- `EDITOR_WANTS_KEYBOARD_FOCUS=TRUE` 是为了插件内部 Undo/Redo。不同宿主对快捷键优先级不同，需确认 Cubase 是否会拦截 Ctrl+Z。
- A/B 和 Match 使用多参数批量写入，需要实际检查宿主 automation/gesture 显示是否符合预期。

### 回滚

当前仍没有用户明确指定的 Stable 版本。

若 0.1.3 失败，优先回滚到：

> v0.1.2 — Candidate / Test（用户截图已确认可在 DAW 中打开 UI）

不要回退 Ratio 到 0.1.0 waveshaper 方案，也不要恢复已由用户否决的 10 ms latency 或 Makeup Gate，除非用户之后明确要求重新评估。

### 后续建议

Codex 首次编译应优先处理任何 JUCE API 差异，不要趁机重构 DSP。编译成功后先测试 UI/交互，再测试 Match 的播放轮次统计和 LR/MS 独立补偿。


---

## v0.1.4 — Variable Lookahead Peak Experiment

**日期：** 2026-08-25  
**状态：** Candidate / Test  
**基于：** v0.1.3

### 用户需求

用户先通过 PluginDoctor Dynamics 发现 0.1.3 明显存在 Attack/Release-like 时间响应，随后通过 Harmonic Analysis 发现稳定正弦仍有可见 3rd/5th 等谐波。用户重新明确设计目标：QQ Super Compression 本质上不是传统压缩器，而是希望先预读一小段未来波形，再按照 Ratio 计算需要降低的整体电平并把原波形重新播放；不希望靠传统 Attack/Release 包络塑造动态。随后用户要求提供可直接输入毫秒数的 Text 窗口，便于快速比较不同预读长度的声音和 PluginDoctor 结果。最终确认：Lookahead 范围 0–100 ms，默认 5 ms，并让该值同时作为未来分析长度和真实插件 latency/PDC。

### 问题表现

- 0.1.3 在 PluginDoctor Dynamics 的阶跃测试中显示约 20 ms 的时间响应，视觉上等价于固定窗口带来的 Attack/Release-like 行为。
- 0.1.3 在约 521.5 Hz 稳定正弦测试中出现明显奇次谐波；用户认为如果只是按整体电平 Ratio 改变音量，稳定正弦应主要只改变幅度，不应被 detector 周期性调制成谐波。

### 根因

已确认 0.1.1–0.1.3 活跃核心使用固定 20 ms rolling RMS：

```text
sample -> square -> 20 ms moving mean -> sqrt -> Ratio gain -> multiply audio
```

这会同时带来两类问题：

1. 20 ms 历史窗口本身是因果时间窗口，因此输入电平阶跃后需要窗口内容逐步替换，形成可见的 Attack/Release-like 过渡；
2. 当测试正弦频率与 20 ms 窗口不是整周期关系时，moving RMS 会出现载波相关 ripple，Ratio gain 也随之抖动，乘回原正弦后形成幅度调制和新谐波/边带。

这不是“缺少过采样”导致的主要问题，因此本版不通过加入 oversampling 掩盖 detector 根因。

### 修改内容

- 删除活跃 DSP 中固定 20 ms rolling RMS detector 的使用。
- `StaticCompressionEngine` 改成用户可变窗口的 future-window peak analyser。
- 每个域使用预分配 monotonic maximum queue：对于被延迟 N samples 的当前输出样本，使用 `[n ... n+N]` 未来窗口中的最大绝对值作为 level。
- L/R/M/S 四域分析器持续运行，保持 ST/MS/LR 拓扑。
- 新增 `lookaheadMs` APVTS 参数：0.0–100.0 ms，0.1 ms step，默认 5.0 ms。
- UI 新增可编辑 `LOOKAHEAD (ms)` TextEditor；Enter 或失焦提交。
- Lookahead 毫秒数换算成 samples 后，同时设置：
  - analyser future window；
  - audio delay tap；
  - `setLatencySamples()` host PDC report。
- Bypass 不绕过 delay；Active/Bypass 始终共享同一实际 latency 和 host report。
- Lookahead 改变时，保留已有 delay history，并用最近 N samples warm-up 新 peak queue，避免人为插入一段新的静音；宿主仍可能因 latency 改变进行一次 PDC realignment。
- Lookahead 加入 A/B snapshot、A/B copy 和工程状态。
- Ratio law 暂时保持 `gain = 1 / (1 + (Ratio - 1) * level)`，本版只替换 detector，避免同时改变两项核心变量。
- 0 ms 明确保留为实验对照：窗口退化为瞬时 sample magnitude，预期可能重新出现明显谐波，不能把它当作无失真承诺。

### 保持不变

- 无 conventional compression Threshold。
- 无用户 Attack / Release 参数。
- 0.1.3 的 ST/MS/LR 拓扑、独立 Makeup、Mix 最终级、Dynamic Display 定义、双通道 Meters、Meter 面板尺寸、窗口尺寸记忆、A/B 工作流、Shift/Alt/Undo/Redo、按钮式 Bypass/Mode 均保留。
- Match 本版**故意不修改**。用户已经要求未来严格 LUFS Match，但随后明确要求等待其他行为确认后再一起改；因此 0.1.4 Match 仍是旧 integrated power/RMS-equivalent，不能称为 LUFS。
- UTF-8、MSVC `/utf-8`、PingFang SC / Microsoft YaHei 规则不变。

### 为什么这样修改

用户真正要测试的是“未来波形分析后生成音量控制”，而不是传统 compressor detector 的追随速度。future-window peak 有两个实验优势：

1. 稳定正弦只要窗口足够覆盖代表性的波峰，peak level 会比非整周期 rolling RMS 稳定得多，可直接验证谐波是否随 Lookahead 增大而下降；
2. 控制信号拥有未来信息，电平事件不需要在事件发生后再通过 20 ms 历史窗口追赶，因此可以与传统 causal Attack 行为区分。

选择可输入 0–100 ms 而不是锁死 5 ms，是因为不同频率的周期长度不同，5 ms 对 500 Hz 以上可能足够，但对 50/100 Hz 未必足够。用户需要用 PluginDoctor 自己找到可接受的折中。

### 验证

- [x] 基于 0.1.3 源码和交接文档完成最小范围修改。
- [x] 模型侧数学/算法仿真（非 VST3 实测）：future-window peak 在窗口覆盖足够周期后，对稳定正弦的 gain ripple 明显低于旧 moving RMS；该结果仅作为实现依据，不得写成插件已验证。
- [x] UTF-8 全工程文本解码检查通过。
- [ ] 当前 AI 环境真实 JUCE 编译。
- [ ] Codex/Windows Release 编译。
- [ ] VST3 扫描和加载。
- [ ] PluginDoctor Dynamics 实测。
- [ ] PluginDoctor Harmonic Analysis 实测。
- [ ] Cubase PDC/Bypass/Lookahead 改值实测。
- [ ] 用户确认。

### 已知问题 / 未解决问题

- 0 ms 预期会接近瞬时 waveshaping，仅保留做基准。
- future-window peak 的最小无明显谐波窗口与信号最低有效频率相关；尚无用户实测结论。
- peak-based level 与旧 RMS-based level 的绝对压缩量会不同，即使 Ratio law 相同；需要主观试听。
- 改 Lookahead 会改变真实插件 latency；宿主实时 PDC 更新行为需实测。
- future-window peak 没有传统 Attack/Release，但其 gain 仍可在未来窗口最大值改变时发生控制变化；是否完全符合用户“自动化式”听感必须由 PluginDoctor + 听感共同确认。
- Strict LUFS Match 尚未实现，已排入下一阶段，但本版按用户要求不动。

### 回滚

当前没有用户确认的 Stable 版本。

如果 0.1.4 编译或架构失败，可用 0.1.3 Candidate 恢复 UI/工作流并作对照，但 0.1.3 的 20 ms rolling RMS detector 已有用户 PluginDoctor 失败证据，不能当作最终核心。

绝对不要恢复 0.1.0 `abs(x)^(1/Ratio)` sample-domain waveshaper。

### 后续建议

先让 Codex 编译，不做额外 DSP 优化。成功后优先跑 PluginDoctor 多频率/多 Lookahead 矩阵，再决定 future-window peak 是否值得继续。确认核心后，才把 Match 改成用户要求的严格 LUFS。


---

## v0.1.5 — Fixed Lookahead Presets / Last-Choice Memory

**日期：** 2026-08-25  
**状态：** Candidate / Test  
**基于：** v0.1.4

### 用户需求

用户完成 0.1.4 PluginDoctor 对比后，不再需要任意毫秒 Text 输入。最终要求 Lookahead 固定为 `0 / 10 / 26 / 40 / 80 / 100 ms` 六档下拉菜单，并记住用户最后一次选择，使下一次新开插件实例默认仍为该档位。已有工程/已有实例必须优先恢复工程里自己的 Lookahead。

### 0.1.4 后续实际验证补记

0.1.4 已由用户实际构建并运行 PluginDoctor Harmonic Analysis。用户报告：

- 5 ms 对约 99.6 Hz 以上影响陡降；
- 10 ms 对约 49.8 Hz 以上影响陡降；
- 20 ms 对约 24.9 Hz 以上影响陡降；
- 26 ms 是约 20 Hz 的分水岭；
- 40 ms 对 20 Hz 的谐波比 26 ms 显著更低；
- 80 ms 又明显更干净；
- 继续增加 Lookahead 仍会降低影响；
- 0 ms 虽有可测谐波，但用户 A/B 认为听感细微并要求正式保留为一种口味。

这些是**用户实际 PluginDoctor 结果**，不是模型推测。

### 根因 / 设计收敛

0.1.4 的任意毫秒输入完成了研究任务。继续保留自由 Text 输入会增加参数选择噪声和错误输入空间，因此现在根据实测结果收敛为有意义的六档。future-window peak 核心本身本次不改。

### 修改内容

- `LOOKAHEAD (ms)` TextEditor 改为 ComboBox，选项严格为 0/10/26/40/80/100 ms。
- 保留 `lookaheadMs` 原参数 ID 与 float 状态格式，降低 0.1.4 Candidate 工程迁移风险。
- DSP、PDC、UI 将 Lookahead snap 到最近合法档位；旧 0.1.4 任意数值在状态恢复时迁移到最近档位；若距离完全相同则选择更长 Lookahead，避免 5 ms 旧值被降到特殊的 0 ms 失真档。
- 新实例从 per-user `QQSuperCompression.settings` 读取 `lastLookaheadMs`。
- 用户手动改变 ComboBox 时立即保存 `lastLookaheadMs`。
- 已有工程载入时 APVTS 工程状态覆盖新实例默认，因此不会因为用户后来改变全局默认而改写旧工程的 Lookahead。
- 首次无历史偏好 fallback 为 26 ms。
- Bypass 与 Active 继续使用完全相同的 selected Lookahead latency/PDC。

### 保持不变

- 0.1.4 future-window monotonic peak detector 不改。
- Ratio law 不改。
- ST/MS/LR、Makeup、Mix、A/B、Meters、Dynamic Display、窗口尺寸记忆、Undo/Redo、UTF-8 规则不改。
- Match 仍为旧 RMS/power prototype。用户已明确要求最终改为严格 LUFS，但尚未要求在本次 Lookahead 收敛中一起修改，因此本版不得宣称 LUFS Match 已完成。

### 验证

- [x] 0.1.4 future-window 核心已经有用户实际 PluginDoctor 验证。
- [x] 0.1.5 源码静态 preset/snap/persistence 检查。
- [x] UTF-8 文本检查。
- [ ] 当前 AI 环境真实 JUCE/VST3 编译。
- [ ] Codex Windows Release 编译。
- [ ] Cubase 六档 UI、工程恢复、新实例默认、PDC/Bypass、A/B 实测。
- [ ] 用户确认 0.1.5。

### 已知问题 / 未解决问题

- 0 ms 是用户明确保留的有失真口味，不应误写为无失真档。
- `lookaheadMs` 为兼容 0.1.4 仍保留 float 参数类型；正常 UI 只会写六个合法值。外部宿主若直接自动化任意中间值，DSP 会 snap 到最近 preset；这一宿主自动化边界需后续实测。
- Strict LUFS Match 仍待后续单独实现。

### 回滚

当前仍无用户明确指定 Stable。若 0.1.5 失败，优先回滚 0.1.4 Candidate 的可编辑 Lookahead UI；保留 0.1.4 的 future-window peak 和用户 PluginDoctor 实测结论。不得回到 0.1.3 rolling RMS 或 0.1.0 waveshaper。


---

## v0.1.6 — Strict Integrated LUFS Match

**日期：** 2026-08-25  
**状态：** Candidate / Test  
**基于：** v0.1.5

### 用户需求

用户明确要求把现有 RMS/power Match 改为严格 LUFS Match，不接受 RMS 作为最终实现。

### 问题表现 / 根因

0.1.3–0.1.5 的 Match 只是累计未经 K-weighting 的 Dry/Wet sample power，再把 power ratio 转成 dB；虽然可做能量等效补偿，但它不是 BS.1770 / EBU R128 Integrated Loudness。缺少 K-weighting、400 ms gating blocks、-70 LUFS absolute gate 和 -10 LU relative gate。

### 修改内容

- 新增 `Source/BS1770LoudnessMatch.h`。
- 使用 per-sample-rate BS.1770-compatible K-weighting，两级为 head-related high shelf + RLB high-pass；48 kHz 系数与 BS.1770 表格值一致。
- Integrated measurement 使用 400 ms block、100 ms hop（75% overlap）。
- 第一阶段 absolute gate = -70 LUFS；第二阶段 relative gate = absolute-gated loudness - 10 LU。
- ST：Dry L/R 与 linked Wet L/R 作为 stereo programme 测量，写一个共同 Makeup。
- LR：L、R 分别按 mono BS.1770 gated Integrated LUFS 测量，分别写 L/R Makeup。
- MS：M、S 分别按 mono BS.1770 gated Integrated LUFS 数学测量，分别写 M/S Makeup；M/S 是处理域分量，不是标准扬声器 layout，此边界必须说明。
- Match source 保持 Dry vs compressed Wet pre-Makeup/pre-Mix，因此现有 Makeup/Mix 不污染统计。
- 完全静音或没有通过 gate 的 LR/MS 分量不强制写 0 dB，而是保持原 Makeup。
- 0.1.5 future-window peak、Lookahead 六档、PDC/Bypass、Ratio、A/B、UI、Meter、Mix、ST/MS/LR topology 均保持不变。

### 为什么这样改

用户要的是严格感知响度匹配而非 raw energy matching。BS.1770 / EBU R128 Integrated Loudness 是明确的 K-weighted、双门限 gated programme loudness 定义。

### 验证

- [x] 新 LUFS core 独立 C++ 编译运行。
- [x] 固定差值测试：Dry=-20 dBFS sine、Wet=-26 dBFS sine，ST/L/M Match ≈ +6.00 dB。
- [x] 48 kHz full-scale 1 kHz mono sine ≈ -3.004 LUFS，与 BS.1770 -3.01 LKFS reference point 一致。
- [x] Cross-check against `pyloudnorm` DeMan/BS.1770 reference implementation on a 30 s gated 997 Hz test at 44.1 / 48 / 96 kHz: results matched to displayed precision (about -23.0731 / -23.0759 / -23.0932 LUFS).
- [x] UTF-8/source static check。
- [ ] 当前 AI 环境完整 JUCE/VST3 编译。
- [ ] Codex Windows build。
- [ ] Cubase Match 实测。
- [ ] 与独立可信 BS.1770/EBU R128 meter 对照。
- [ ] 用户确认。

### 已知问题 / 风险

- 至少要有一个完整 400 ms gating block 才可能得到有效 Match。
- 现有 transport reset 逻辑仍需在 Cubase 验证 Stop→Play / seek / loop。
- LUFS block storage 预留约 4 小时，普通音乐工作期间不会 audio-thread realloc；极端连续测量超过预留后 vector 可能在 100 ms block boundary 扩容。
- M/S 的每个分量使用严格 mono BS.1770 math，但 M/S 本身不是 BS.1770 标准 speaker layout。

### 回滚

当前仍没有用户明确指定 Stable。若 0.1.6 Match 有问题，回到 0.1.5 Candidate，只重做 Match；不得回到 0.1.3 rolling RMS 核心或 0.1.0 waveshaper。


---

## v0.1.7 — Auto GR Peak Hold / Version Tag / playHead Warning Cleanup

**日期：** 2026-08-25  
**状态：** Candidate / Test  
**基于：** v0.1.6

### 用户需求

用户确认 0.1.6 LUFS Match 没问题，并报告上一个编译存在非致命 `playHead` 名称遮蔽 warning。用户要求面板从下一版开始低调显示当前版本号；同时加入类似 DB-5035 的 Gain Reduction Hold，但改成保持约 2 秒后自动刷新，不需要点击手动清零。

### 修改内容

- Audio thread 基于每个 block 已捕获的 GR peak 维护两路 2 s Hold，避免 GUI 30 Hz 刷新漏掉短峰。
- 新更深 GR 立即成为 Hold 并重启 2 s；到期后自动刷新到当前 block GR。
- ST linked Hold 一致；LR L/R 独立；MS M/S 独立。Mode / Lookahead 改变会清除旧 Hold。
- GR meter 增加白色 Hold 横线与低调 `H x.x` 数值，当前 GR bar 不冻结。
- `playHead` 局部变量改名为 `hostPlayHead`。
- 面板版本标签读取 `JucePlugin_VersionString`；CMake version = 0.1.7。

### 保持不变

0.1.6 strict LUFS Match、future-window peak DSP、Ratio、Lookahead/PDC、A/B、Makeup、Mix、ST/MS/LR、Dynamic Display 全部不改。Hold 是纯显示状态，不进入参数/工程。

### 验证

- [x] 源码静态数据流检查。
- [x] `playHead` 局部命名检索清理。
- [x] UTF-8 检查。
- [ ] 完整 JUCE/VST3 编译。
- [ ] Cubase GR Hold/版本号实测。
- [ ] 用户确认。

### 已知边界

Hold 计时按 audio callback samples 推进；宿主停止并停止 callback 时不会按墙钟继续倒计时。计时粒度为 audio block，最多约一个 block 误差。

### 回滚

若 0.1.7 Hold/UI 失败，回到 0.1.6 Candidate，只重做 Meter/UI。0.1.6 LUFS Match 已获用户实际确认，不应回退。


---

## v0.1.8 — GR Hold Readability / Uniform 1:1 UI Scaling

**日期：** 2026-08-25  
**状态：** Candidate / Test  
**基于：** v0.1.7

### 用户需求

用户看到 0.1.7 实际面板后指出 GR Hold 的 `H x.x` 数值太小；由于 Hold marker/动态已经足够表达语义，要求去掉 `H`。用户随后补充：当前整体 UI 在缩放时不是 1:1 等比放大，要求整个 UI 真正按同一比例缩放。

### 根因

- Hold 第二行在窄 Meter 中使用 7.5 px 字体并额外占用 `H ` 字符。
- 旧 Editor 允许 width/height 独立 resize，且 `resized()` 只重排 Rectangle；字体、线宽和控件本身没有作为一个整体同比例 scale，所以会出现横向/纵向非比例变形。

### 修改内容

- Hold 数字删除 `H` prefix，font 7.5→8.5；Hold marker 与 2 s 算法不改。
- 增加固定 1020x670 `contentRoot`，所有 UI widget 成为其 child。
- Editor 只对 root 应用一个 X=Y 的 `AffineTransform::scale(uiScale)`。
- Bounds constrainer 固定 1020/670 aspect ratio；min/max size 也按该 aspect 计算。
- 旧非比例 editor width/height 在打开时迁移到“能放进旧矩形的最大 uniform scale”，以后只保存比例尺寸。
- CMake version = 0.1.8；APVTS/工程状态不变。

### 保持不变

0.1.7 Hold timing/data capture、0.1.6 strict LUFS Match、future-window peak、Ratio、Lookahead/PDC、ST/MS/LR、A/B、Makeup、Mix、Undo/Redo、音频 DSP 全部不改。

### 验证

- [x] Hold readout 源码检查：不再生成 `H `。
- [x] 单一 design-root/uniform-transform 源码检查。
- [x] aspect/min/max/saved-size migration 数学检查。
- [x] UTF-8 检查。
- [ ] 完整 JUCE/VST3 compile。
- [ ] Cubase resize/hit-test/reopen 实测。
- [ ] 用户确认。

### 回滚

仍无用户指定 Stable。0.1.8 UI scaling 若失败，回到 0.1.7 Candidate，只重做 scaling；不得回退 LUFS Match/future-window peak。

---

## v0.1.8 - Open-source release / Plan D build infrastructure

**Date:** 2026-08-26  
**Status:** Candidate / Test  
**Based on:** v0.1.8 validated Windows source

### Changes

- Added the MIT License and public-repository metadata.
- Made AU an Apple-only CMake format while retaining VST3 on Windows and macOS.
- Added GitHub Actions jobs for Windows x64 VST3, macOS Apple Silicon VST3, macOS Intel VST3 and macOS Universal 2 AU.
- Added bilingual installation guides and the Plan D release checklist.
- No audio DSP, parameter ID, default value, project-state or UI behaviour was changed.

### Validation discipline

- The pre-release Windows 0.1.8 build, LUFS self-test and VST3 factory-load smoke test passed before these packaging-only changes.
- Rebuild Windows after the CMake change and record GitHub Actions results before calling Plan D complete.
- The product remains Candidate / Test until the user explicitly confirms Stable.
