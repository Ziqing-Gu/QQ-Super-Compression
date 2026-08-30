# AI 持续开发交接规范

## v1.0.2 — Complete Relative LINK — Stable baseline

- 用户于 2026-08-30 明确要求把此版本设为稳定基线并执行 Plan B/C/D。
- LR/MS 的 LINK 完整覆盖 Ratio、Threshold、Makeup、Mix；保留相对差值，不强制相等。
- drag、Shift-fine、直接数值输入共享同一 delta/boundary 规则；任一侧到边界时双方共同停止。
- LINK 保存为工作流状态但不进入 A/B 声音快照。
- DSP、参数 ID、状态结构、Lookahead、Threshold 与 Display 绘图规则保持 1.0.1 稳定基线不变。
- 回滚顺序：先回滚到本次 Plan B 的 1.0.2 快照；若问题来自 LINK 交互，则对照 1.0.1 Display 稳定基线，仅重做编辑器联动。

English: 1.0.2 completes offset-preserving Relative LINK for Ratio, Threshold, Makeup, and Mix across drag, Shift-fine, and direct numeric entry. Both sides stop together at parameter boundaries. This is an editor-interaction change only; DSP and state compatibility remain on the 1.0.1 stable foundation.

## 当前稳定基线 / Current stable baseline — 1.0.2 Complete Relative LINK

**日期 / Date:** 2026-08-30
**状态 / Status:** Stable baseline under the user's standing Plan D rule
**回滚基线 / Rollback:** 1.0.1 Display 0…-90 dB Scale Polish Plan B snapshot

- 当前 DSP 是 future-window peak / Lookahead 核心。未经用户明确同意，不得恢复已拒绝的 1.0.0 Direct/Analytic/Hilbert 路线。
- Threshold OFF 必须精确保留旧 QQ law；有限 Threshold 只是连续作用下限，不得偷偷改 detector、Attack、Release、knee 或 smoothing。
- ST 使用共同控制；LR/MS 具有独立 Ratio、Threshold、Makeup 与 Mix。Relative LINK 完整覆盖四组控制，在拖动、Shift 精调和直接数值输入中保留差值，并在边界共同停止。
- Dynamic Display 固定绘图范围为 0…-90 dB、15 dB 间隔；不得因此改变 DSP、Meter、参数、LUFS 或 -120 dB Threshold OFF sentinel。
- 1.0.2 Plan A 安装包（本机 JUCE 8.0.14）与最终仓库重建包（JUCE 8.0.15）的项目自测和 Steinberg validator 均通过；最终 8.0.15 包未在本次 B/C/D 中重复覆盖系统安装。Plan D 四类跨平台成品必须来自同一公开提交。
- Cubase 最终听感、界面、PDC、Mix/Bypass、自动化与旧工程迁移仍由用户复核。

English summary: keep the future-window peak/Lookahead core, exact Threshold OFF legacy law, independent LR/MS controls, offset-preserving Relative LINK, and drawing-only 0…-90 dB Display rule. Do not revive rejected detector or Direct/Analytic/Hilbert experiments without explicit user approval.

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
当前稳定版本：1.0.2 — Complete Relative LINK（用户明确指定；Plan D 稳定基线）
当前候选版本：无（1.0.2 正在完成本次 Plan D 跨平台交付）
主要平台：Windows / macOS
插件/程序格式：VST3 / AU
主要开发环境：JUCE 8 / CMake / C++17
主要测试环境：Windows + Cubase / PluginDoctor（由用户/Codex 实际构建与验证）

核心产品动机：
- QQ Super Compression 不是为了泛泛地“重新发明压缩器”。
- 用户设计它的主要原因是：传统压缩器的 Attack / Release 行为会重塑声音的瞬态/音头。
- 吉他拨弦、人声字头/辅音、钢琴锤击、贝斯拨弦/指弹等素材有时需要动态收敛，但用户并不希望压缩器同时改变原始起音和听感。
- 因此本插件的核心目标是：在需要动态压缩时，尽量避免传统 Attack / Release 包络对瞬态/音头的重塑。
- Future-window Lookahead 的意义是提前知道未来电平，使处理不依赖传统 Attack 去“追”已经发生的瞬态。
- 详细说明见 PRODUCT_DESIGN_NOTES.md；以后 Codex/AI 写 GitHub/Release 介绍前必须先读。

当前已完成：
- 0.1.3 UI/工作流：Meter 面板瘦身、窗口尺寸记忆、A/B、A→B/B→A、Shift 微调、Alt 默认、Undo/Redo、按钮式 Bypass、循环 Mode、LR/MS 独立 Makeup。
- Dynamic Display：Dry/Input、Wet pre-Makeup、最终 post-Mix Output。
- ST / MS / LR 三模式；Input / Output / Gain Reduction 双通道 Meter；MS 显示 M/S；GR 从顶部向下。
- 0.1.4 future-window sliding peak 核心替代 rolling RMS；Lookahead 同时承担未来分析窗口与真实延迟/PDC。
- 0.1.5 Lookahead 固定为 0 / 10 / 26 / 40 / 80 / 100 ms；工程保存自身值，新实例使用用户上次手动选择，首次 fallback=26 ms。
- 0.1.6 Strict BS.1770 / EBU R128 Integrated LUFS Match：K-weighting、400 ms block、75% overlap、-70 LUFS absolute gate、-10 LU relative gate；用户后续明确确认 LUFS Match 没问题。
- 0.1.7 自动 2 秒 GR Peak Hold；面板低调版本号；清理 playHead shadow warning。
- 0.1.8 GR Hold 可读性微调；Editor 采用固定 1020x670 design root + uniform scale + 固定比例；用户把版本分享给朋友使用后反馈整体反响不错。
- 0.1.9 首次引入通用 1x/2x/4x/8x FIR Oversampling、combined PDC，并清理 constrainer shadow warning，作为实验版本。
- 用户后续 PluginDoctor/宿主实测得出 Oversampling 最终产品结论：10 ms 及以上基本无需要处理的混叠；2x/4x 在 0 ms 下混叠仍严重；Oversampling 增加的 latency 很小。
- 0.1.10 因此收敛为：仅 0 ms 显示/启用 Oversampling，按钮单击循环 1x -> 8x -> 16x -> 1x，默认记忆值 8x；10/26/40/80/100 ms 内部固定 1x 并隐藏按钮。
- 0.1.10 保留用户最后的 0 ms OS 选择：切离 0 ms 不清除，切回恢复；工程/A-B/Undo 同步保存。
- 0.1.10 加入 16x FIR 路径；1x 为 raw colour，8x 默认平衡，16x 进一步减少 aliasing。Oversampling 目的只是在 0 ms 减少 alias fold-back，不消灭用户主动保留的非线性染色。
- 0.1.10 新增 PRODUCT_DESIGN_NOTES.md 与 OVERSAMPLING_DESIGN_NOTES.md，专门让未来 Codex/AI 理解产品理由、用户实测结论与不能随意回退的设计。
- 0.9.0 在不改 DSP 的前提下进入正式发布前 UI 阶段：浅暖 ivory/sand 背景、暖橙主 accent、cyan technical accent、柔和 vector glow；Dynamic Display 结构不改；新增 UI_DESIGN_NOTES.md。
- 0.9.2 加入 Input Gain / Output Gain 与完整 A/B/state/Undo/Display 语义；同版 bitmap filmstrip knob 视觉路线后被用户实机否决。
- 0.9.3 恢复 v0.9.1 vector UI 绘制，保留 v0.9.2 两只 Gain 和全部功能逻辑；bitmap 资产不再参与 build/runtime。
- 0.9.4 补齐浅色主题下 Slider 双击直接输入的 Label/TextEditor 编辑态配色，解决白字不可见；仅改 UI LookAndFeel，不改 DSP/参数/布局。
- 1.0.1 恢复透明 future-window/Threshold 核心，完成独立 LR/MS 分域、1020×820 Display-first 布局与 0…-90 dB 绘图范围，并按 Plan D 成为稳定基线。
- 1.0.2 在不改 DSP、参数 ID 或 state schema 的前提下，补全 Ratio、Threshold、Makeup、Mix 的 Relative LINK、直接输入与共同边界规则。
- Ratio law 仍为 gain = 1 / (1 + (Ratio - 1) * level)。
- UTF-8/CJK 跨平台规则继续保留。

当前已知问题：
- 0.1.0 sample-domain abs(x)^(1/Ratio) waveshaper 已被用户否决，禁止恢复。
- 0.1.1–0.1.3 fixed 20 ms rolling RMS 被用户 PluginDoctor 证明具有不希望的 Attack/Release-like response 与谐波，禁止作为最终核心恢复。
- Lookahead=0 ms 本质上是 instantaneous level、会产生明显谐波/染色；用户明确要求保留为 flavour。不要以“修 bug”为名偷偷加 smoothing/隐藏 Lookahead。
- 用户实际 PluginDoctor 结果：5 ms≈99.6 Hz、10 ms≈49.8 Hz、20 ms≈24.9 Hz；26 ms 把分水岭推至约 20 Hz，40/80 ms 对 20 Hz 继续更干净。
- 用户实际 Oversampling 结果：10 ms+ 无明显 aliasing 需求；0 ms 的 2x/4x aliasing 仍严重；OS latency 增加不多，因此最终不提供 2x/4x，直接保留 1x/8x/16x。
- PluginDoctor 0 ms Ratio>1 LinearAnalysis 曾出现 ratio-dependent 高频上翘；Ratio=1:1 控制测试显示 8x FIR 本身基本平坦，仅 Nyquist 附近正常 roll-off。用户后续确认不需要把该现象当成 Oversampling FIR 故障；不要擅自加补偿 EQ。
- 运行中改变 Lookahead 或 0 ms OS factor 会改变真实 plugin latency，宿主可能做一次 PDC realignment；当前 Candidate 不加入用户未要求的 realtime crossfade。
- 1.0.2 的 JUCE/VST3 完整编译、项目自测与 Steinberg validator 已通过；CPU、PDC、Dry/Wet/Bypass 对齐、A/B/Undo/工程恢复仍需 Cubase 用户侧复核。
- Ctrl/Cmd+Z 是否被宿主优先截获仍需实际 VST3/Cubase 键盘焦点验证。
- 1.0.2 已由用户明确指定为 Stable；Cubase 中的 LINK 拖动、Shift 精调、直接输入、自动化、工程恢复与声音/PDC 仍建议由用户复核。

当前正在开发：
- 无（1.0.2 为当前稳定基线；当前只执行发布与跨平台验证）。

当前回滚基线：
- 1.0.2 Stable；若后续回归，优先回滚到本次 Plan B 的 1.0.2 快照。若问题来自 LINK 交互，则对照 1.0.1 Display Stable，只重做 Editor 联动，不回退 DSP。
- 若 0.1.10 的 16x / OS UI / state migration 出现问题，优先回滚到 0.1.9 Candidate 只重做 Oversampling；不要回退 0.1.8 UI、0.1.7 Hold、0.1.6 LUFS Match 或 future-window peak。
- 若通用 0.1.9 Oversampling 架构本身存在根本 PDC/API 问题，可回到用户上传的 0.1.8 Plan B Candidate，只重新实现 0 ms-only Oversampling。
- 永远不要恢复 0.1.0 waveshaper 或 0.1.1–0.1.3 rolling RMS 作为最终核心。

下一步建议：
- 后续开发前先完整读取 PRODUCT_DESIGN_NOTES.md、OVERSAMPLING_DESIGN_NOTES.md、UI_DESIGN_NOTES.md 与本文件，再从 1.0.2 Stable 开始。
- 确认 build 不再创建/链接 `QQSCAssets`，旋钮使用恢复后的 v0.9.1 vector LookAndFeel。
- 在 LR/MS 分别验证 Ratio / Threshold / Makeup / Mix 的 LINK：拖动、Shift 精调、直接数值输入与边界停止必须保持相对差值；同时复核编辑文字、caret 与 selection。
- 重点验证 Input/Output Gain 仍完整存在：Input 改变 detector/compression 与 Input meter，但不移动 Dynamic Display Dry/Input；Output 改变最终 Output meter 与 Display Output。
- 验证 A/B、Undo/Redo、工程保存/旧 state migration 仍包含两只 Gain。
- 确认 JUCE 8.0.15 的 8x(3 stage)/16x(4 stage) FIR API、PDC/Bypass、LUFS Match、GR Hold 与 ST/MS/LR 均无回归。
- PluginDoctor：0 ms 对比 1x/8x/16x aliasing；不要把谐波仍存在误判为 Oversampling 失败。
- Cubase：0 ms 三档 PDC、Mix 50%、Bypass；10 ms+ 确认 OS 隐藏且 effective 1x/Lookahead-only PDC。
- v1.0.2 已由用户明确指定并按 Plan D 规则记录为 Stable。公开 GitHub Release 仍停留在 0.9.4；更新 Release 属于单独的 Plan G。
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

## v0.1.9 — FIR Oversampling / PDC Alignment / Warning Cleanup

**日期：** 2026-08-27  
**状态：** Candidate / Test  
**基于：** v0.1.8 Plan B（用户上传 `QQ Super Compression 0.1.8-PlanB-20260825-161114.zip`）

### 用户需求

用户反馈当前版本已分享给朋友使用，整体反响不错，认为主要功能已接近完整形态。除 0.1.8 编译出现的非致命 `constrainer` 名称遮蔽 warning 外，用户要求加入最高 8x Oversampling，主要给 0 ms / 10 ms Lookahead 的主动染色模式使用：允许用户保留该声音特征，同时通过 2x/4x/8x 选择减少 alias fold-back。

### 问题表现 / 根因

- 0/10 ms 的 future-window gain modulation 本身会生成谐波；在 host sample rate 直接运行时，高次内容可能折返到 Nyquist 以下形成 aliasing。这里的“谐波生成”属于用户要保留的声音行为，而“折返 aliasing”才是 Oversampling 希望减轻的部分。
- 0.1.8 Editor 中局部变量 `constrainer` 与 JUCE/类上下文命名触发编译器 shadow warning，虽不影响运行但应清理。
- 加入 FIR Oversampling 后 Wet 会获得额外滤波 latency；若仍只按 Lookahead 延迟 Dry/Bypass，Mix 会产生时间偏移/梳状，因此 PDC 必须升级为 combined latency。

### 修改内容

- 新增 APVTS Choice 参数 `oversampling`，固定 `1x / 2x / 4x / 8x`，默认 1x；参数追加在原 0.1.8 参数序列之后，尽量不改变既有 host-facing 参数顺序。
- 预创建 4 套 `juce::dsp::Oversampling<float>`：1x dummy；2x/4x/8x = maximum-quality `filterHalfBandFIREquiripple` + `useIntegerLatency=true`。CMake 新增 `juce::juce_dsp`。
- Detector、future-window monotonic peak、Ratio smoother 和 Ratio gain application 在选定 oversampled domain 运行。Lookahead 继续先按 host sample rate 取原有 rounded sample 数，再乘 factor，保持六档实际时间/PDC 语义。
- 为避免破坏 0.1.6 起 Match 同时累积 ST/LR/MS 的行为，Oversampling 内部一次生成/下采样 6 路 pre-Makeup Wet：ST linked L/R、LR independent L/R、MS M/S。严格 LUFS accumulation 仍在 host rate 使用这六路。
- Makeup、Mix、最终 output Meter/Display 仍在 host sample rate。
- total latency = Lookahead base samples + JUCE FIR integer latency；独立 host-rate Dry delay 用相同 total latency，Active Mix Dry 与 Bypass 都走该路径。
- Oversampling 进入工程状态、A/B Snapshot、A→B/B→A 和 Undo/Redo；没有 per-user last selection，新实例默认 1x，0.1.8 或更早 state 缺参时也显式迁移到 1x。Editor Timer 对 Undo/Redo/host-side choice 同步时补一次 latency notification。
- `OVERSAMPLING` ComboBox 放在 Lookahead 下方，保持既有固定 1020x670 design root 和 uniform scaling。
- 局部 `constrainer` 改名 `editorBoundsConstrainer`；既有 `hostPlayHead` 命名继续保留。
- CMake project version 更新到 0.1.9，面板 `JucePlugin_VersionString` 因此自动显示低调 `v0.1.9`。

### 保持不变

- Ratio law 仍为 `gain = 1 / (1 + (Ratio - 1) * level)`。
- Lookahead 仍只有 0 / 10 / 26 / 40 / 80 / 100 ms；0 ms 染色语义不取消、不偷偷加隐藏 smoothing。
- 0.1.6 strict BS.1770 / EBU R128 Integrated LUFS Match 定义不改：Dry vs compressed Wet pre-Makeup/pre-Mix。
- ST/MS/LR、Makeup、Mix、2 s GR Hold、Dynamic Display、A/B 其它字段、UTF-8/CJK 和插件产品/internal identity 不改。
- 没有加入自动 Oversampling、没有按 Lookahead 强制某个 factor。

### 为什么这样修改

FIR linear-phase 路径优先服务于本插件已有 Dry/Wet Mix 的时间/相位可控性；integer latency 让 host PDC 与 base-rate Dry delay 能共享明确的整数 sample count。1x 使用 dummy stage，使默认状态尽量贴近 0.1.8，而不是强制所有用户承担 CPU/latency。保留六路 Wet 的代价是 8x CPU 更高，但避免为了性能而暗改 Match 的“所有域同时测量”语义。

### 验证

- [x] 源码静态数据流检查：OS factor → internal Lookahead → 6-Wet downsample → host-rate Match/Makeup/Mix → combined Dry/Bypass delay 已连接。
- [x] CMake version = 0.1.9，新增 `juce::juce_dsp` link；UI version 仍从 JUCE metadata 读取。
- [x] 源码检索：未重新引入局部 `playHead`；原 `constrainer` 局部变量已改为 `editorBoundsConstrainer`。
- [x] JUCE-free `tests/bs1770_match_selftest.cpp` 可继续作为 LUFS 回归基准（实际运行结果另见本版静态检查报告）。
- [ ] 当前 AI 环境完整 JUCE 8.0.15 / VST3 编译。
- [ ] Windows/Codex Release compile + warning 验证。
- [ ] Cubase PDC / Mix / Bypass / project/A-B/Undo 实测。
- [ ] PluginDoctor 0/10 ms 1x/2x/4x/8x aliasing 对比。
- [ ] 用户确认 0.1.9。

### 已知问题 / 边界

- 当前实现为了保持三个 Match domain 同时可用，2x/4x/8x 下会对 6 路 Wet 做 downsample，8x CPU 成本预计明显高于只处理当前模式；这是已知取舍，需要实际 profiling。
- 切换 Oversampling factor 会 reset FIR/detector/delay history，并改变 host latency；运行中可能出现一次 setting-change transient/PDC realignment。当前未加入用户未要求的双路径 crossfade。
- 当前预分配按至少 16384 host samples/block；正常 DAW block 远小于此值。异常大于 reserve 的 callback 会 fail-safe clear，而不是 realtime realloc；需实际宿主确认不会触发。
- 完整 JUCE API/编译尚未在当前环境验证，因此不能把静态检查写成“已编译”。

### 回滚

当前仍无用户指定 Stable。若 0.1.9 Oversampling/PDC 失败，优先回滚到用户上传的 **0.1.8 Plan B Candidate**，只重做 Oversampling / combined latency；不要回退 0.1.8 UI、0.1.7 Hold、0.1.6 strict LUFS Match 或 future-window peak 核心。

### 后续建议

Codex 首先完成 Windows Release 编译并看 warning；随后在 Cubase 测 combined latency/Mix/Bypass，再用 PluginDoctor 比 0/10 ms 的 1x→8x aliasing。只有用户明确确认后才能把版本提升为 Stable。

---

## v0.1.10 — 0 ms-Only 1x/8x/16x Oversampling / Product Design Documentation

**日期：** 2026-08-27  
**状态：** Candidate / Test  
**基于：** v0.1.9 Candidate

### 用户需求

用户完成 0.1.9 Oversampling 的 PluginDoctor/宿主比较后，决定不再把 Oversampling 作为所有 Lookahead 的通用品质选项：仅 0 ms 需要它。用户先确认 0 ms 默认 8x、10 ms 及以上固定 1x，随后希望 0 ms 提供 1x/8x/16x 三个选择，并使用单击按钮循环；其它 Lookahead 时按钮隐藏。

用户还明确要求源码仓库必须记录“为什么这样设计”，因为后续会让 Codex 基于 GitHub 源码撰写插件介绍，而 Codex 看不到本次聊天。用户进一步补充了插件最核心的产品动机：传统 Compressor Attack/Release 会改变吉他、人声、钢琴、贝斯等声音的瞬态/音头与听感；QQ Super Compression 是为“需要压缩动态、但不愿意同时改变瞬态”的场景设计。

### 用户实测 / 决策依据

- PluginDoctor/宿主实测：10 ms 与更长 Lookahead 基本没有需要 Oversampling 处理的明显 aliasing，因此固定 1x。
- PluginDoctor 实测：0 ms 下 2x、4x 的 aliasing 仍然严重，改善不足。
- 用户观察：8x Oversampling 增加的 latency 并不多，因此没有必要为了少量 latency 保留效果不足的 2x/4x；直接提供 8x 与 16x 更合理。
- 0 ms 即便 8x 仍保留明显染色；这是该 mode 的非线性 character，不等于 Oversampling 失败。16x 用于进一步降低 alias fold-back，而不是消灭 character。
- 0 ms Ratio>1 的 PluginDoctor LinearAnalysis 曾显示 ratio-dependent 高频上翘；Ratio=1:1 控制测试显示 8x FIR 自身基本平坦，只在 Nyquist 邻近出现正常 roll-off。用户随后确认该问题无需处理，不要为了图形强行补 EQ。

### 修改内容

- Oversampling choices 从 v0.1.9 `1x/2x/4x/8x` 收敛为 `1x/8x/16x`；默认 remembered 0 ms choice = 8x。
- JUCE Oversampling 预创建三条路径：1x dummy、8x=3 FIR stages、16x=4 FIR stages，均沿用 maximum-quality linear-phase half-band FIR + integer latency。
- `effectiveOversamplingChoiceIndex()` 强制所有非零 Lookahead 使用 1x；只有 0 ms 使用保存的 1x/8x/16x choice。
- 因此 10/26/40/80/100 ms 的 future-window detector 恢复/保持 host-rate 1x 运行，PDC 为 Lookahead-only；0 ms 8x/16x 的 PDC 为 FIR latency。
- UI 将 Oversampling ComboBox 改为 TextButton；0 ms 显示，点击 `1x -> 8x -> 16x -> 1x`；非零 Lookahead 隐藏 label/button。
- 切离 0 ms 时不改写 `oversampling` 参数；切回 0 ms 恢复之前的 choice。
- A/B、Undo/Redo、project state 继续保存 remembered 0 ms choice。
- 增加 state schema version；0.1.8 或更早无 OS 参数的 state -> 8x；0.1.9 legacy 1x -> 1x，2x/4x/8x -> 8x。
- CMake/面板版本更新为 0.1.10。
- 新增 `PRODUCT_DESIGN_NOTES.md`：记录插件存在理由、吉他/人声/钢琴/贝斯瞬态应用场景、Lookahead 思路、0 ms colour 与 Match 的定位。
- 新增 `OVERSAMPLING_DESIGN_NOTES.md`：记录为什么只 0 ms 有 OS、为什么没有 2x/4x、为什么默认 8x、为什么有 16x，以及测量中高频 LinearAnalysis 的解释边界。
- README/CODEX_BUILD/CHANGELOG/TEST_CHECKLIST/DEVELOPMENT_HISTORY/HANDOFF 同步更新，保证未来 Codex 不依赖聊天即可理解设计。

### 保持不变

- Ratio law 不变。
- 0/10/26/40/80/100 ms 六档 Lookahead 不变。
- 0 ms 的 deliberate nonlinear character 不取消，不加 hidden smoothing/Lookahead。
- 0.1.6 strict BS.1770 / EBU R128 Integrated LUFS Match 不变。
- ST/MS/LR、Makeup、Mix、A/B 其它语义、2 s GR Hold、Dynamic Display、Meter、uniform UI scaling、UTF-8/CJK、插件 identity 不变。

### 为什么这样修改

产品功能应针对实际存在的问题，而不是机械提供所有常见倍率。2x/4x 已经由用户实测证明无法充分解决 0 ms aliasing，而 10 ms+ 又无需 Oversampling；继续保留通用 1/2/4/8 菜单只会增加用户决策成本和 CPU/PDC 路径复杂度。1x/8x/16x 只在 0 ms 出现，直接表达三种实际有意义的选择：raw、default-cleaner、cleanest-colour。

### 验证

- [x] 用户 PluginDoctor/宿主实际测试形成上述产品决策（测试对象为 0.1.9 Oversampling 实验版）。
- [x] 0.1.10 源码静态数据流/状态/UI 检查。
- [x] CMake version / 文档 / UTF-8 静态检查。
- [x] JUCE-free BS.1770 self-test 回归（结果记录在 STATIC_CHECK_0.1.10.txt）。
- [ ] 当前 AI 环境完整 JUCE 8.0.15 / VST3 compile。
- [ ] Codex Windows Release compile。
- [ ] 0 ms 16x PluginDoctor aliasing / CPU 实测。
- [ ] Cubase combined PDC / Mix / Bypass / A-B / state migration 实测。
- [ ] 用户确认 0.1.10。

### 已知问题 / 边界

- 16x CPU 会高于 8x，尤其当前为保持所有 Match domain 同时累积，会 downsample 六路 Wet；需要实际 profiling。
- 切换 active 0 ms OS factor 或 Lookahead 会 reset FIR/detector/delay history并改变 PDC，可能有一次 setting-change transition/host realignment；未加入未经要求的 crossfade。
- 0 ms 仍会产生谐波/染色，即使 16x 也不应以“必须无谐波”作为验收标准。
- 当前仍无用户指定 Stable。

### 回滚

若 0.1.10 失败，优先回到 0.1.9 Candidate 只重做 0 ms-only OS；若 0.1.9 通用 OS 架构本身有问题，则回到用户上传的 0.1.8 Plan B Candidate 重做 OS。不得回退 strict LUFS Match、GR Hold、uniform UI 或 future-window peak。

### 后续建议

Codex 编译前必须读两个 Design Notes；如果未来写 GitHub 产品介绍，应首先说明“传统 Attack/Release 会改变音头，而本插件服务于需要动态控制但不希望改变瞬态的场景”，再解释 Lookahead 与 0 ms colour。不要把产品只写成普通的 threshold-free compressor。



---

## v0.9.0 — Warm Transparent UI Candidate

**日期：** 2026-08-27  
**状态：** Candidate / Test  
**基于：** v0.1.10

### 用户需求

在核心 DSP 获得用户与网友积极实际反馈后，进入正式发布前 UI 优化。用户拒绝深黑/重霓虹赛博朋克方案，因为它会让人联想到重度失真；最终确认浅暖、简洁、透明、柔和内发光方向。用户对现有 Dynamic Display 结构满意，只接受颜色/背景调整。Mode 明确采用方案 A：一个按钮单击循环三个模式，而不是下拉菜单。版本规划跳到 0.9.0，最终确认后再发布 1.0.0。

### 修改内容

- CMake/JUCE version -> 0.9.0。
- `UTF8LookAndFeel.h` 增加 warm ivory/sand palette、自定义 rotary knob、soft vector glow、浅色按钮和 ComboBox。
- Editor 主背景/面板改为浅暖色，保留 1020x670 uniform scaling。
- Dynamic Display 仅改 palette/background：Dry neutral、Wet cyan、Output coral/orange。
- Meter panel 同步浅色语义，GR Hold 算法不变。
- Mode 保持单按钮循环 `ST -> MS -> LR -> ST`，不增加 dropdown。
- 新增 `UI_DESIGN_NOTES.md` 记录视觉动机、被否决的暗色方向、Glow 实现和不可改 DSP 边界。

### 保持不变

Ratio / Lookahead / Oversampling / PDC / strict LUFS Match / A-B / Makeup-Mix / ST-MS-LR DSP / 2 s GR Hold / parameter IDs 全部不改。

### 验证

- [x] DSP 核心文件与 0.1.10 原包 byte-for-byte 对照未改。
- [x] 源码/UTF-8/文档静态检查。
- [ ] JUCE/VST3 完整编译。
- [ ] Cubase UI/resize/hit-test 实测。
- [ ] 用户视觉确认。

### 回滚

若 0.9.0 UI 失败，回到 0.1.10 的 UI 文件重新做视觉层；DSP 不回退。

### 后续建议

Codex 必须先读 `UI_DESIGN_NOTES.md` 再做后续 GitHub 文案或 UI 调整。用户最终确认后才发布 v1.0.0。


---

## v0.9.1 — Lighting & Material Refinement

**日期：** 2026-08-27  
**状态：** Candidate / Test  
**基于：** v0.9.0 — Warm Transparent UI Candidate

### 用户需求

用户实际编译并截图 v0.9.0，确认整体布局已经比较满意，但对“灯光效果没有出来”表示失望；此前用户曾特别询问如何实现亮灯光影，因此本轮明确只继续精修灯光/材质，不重做布局。

### 问题表现

真实 Cubase 截图中，Ratio / Makeup / Mix 的橙色 value arc 很清楚，但周边 halo、底部暖光反射和按钮 back-light 太弱，视觉上像橙色描边而不是灯。

### 根因

0.9.0 多层矢量 glow 在亮 ivory 背景上的透明度和扩散范围不足；同时旋钮实体本身缺少足够的 rim/highlight/reflected-light 层，因此“光源 -> 光晕 -> 面板受光 -> 实体反射”的完整链条没有建立。

### 修改内容

- 旋钮：四级 halo/bloom、crisp arc、endpoint hot core、底部大面积 warm spill、旋钮内反射、实体高光/阴影。
- 按钮：激活态三层 halo + bottom spill，仍沿用原 toggle / always-lit state。
- 面板：不改位置和尺寸，只增加极轻材质 gradient / rim / shadow。
- CMake/JUCE version -> 0.9.1。
- 用户实编译发现的非致命 MSVC C4459：仅重命名 `lookaheadMs` 冲突 argument/local；无行为变化。

### 保持不变

- Layout / Display / Meter geometry。
- Mode 单按钮 `ST -> MS -> LR -> ST`。
- Ratio / Lookahead / Oversampling / PDC / LUFS Match / A-B / Makeup / Mix / GR Hold。
- 参数 ID / state schema / DSP signal flow。

### 验证

- [x] 源码静态 diff：布局坐标未修改。
- [x] C4459 对应 namespace helper 冲突命名已删除。
- [ ] Codex Windows Release 编译。
- [ ] VST3 Cubase 实际光影截图。
- [ ] 用户确认。

### 已知问题

Glow 是视觉参数，必须以真实宿主截图继续微调。不要为了让 glow 显眼而把整个背景改暗。

### 回滚

若本轮光影不满意，回到 v0.9.0 UI；DSP 不回退。

### 后续建议

Codex 编译时首先确认 C4459 已消失且无新 warning，再截图检查旋钮底部暖光、endpoint lamp、Mode/A-B/Bypass/OS back-light 是否真正可见。


---

## v0.9.2 — Asset Knobs / Input & Output Gain

**日期：** 2026-08-27  
**状态：** Candidate  
**基于：** v0.9.1

### 用户需求

用户指出 0.9.0/0.9.1 的程序绘制 glow 无法达到概念图质感，要求真正引入 UI 图片资产。旋钮应使用 128 帧透明 PNG，对应 MIDI CC 0–127 的视觉状态，但数字、`.`、`:`、`%`、`dB` 等必须继续用字体实时显示并可直接输入。用户还要求之前讨论的 Input Gain / Output Gain 在这一轮一起加入。

硬规则：

- 128 帧只量化视觉，不量化参数/DSP。
- 指针在 0 和 127 都必须可见。
- 灯带从最小值左侧开始累计点亮，20% 只亮前约 20%，127/100% 亮完整有效弧。
- Input Gain 位于 detector/compression 之前，会改变压缩行为；但 Dynamic Display Dry/Input 继续显示 Input Gain 之前的原始参考。
- Output Gain 位于 Makeup + Mix 后，必须进入最终 Output Display。

### 架构变化

- 新增 `QQSCAssets` BinaryData target，将 PNG 直接嵌入插件。运行时不依赖外部资源路径。
- `UTF8LookAndFeel::drawRotarySlider()` 不再生成旋钮材质/Glow，而是 `round(normalizedValue * 127)` 后从 filmstrip 取对应帧。
- Slider、APVTS、SliderAttachment、Undo、文本编辑全部继续存在；图片只负责外观。
- 数值与单位不写入 PNG。未来更换旋钮资产时不应改参数精度或 DSP。
- Buttons/ComboBox 暂时继续 vector/JUCE；尚未获得单独 button sprite 设计确认，因此不能为了“一致”自行图片化。

### 新增 Input / Output Gain

当前 Candidate 参数范围均为 -24..+24 dB，默认 0 dB。新参数追加在完整旧参数序列末尾，避免插入旧参数之间。

Signal flow（Active）：

```text
Original Input
  ├─> untouched copy -> Display Dry/Input reference + true Bypass delay
  └─> Input Gain -> Oversampling/Detector/Compression -> Makeup -> Mix -> Output Gain -> Output
```

- Active Dry/Mix path使用 Input Gain 后的 Dry。
- LUFS Match 继续比较 Input Gain 后 Dry 与 Wet pre-Makeup，并且只写 Makeup；Output Gain 不参与 Match。
- Output meter / Display Output 使用 Output Gain 后最终信号。
- Input meter 当前显示真正进入压缩器的 Input-Gain 后信号。
- True Bypass 保留相同总 latency/PDC，但绕过 Input Gain、compression、Makeup、Mix、Output Gain，输出 untouched delayed input。

### State / A-B / Undo

- Input/Output Gain 进入 APVTS、A/B snapshots、Undo/Redo、project state。
- State schema 升至 3，但 Oversampling legacy 判定仍以 schema 2 为界，避免把 0.9.1/0.1.10 的 16x 错误当成旧 0.1.9 OS schema。
- pre-0.9.2 state 缺失 trims 时显式迁移为 0 dB。

### 保持不变

- Ratio law、future-window peak、Lookahead、0 ms Oversampling、PDC、LUFS Match、GR Hold 不重设计。
- Dynamic Display / Meter 仍实时绘制，不图片化。
- Mode 仍单按钮 ST -> MS -> LR -> ST。

### 验证

当前只完成源码/资产静态检查；当前 AI 环境没有 JUCE checkout，不能声称 VST3 已编译。必须由 Codex/用户实际验证 BinaryData 编译、UI 资产加载、文本输入、PDC/Bypass、新 Gain 参数及 Display 语义。

### 回滚

若本版失败，回滚到 v0.9.1 Candidate；不要回退已验证的核心 DSP 历史。


---

## v0.9.3 — UI Rollback / Features Retained

**日期：** 2026-08-27  
**状态：** Candidate / Test  
**基于：** v0.9.2

### 用户需求

退回上一版 v0.9.1 的 UI 设计/绘制风格，但保留 v0.9.2 功能。

### 问题表现 / 根因

v0.9.2 128-frame bitmap knob 在真实插件中的视觉被用户明确否决；多轮 AI 资产生成也无法稳定达到已认可概念。该问题属于 UI asset 路线失败，不属于 DSP 功能错误。

### 修改内容

- 恢复 v0.9.1 `UTF8LookAndFeel` vector rotary drawing。
- 删除 CMake 运行时 `QQSCAssets` BinaryData 依赖。
- 保留 v0.9.2 Input/Output Gain、Display/Meter 语义、A/B/state/Undo、PDC 等功能。
- 旧 PNG/asset architecture 文档继续留档，不再 active。

### 验证

- [x] 静态 source diff / hash 检查。
- [ ] Codex/JUCE/VST3 编译。
- [ ] Cubase 实测。
- [ ] 用户确认。

### 回滚

若 UI restore 编译失败，只回滚/修复 UI/CMake；不要回退 v0.9.2 的 Input/Output Gain 功能。


---

## v0.9.4 — Editable Numeric Text Contrast

**日期：** 2026-08-28  
**状态：** Stable baseline（Plan D policy）  
**基于：** v0.9.3 — UI Rollback / Features Retained

### 用户需求

用户认为当前版本已经基本完善，但发现一个小遗憾：双击旋钮数值进入直接输入状态后，正在编辑的数字显示成白色，在浅 ivory 背景上几乎看不见。

### 问题表现

Slider 非编辑状态的数值颜色正常；只有双击后进入 JUCE 临时文本编辑状态时，编辑文字变成白色。

### 根因

已从源码定位为 UI 状态配色遗漏：v0.9.3 已设置 `Slider::textBoxTextColourId`，它覆盖普通显示状态，但没有显式设置 `Label::textWhenEditingColourId` / TextEditor 编辑态颜色。浅色主题下 JUCE 编辑器默认文字颜色与背景对比不足。

### 修改内容

- `UTF8LookAndFeel.h` 显式设置 Label 编辑态文字/背景/outline。
- 同时设置 TextEditor 文字、背景、选区与 highlighted text，避免 JUCE 内部编辑器继承不适合浅色主题的默认色。
- Caret 使用现有 warm accent，保证输入光标可见。
- CMake 版本号更新为 0.9.4；README / CHANGELOG / CODEX_BUILD / UI_DESIGN_NOTES / TEST_CHECKLIST 同步记录。

### 保持不变

- 不改任何 Slider 参数范围、默认值、数值精度或直接输入行为。
- 不改布局、旋钮几何、Display、Meter。
- 不改 Ratio / Lookahead / Oversampling / PDC / Bypass / LUFS Match / ST-MS-LR / GR Hold。
- 不改 Input Gain / Output Gain 信号位置和 Display 语义。
- 不改 A/B、Undo/Redo、工程状态。

### 为什么这样修改

这是一个明确的浅色 UI 编辑态对比度 Bug，最小风险修复应只补齐 JUCE Label/TextEditor 的颜色状态，而不是重写数值输入组件或改变交互。

### 验证

- [x] 源码静态检查：新增编辑态颜色 ID，DSP/Processor/Parameters 未修改。
- [x] JUCE/VST3 实际编译、安装、BS.1770 自测与 SHA-256 核对（Plan A）。
- [ ] Cubase 双击数值输入实测。
- [ ] 用户最终确认。

### 已知问题

暂无新增 DSP 已知问题。编辑态实际 caret/selection 观感仍需宿主实机截图确认。

### 回滚

若 v0.9.4 出现意外 UI 编译/显示问题，回滚到 v0.9.3；不要回滚 v0.9.2 已加入的 Input/Output Gain 功能。
