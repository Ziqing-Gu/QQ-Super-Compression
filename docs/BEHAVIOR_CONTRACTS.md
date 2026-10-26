# 用户确认的行为约定

目的：让换聊天、换 AI 或后续重构时，产品行为仍有明确依据和可执行验证。本文件记录已确认约定；代码和旧测试不构成变更约定的理由。

## MODE-001：模式按钮的循环顺序

用户于 2026-10-14 明确要求：Comp 的 Mode 按钮按 **ST → LR → MS → ST** 循环，中文悬停说明使用相同顺序。

这是按钮的切换顺序。已有工程及自动化参数仍沿用原来的数值含义（ST=0、MS=1、LR=2），不得交换参数编号或误读已保存的模式。

## LINK-001：Limiter 的 Output LINK 不受压缩设置影响

用户于 2026-10-14 再次明确：阈值、Makeup 和 Output 的关联不可与其他参数相关，Ratio 为 1:1 时仍必须联动。

### 开关与方向

以下均为用户直接编辑控件时的 dB 变化；所有联动比例固定为 1:1，与声音是否发生压缩无关。

| 用户操作 | LINK 打开 | LINK 关闭 |
| --- | --- | --- |
| 单压 Threshold 增加 Δ dB | Output 减少 Δ dB | Output 不动 |
| 双压 DOWN 增加 Δ dB | Output 减少 Δ dB | Output 不动 |
| Makeup 增加 Δ dB | Output 减少 Δ dB | Output 不动 |
| Output 增加 Δ dB | 单压 Threshold 或双压 DOWN 减少 Δ dB | Threshold / DOWN 不动 |

- 减少数值时方向相反；拖动、双击输入、Alt 复位遵守同一规则。
- 双压 UP 按此前用户要求独立，不加入 Output / Makeup 联动。
- 调整 Threshold 不改 Makeup，调整 Makeup 不改 Threshold，调整 Output 不改 Makeup；保持既定方向，不改成三个参数同时移动。
- Ratio 为 1:1、单压上压 Ratio、下压关闭时仍然联动。不得通过 Ratio、ON/OFF、Mix、Classic/Super、Input、Lookahead、Window、Safe、OS、侧链或数值 L/R Link 判断联动是否生效。
- 改变这些其他参数本身不应移动 Threshold / Makeup / Output，也不得在下次编辑时补算任何累计差额。
- 仍遵守参数真实上下限及 UP/DOWN 的合法顺序。在达到边界时限制双方共同位移，不能只限制一端而破坏 1:1；这不构成依赖 Ratio 或动态状态的例外。
- A/B、工程恢复、撤销重做和宿主参数回读恢复其保存值，不当作新的用户联动手势。
- Clipper 共用该控件关系，但其 LINK 默认关闭；本次不改变 Comp 的 Input/Output 联动规则。

### 自动验证

用例：`tests/limiter_unconditional_link_checks.inc`。

在已配置的 D 盘构建目录执行：

```powershell
cmake --build <build-directory> --config Release --target QQSCContractCheck --parallel 3
```

新的构建目录需要启用 `-DQQSC_BUILD_DYNAMICS_CHECK=ON`。检查失败会返回非零状态；Windows 构建流程在打包前运行此目标。配置/编译仍使用本机已有的 Visual Studio 开发环境。

### 本次缺陷证据

- 旧实现以 `effectiveSingleRatio != 1`、`downRatio > 1` 和 `downEnabled` 限制双向联动，违背上述规则。
- 修复前测试复现：LINK 开启、单压 Ratio 1:1，Threshold 从 −20 降到 −21.25 dB，Output 错误地停在 6 dB；正确结果应为 7.25 dB。
- 旧条件在 2026-10-01 的 1.2.41 提交 `0de6608` 已存在；现有提交粒度不足以证明最初编写的具体日期。
- 本次证据目录：`local Limiter Link validation evidence`。保留失败复现和最终验证日志，不能用重新生成的基线掩盖回归。

## THRESHOLD-001：取消 Super 的 −inf 阈值（历史，已由 THRESHOLD-002 取代）

用户于 2026-10-14 要求：Super 的默认阈值为 −90 dB，并明确“不需要 −inf”。

- Super、Classic 的 Threshold / UP / DOWN 都采用 −90～0 dB 的有限范围；有限 Range 同样最低为 −90 dB，Range OFF 保留。
- 原先为 −inf 的新实例默认值与阈值 Alt 复位改为 −90 dB；原先为 0 dB 的 DOWN、Limiter/Clipper 单压默认值保持 0 dB。
- 旧工程、预置与 A/B 中低于 −90 dB 的边界统一钳制到 −90 dB，实际声音使用有限阈值；界面不得显示 Threshold OFF 或 −inf。
- 保留参数 ID、顺序，以及 −90～0 dB 原有宿主自动化位置；原自动化低于 −90 dB 的区间钳制到 −90 dB。
- 音量表的静音 −inf 显示仍然有效；本规则只改变动态边界，不改变 LINK、Ratio、Lookahead、Window、Safe 和过采样规则。

验证：tests/finite_threshold_checks.inc；已纳入 QQSCContractCheck 必跑门禁。旧实现首次运行失败：默认值实际 −120（−inf），期望 −90。

## DISPLAY-001：框选放大后始终跟随最新音频

用户要求框选后继续随播放滚动、保持放大比例，参数调整仍实时更新历史曲线。2026-10-16 用户报告长时间滞后，并确认右键恢复完整视图后立即同步；用户随后授权将 Lookahead 改为 0/25.3/50.6/76/105 ms，并构建覆盖，Display 修复一并交付。

- 框选只设置显示时长和纵向 dB 范围。横向右边缘始终代表最新音频，不保留选区距“现在”的历史偏移。
- 多次放大、播放时间增长、A/B、换模式、保存和重开工程均不得累加历史偏移。
- 旧工程的 `qqscDisplayZoom3` 偏移忽略并保存为 0；其余三个有效缩放值保留，不把旧工程整个视图复位。
- 鼠标右键仍恢复完整视图。缩放不改变阈值、声音、插件延迟、Lookahead/Window 或音量联动。
- 不改变实时采集和历史曲线重算的调度，不通过降低响应速度解决滞后。

缺陷复现：1.4.1 真实编辑器中，选中 8 秒图表的中间一半后，保存的偏移为 2 秒；最新点 x=1235，图表右边缘 x=844，最新音频被画到可见范围外。日志：`local Display baseline reproduction log`。

测试：`tests/display_live_follow_checks.inc` 已加入 `QQSCContractCheck`；`tests/display_zoom_checks.inc` 原先错误接受 2 秒偏移的断言改为 0，其余缩放、对齐及音频不变性检查保留。还需运行 `display-scale` 和 `display-replay-drag` 检查交互与实时响应。2026-10-16 已通过修复后的 `QQSCContractCheck`，其中重复缩放与旧偏移恢复检查通过；完整交付结果见 `DEVELOPMENT_CHECKPOINT_1.4.2.md`。

## LOOKAHEAD-001：五档与小数显示

用户于 2026-10-16 在 1.4.2 中用 105 ms 档试听/测量后确认 100～105 ms 差异不大，要求最后一档定为 **100 ms**。当前五档为 **0、25.3、50.6、76、100 ms**；不改成任意输入 Lookahead。

- Comp、Clipper、Limiter 使用相同五档；菜单和宿主参数文字保留 25.3、50.6 的小数。
- 首次默认使用第一非零档 25.3 ms；新实例继续记忆用户上次选择，旧记忆就近映射到当前档位。
- 真实切换 Lookahead 后 Window 恢复为该档最大值；Alt/Option 同样恢复最大值。同档重复选择不重置 Window。
- 最大档为 100 ms，Window 在 0～100 ms 内调整；输入更大值钳制到 100 ms。调整 Window 不改变已报告延迟。
- 旧工程及 A/B 中 26→25.3、40→50.6、80→76、105→100 ms；100 ms 保留；更早的 5/10 ms 继续迁移到第一非零档。保留参数 ID、顺序与各模式独立保存；保留已保存的 Window 比例。
- 音频、Display 重算与预分配缓冲使用统一最大值 100 ms。实际延迟按宿主采样率取最近整数采样；过采样和 Limiter Ceiling 的既有附加延迟保留。
- 不改压缩算法、Limiter 的无条件 Output LINK、Safe 或过采样默认规则。

验证入口：`lookahead-presets`（纳入 `QQSCContractCheck`），以及 `QQSCLimiterVSTCheck --lookahead143`。包含新档位文字、旧状态/各模式/A-B 恢复、Window 范围、不同采样率/OS 下的延迟和实际脉冲位置。

## NAMING-001：Safe 更名为 Transparent

用户于 2026-10-16 要求将 Safe 按钮更名为 Transparent，以表达其在瞬态、细节方面的用途，而非仅用于减少噼啪声。

- 按钮显示 TRANSPARENT；相关 Window、Range 中文悬停说明和宿主 Detector 选项使用 Transparent。
- 仅修改用户可见文字，不改变检测算法、参数 ID/顺序/数值、默认关闭、三种模式及 A/B 的保存行为。
- Lookahead 或 Window 为 0 ms 时仍变灰禁用，恢复非零后保留原开关状态。
- 保留 Window 可能压低峰值前后声音的说明，不承诺所有素材均具有相同的瞬态或细节改善。旧工程 Safe 状态原样对应 Transparent。
- 旧记录与源码内部的 Safe 名称可保留为历史或内部标识，不作全局替换。

## DISPLAY-002：滚轮缩放替代框选

用户于 2026-10-16 在 1.4.4 开发期间要求：取消框选放大，改为普通滚轮以鼠标位置为中心纵向缩放，Shift + 滚轮横向缩放。本条替代 DISPLAY-001 的框选交互，继续保留其最新音频跟随规则。

- 向上滚动放大，向下滚动缩小；普通滚轮保持鼠标所在电平的屏幕位置，达到 −90～0 dB 边界时限制显示范围。最小跨度仍为 1 dB。
- Shift + 滚轮仅调整显示时间范围（0.1～8 秒），右侧始终为最新音频，历史偏移保持 0；不会为了横向鼠标锚点引入固定滞后。
- L/R 与 M/S 任意子图均支持操作，两个子图继续使用共同范围。图外滚轮无效；左键拖动不再框选；右键恢复 −90～0 dB、8 秒全局视图。
- 保留阈值推子对齐、工程缩放保存、A/B 一致性，以及 Window/HPF 等参数改变时的实时曲线重算。缩放不修改声音、阈值或延迟。
- 对应交互测试改为滚轮事件，继续检查音频不变性；不得恢复旧的框选历史偏移断言。

## DISPLAY-003：纵轴刻度小数位

用户于 2026-10-16 要求 Display 左侧电平刻度只保留一位小数，即 0.1 dB。所有缩放范围使用相同显示格式，不再根据网格跨度切换小数位。仅格式化刻度文字，不量化实际视图范围、阈值、声音参数或曲线坐标。

## DISPLAY-004：滚轮缩放细化

用户于 2026-10-16 反馈一次滚动变化过大，要求更细微。1.4.6 将普通滚轮与 Shift 滚轮的指数灵敏度降为 1.4.5 的四分之一（3 → 0.75），四次相同滚动达到原一次的缩放幅度。不增加延时、平滑追赶或改变鼠标锚点、最新音频跟随、右键复位和缩放范围。现有交互检查改为输入四次事件，原来的几何、状态和音频断言保持不变。

用户同轮追加：Display 中文悬停说明同步描述小步滚轮缩放、连续滚动逐步调整、Shift 横向缩放和右键恢复全局视图。与灵敏度修改合并为同一未交付的 1.4.6。

## DISPLAY-005：框选与滚轮并存

用户于 2026-10-16 要求把框选保留下来，与 1.4.6 的细步幅滚轮同时使用，并同步修改 Display 中文说明。本条替代 DISPLAY-002 中取消框选的约定。

- 左键在图表内拖动框选，同时改变纵向电平和横向时间比例；保留反向框选与子图边界限制，小于 6 像素的选区不生效。
- 框选与细步幅滚轮可以交替使用。普通滚轮纵向、Shift 滚轮横向，灵敏度保持 0.75。框选进行中暂不处理滚轮，避免一项手势改变另一项的基准。
- 两种方式都保持历史时间偏移为 0，右侧始终跟随最新音频；恢复框选不能恢复过去的固定历史滞后。
- 右键恢复全局并取消正在进行的框选；尺寸变化时取消框选。保留一位小数刻度、推子对齐、实时响应和声音不变性。
- 中文悬停说明同时列出框选、细步幅滚轮、Shift 横向缩放及右键全局复位。


## DISPLAY-006：FREEZE 控制停播后冻结

用户于 2026-10-16 确认：在 Display 右上方增加 FREEZE 小按钮。点亮后，播放中正常滚动，宿主停止播放后才冻结；关闭时保持原有更新。首次默认点亮，新开插件使用上次手动选择。

- Freeze 只暂停 Display 原始历史及对应采样点采集；保持音频处理、电平表、Window/HPF 参数重算和缩放交互。冻结超过历史缓存长度也不能丢掉原始证据。重新播放恢复采集，右端仍跟随当前音频，不累计停播时间或历史偏移。
- ECO/FULL、Comp/Clipper/Limiter、ST/LR/MS 均支持。录音、离线导出及没有宿主播放状态时继续采集，不凭信号静音猜测停播。
- 各实例独立，工程保存该实例开关，不受 A/B 影响。只在用户手动点击时写入新实例默认值，工程恢复不修改全局偏好；旧工程无此字段时使用新实例偏好。
- 保留现有冻结后改变通道模式或侧链来源时的历史源检查，不将旧声道数据误标为新模式。
- 按钮及 Display 悬停说明使用显式 UTF-8 中文。门禁：tests/display_freeze_checks.inc，display-freeze。


## DISPLAY-007：Ctrl 滚轮与停播鼠标中心

用户于 2026-10-16 确认：横向缩放由 Shift 改为 Ctrl + 滚轮；停播时以鼠标位置为中心，播放时跟随最新音频。本条覆盖 DISPLAY-002/005 的旧修饰键与停播时固定右端规则。

- 普通滚轮纵向、Ctrl 滚轮横向；Shift 不再切换横向，旋钮/推子的 Shift 微调保持原样。步幅继续使用 0.75。
- 宿主明确停播时，横向缩放保持鼠标下的时间位置，直到完整 8 秒历史边界；框选停播历史显示实际选区。FREEZE 开关仍只控制停播采集。
- 停播查看位置仅存在当前 Display 内，不写入工程或 A/B 的历史偏移。播放、录音、离线渲染或未知播放状态恢复最新右端，保持缩放比例；旧工程偏移迁移为 0，避免历史滞后复发。
- 右键恢复全局，保留框选、纵向鼠标中心、一位小数、阈值对齐、Window/HPF 实时重算；只影响显示，不改变音频。
- 中文悬停及中英文说明书同步说明 Ctrl、停播/播放区别、FREEZE。
- 验证：display-freeze 停播指针及恢复测试，display-scale，display-live-follow 和 QQSCContractCheck。


## READOUT-001：Lookahead 数值与 ms 完整可见

用户于 2026-10-16 报告 1.4.9 的 Lookahead 显示为“25.3…”；需要完整显示数值及 ms 单位。

- 所有 0 / 25.3 / 50.6 / 76 / 100 ms 选项在三个主题、处理模式及 ST/LR/MS、单压/双压、最小合法窗口尺寸下都不得被省略号截断。
- 保持 11 px 逻辑字号及 Lookahead / Window 原有大小、同排关系；只减少该下拉框的箭头占位和文字内边距，不影响其他下拉框。
- 不改变 Lookahead 参数、自动化、延迟、Window 复位、Display 缩放/冻结或音频处理。
- tests/lookahead_presets_checks.inc 验证真实标签字体、可用区域及各档字符串宽度，已随 QQSCContractCheck 运行。修复前“25.3 ms”需 37.2558 px，实际仅 26 px，断言失败。


## DISPLAY-008：顶部区域的滚轮缩放

用户于 2026-10-16 要求 Display 顶部标题区域也支持缩放，以 0 dB 为纵向参考，同时允许横向缩放。

- 顶部标题及图表上方留白支持普通滚轮，0 dB 固定在上沿；已有偏移范围也恢复此参考。步幅保持 0.75、跨度 1～90 dB。
- 顶部 Ctrl + 滚轮同图表内部：停播以鼠标横坐标为中心；播放/录音/离线/未知状态跟随最新音频。时间范围仍为 0.1～8 秒。
- 图内普通滚轮仍以鼠标电平为中心；保留框选、右键复位、阈值对齐、FREEZE 和实时参数重算。滚轮经过 FREEZE 按钮不改变其开关。
- 不改变音频、参数、延迟或历史采集调度。顶部缩放只改变视图。
- 修复前 display-scale 失败：Header wheel must zoom vertically from 0 dB。该测试加入 QQSCContractCheck；display-freeze 同时覆盖顶部停播中心。

## METER-001：Clipper 实时柱条与最大值分别显示

用户于 2026-10-16 明确选择“柱条实时，数字与标记线保持最大值”。此条细化此前 Clipper GR HOLD 的要求。

- 左右柱条实时显示当前动态衰减；数字及标记线独立保持本次播放的最大衰减。柱条不得使用保持值。
- 停止播放后保留最大值；双击 Gain Reduction 区域或下次开始播放时清零。现有左右独立、ECO 后台记录、关闭再打开编辑器及 Window/Lookahead 重设保持规则不变。
- Makeup / Output Gain 不计入衰减量。不改变 Comp / Limiter 表计行为、DSP 或刷新频率。中文悬停明确区分柱条与保持值。
- 修复前真实绘制检查失败：Clipper bar ignores live GR while maximum is unchanged。clipper-hold 检查三个主题下柱条变化、数字/标记不动及原有清零规则，纳入 QQSCContractCheck。


## DISPLAY-009：Shift 滚轮纵向平移

用户于 2026-10-16 要求 Shift + 上下滚轮上下移动 Display，不放大，并同步悬停说明。本条替代 Shift 与普通滚轮相同的旧行为。

- Shift + 滚轮纵向移动可见电平范围，跨度与时间比例不变；向上查看更高电平，向下查看更低电平。每单位 delta 移动当前跨度的 15%，细步幅，无额外延迟。
- 两个纵向边界一起移动，限制在 −90～0 dB 内；完整 90 dB 视图已覆盖全局，平移不缩小或越界。
- 顶部标题区域及 ST/LR/MS 各子图均支持；在顶部平移时不强行将上沿重置为 0 dB。普通顶部滚轮的 0 dB 缩放保持原样。
- Ctrl + 滚轮仍只横向缩放，Ctrl 与 Shift 同按时优先 Ctrl；图内普通滚轮仍以鼠标电平缩放。旋钮/推子的 Shift 微调不变。
- 保留停播浏览的横向位置、FREEZE 历史、播放跟随、框选、右键复位、推子对齐、A/B/工程保存以及 Window/HPF 的实时重算；不得改阈值、声音或延迟。
- 中文 Display 和 Freeze 悬停说明同步。tests/display_zoom_checks.inc 与 display_freeze_checks.inc 验证平移及边界，随 QQSCContractCheck 运行。


## DISPLAY-010：整个 Display 起拖，框选超限贴边

用户于 2026-10-16 要求扩大鼠标框选范围，超出极限的部分按极限边缘计算。本条替代只能从曲线绘图区起拖的规则。

- Display 面板内的标题、纵轴刻度、底部图例及留白均可作为左键框选起点；FREEZE 按钮保持原点击行为。不能从 Display 外的其他控件启动选区。
- 起点和终点分别限制到实际曲线绘图区的最近边缘；拖动中可以越出整个面板，超出部分不参与时间/电平缩放计算。选区轮廓只绘制有效部分。
- LR/MS 双图以起点最近的子图为准，在一次拖动期间不切换子图；两图继续共用显示比例。中间间隙选择最近边缘，等距时使用上图。
- 全部落在同一条边缘的零宽/零高选区不放大；保留原小于 6 像素的有效选区忽略规则、反向拖动、右键/调整界面尺寸取消选区。
- 保留普通/Shift/Ctrl 滚轮、冻结后选中历史区间、播放跟随最新、阈值对齐、实时参数重算和音频不变性；不改变参数、声音或延迟。
- 中文 Display 悬停同步说明。display-scale 验证所有边缘、越界终点、双图归属、按键保留；display-freeze 验证停播选区位置，均纳入 QQSCContractCheck。


## HELP-001：Display 悬停只列常用操作

2026-10-16 用户要求精简整段 Display 悬停。保留框选、右键恢复、普通/Shift/Ctrl 滚轮、FREEZE 停播保留和不影响声音的简短说明。不在悬停中介绍扩大的框选范围、边缘计算或缩放中心等细节；详细规则放到说明书。此次只改文字，不改变任何交互或声音。

## ECO-001：关闭 UI 后停止分析，漏算的播放不能 Match

2026-10-17 用户重申原设计：ECO 关闭 UI 后播放的内容不计算；停止播放后重新打开 UI，不能使用这次未完整统计的播放执行 Match。此规则独立于 FREEZE。

- “关闭 UI”包括销毁编辑器、隐藏 JUCE 组件，以及宿主仅隐藏原生外层窗口但保留编辑器对象。不能只凭对象存在或 Component::isShowing() 判定仍在显示。
- ECO 在 UI 实际不可见时停止 Display 历史采集与重算、普通表计分析、LUFS 与 Match 统计；音频处理继续。Clipper 最大 GR 的既有后台保持约定不变。
- 本轮播放只要存在未分析片段，重新打开 UI 或停止播放都不能使残缺数据变为有效 Match；打开 UI 后重新开始一次完整播放才恢复。已有累计 Match、重复点击不重置起点的规则不变。
- FULL 关闭 UI 后仍继续分析，重新打开可以使用完整统计；不可见时无需生成 Display 绘制路径。
- FREEZE 仍只控制停播后是否保留图表，不得被用作 ECO 后台分析开关。不得改变音频、延迟、参数、侧链或任何 LINK 规则。
- 原生窗口可见性查询只在消息线程执行，不放入音频回调。界面挂接/可见性回调及时同步状态，保留周期检查以覆盖原生宿主隐藏时不发 JUCE 回调的情况。

验证：tests/eco_ui_visibility_checks.inc 使用真实处理器和编辑器、始终隐藏的原生父窗口，检查三种处理模式、FREEZE 两种状态、ECO/FULL、Display 重算取消、Match 拒绝及下一轮恢复和音频一致性。tests/cumulative_match_checks.inc 验证累计 Match 行为；两者纳入 QQSCContractCheck。

修复前：1.4.14 的原生窗口不可见，但 editorOpen=1，后台仍累计约 2.13 秒波形，Match 可实际执行。新回归断言在旧共享代码上失败：Native-hidden editor incorrectly published as open。

## ECO-002：ECO 重开 UI 从当前音频重新绘图，与 Freeze 开关无关

2026-10-18 用户要求修复 QQ Host 保留编辑器对象时的旧图续接问题，并指定版本 1.4.16。修复位于超级压缩，不修改 QQ Host。

同日用户在 1.4.16 验证后进一步明确：Freeze 点亮也必须在 ECO 重开 UI 时清空旧波形，避免误导；Cubase 当前表现正确，QQ Host 的保留编辑器路径仍有问题。1.4.17 因此取消原来的 Freeze 关闭条件，以本条更新后的规则为准。

- ECO 暂停 UI 分析后，恢复分析必须清空旧曲线及历史重算缓存，从恢复后采集的音频重新绘制，与 Freeze 是否点亮无关。不能把隐藏前后的两段波形直接接在一起，也不能在新音频到达前重复添加旧表计帧；隐藏期间停播后重开也应为空图，等待新的音频证据。
- 清理应使旧的异步重算结果失效；与音频回调交错的缓存换代不能把旧帧标成新数据。UI 缓存分配只在消息线程执行。
- 保持用户的电平和时间缩放。FULL 的后台分析不变；界面持续打开时，Freeze 点亮仍保留停播图表。Freeze 不决定 ECO 是否在后台分析，也不能保留 ECO 隐藏之前的旧图。
- 清理 Display 不重置音频处理器，不恢复残缺播放的 Match 资格。只有重新开始完整的 UI 可见播放才恢复 Match；不改变 LINK、阈值、声音或延迟。

验证入口：tests/eco_ui_reopen_checks.inc / eco-ui-reopen，纳入 QQSCContractCheck。使用屏幕外的独立原生测试窗口，真实执行显示、隐藏、继续播放、重新显示，以及隐藏期间停播的流程，覆盖 Comp ST/LR/MS、Clipper、Limiter、ECO/FULL、Freeze 两种状态。1.4.15 修改前失败：ECO Freeze-OFF reopen retained stale Display history。

1.4.17 修改前：按用户更新后的规则运行回归，1.4.16 在 ECO + Freeze ON 的保留编辑器重开场景失败：ECO reopen retained stale Display history (Freeze must not preserve a hidden gap)。


## DYNAMICS-002：Range OFF 时保留超过 0 dB 的真实检测电平

2026-10-26：用户报告 Classic 1000:1 时，Clipper 输出明显高于阈值，Transparent 开启及 100 ms Window 仍然如此。真实 1.4.17 处理器复现：阈值 −3.65 dB，输入峰值 +2 dB，输出 −1.646350 dB；正确 Classic 输出应为 −3.644350 dB。原因是检测采样与下压计算把幅度限制为 1.0。

- Clipper、Limiter 以及 Comp 的 Range OFF 路径都必须保留超过 0 dB 的真实幅度。按各自 Classic / Super 公式继续处理；不得在检测器、增益计算、Display 缓存或历史重算中重新引入 0 dB 封顶。
- 有限 Range 保留现有作用：超过 Range 不作动态处理。有限 0 dB Range 与 Range OFF 仍不同；上压、相等阈值禁用动态处理等边界规则保持。
- Threshold / UP / DOWN 的可调范围及 Display 缩放范围不因此改变。Limiter Ceiling 保持最终独立保护；Clipper 不新增 Ceiling。
- 原有 Window、Transparent、OS、Mix、联动、A/B 与实时参数回算继续工作。
- tests/headroom_checks.inc 的 headroom 检查纳入 QQSCContractCheck：真实处理器、正电平 Display 与历史重算、A/B 计算路径、有限 Range 和上压边界。

## CEILING-001：Limiter 独立 Ceiling 开关

2026-10-26 用户要求增加 Ceiling 开关，默认开启，关闭后 Ceiling 机制不工作。

- 只在 Limiter 显示 CEILING 开关；点亮开启。关闭时跳过真峰值与采样峰值限制，不应用 Ceiling 电平/历史校准偏移，不增加 Ceiling 附加延迟；核心 Lookahead、Window、OS、压缩与 Output Gain 保持工作。
- 关闭后 Ceiling 数值、TP、恢复方式变灰并保留。重新开启恢复原设置。开关随工程、预置和 A/B 保存，不影响 Comp/Clipper 参数库。
- 旧工程及旧 A/B 缺少此参数时按开启恢复，包括载入到原先关闭的实例。参数追加到列表末尾，不改变旧 ID 或编号。
- Threshold、Makeup、Output 的 LINK 仍独立，仅由 LINK 开关决定。Ceiling 开关不能成为联动条件。
- Display 回算也使用当前 Ceiling 开关；关闭后不再画出 Ceiling 的限幅。开关改变实际附加延迟，宿主按新值补偿。

验证入口：ceiling-switch，包含实际音频、附加延迟、保存恢复、旧工程、A/B、界面与无条件 LINK，纳入 QQSCContractCheck。

## DISPLAY-011：Input 移动压缩前曲线，刻度与阈值保持固定

2026-10-26 至 27 日用户明确替代早期设计：Input 影响 Display 中压缩前曲线的位置；左侧刻度不动，阈值线不再为 Input 偏移。此次未交付的“移动左刻度”方案取消。

- 白色 Input 曲线显示当前 Input Gain 后、动态压缩前的电平。调整 Input 对保留历史立即回算，不累加先前的 Input 值。
- 蓝/绿动态曲线与白线使用相同的 Input 后基准；橙色 Output 包含 Input、Makeup、Mix、Output Gain 和当前启用的 Ceiling，各项增益只计一次。
- 左侧刻度、Threshold/UP/DOWN/Range 的线及推子始终按相同绝对 dB 坐标；Input 不改变视图范围或参数值。缩放和平移仍只由视图操作决定。
- EXT 波形仍表示外部侧链，不随主 Input 改变。Window/Transparent 保留对压缩结果的作用，不用检测包络替代主音频波形。Bypass 显示真实未处理输入。
- Clipper 不显示 LUFS 响度框；Limiter 继续显示。Clipper 的 Match 功能不因此关闭。

验证入口：input-display（ST/LR/MS、Clipper/Limiter、INT/EXT、保留历史重算），纳入 QQSCContractCheck。

## MATCH-001：原始输入响度为参考

2026-10-27 用户先要求 Input 后参考，随后明确改为 Input 前，最终确认移除 Input 音频增益并改为纯显示 Adjust。后一个决定覆盖本轮早先方案，不得恢复 Input 后匹配或为 Adjust 增加监听补偿。

- Comp ST/LR/MS、Clipper、Limiter 均比较延迟对齐的原始输入与动态压缩后的单位 Makeup Wet，求绝对 Makeup 目标。
- Adjust、Output Gain、Ceiling、耳机监听不进入 Match 统计；累计统计、重复点击不叠加、完整播放资格、ECO 隐藏 UI 不计算的规则保留。
- Limiter/Clipper Output LINK 独立作用于 Match 引起的 Makeup 编辑，继续遵守 LINK-001。
- 验证：input-match、cumulative-match、ECO 门禁；不允许为了兼容历史 Input 测试恢复已取消的增益。

## MONITOR-001：监听抵消 Output，Adjust 不参与音频

2026-10-27 最终规则：Input 音频增益已取消。耳机监听保留处理结果，在 Limiter 与开启的 Ceiling 后抵消 Output Gain；Adjust 不需要也不允许任何音频补偿。不修改 Threshold、Ratio、Makeup、Mix 的处理。

验证：input-monitor，对比 Adjust、旧 Input 宿主自动化变化前后的真实音频；包括 Ceiling 开关、Ratio、Makeup、Mix。

## CEILING-002：TP OFF 使用采样峰值 Limiter

2026-10-27 用户明确用普通采样峰值 Limiter 取代非 TP 硬削波，并在 TP 关闭时保留 Auto、Tight、Smooth。

- TP 开启：保持真峰值限制，检测重建的采样间峰值。TP 关闭：以宿主采样率下的最终采样峰值为准，使用前瞻增益衰减和恢复，不再走硬削波。
- 两种模式共用已有恢复选择：Tight 恢复较快，Auto 自适应，Smooth 较慢。按钮均可操作；切换 TP 不重置恢复选择或 OS。原参数 ID 保留，旧项目中 TP OFF 从 1.4.19 起使用新采样峰值行为。
- 相同 Lookahead 与 OS 下，切换 TP 不改变 Ceiling 附加延迟。主 CEILING 关闭时两种限制都停用，三个子控件保留数值并禁用。
- Display 对两种模式均回算恢复过程；只有 TP 使用采样间峰值证据。实时仪表仍来自真实 DSP。Clipper 模式本身不受此 Ceiling 改动影响。

验证入口：sample-peak-ceiling；包括实际峰值上限、峰后恢复、三种恢复差异、已知采样间超峰对照、TP OFF 控件和状态保存。修改前用例已复现硬削波无恢复；另保留当前 TP 音频前后对照证据。

## CEILING-003：原生采样率 TP 最终重建保护

2026-10-27：额外压力测试在 1×、44.1 kHz 发现最高约 0.033 dB 真峰值超限。用户最初选择保留 TP，随后明确追加“把 tp 那个也纳入修改”，以后一个指令为准。

- 仅将原生 1× 的 TP 重建保护余量从 0.01 dB 收紧为 0.05 dB，在触发最终保护时生效，不对所有输出附加固定衰减。4×/8×/16× 的原保护余量保持 0.01 dB。
- TP、SP 均保留 Auto/Tight/Smooth、Ceiling 开关、当前延迟、用户 OS 选择及独立 LINK。SP 仍只保护最终采样峰值，不使用 TP 余量。
- Display 使用与 DSP 相同的当前 TP 保护余量。TP 基线“保持原算法”的旧范围决定已被用户后续追加修复取代，不可据此前决定回退修复。
- sample-peak-ceiling 必需检查内加入两组原始失败信号、三种恢复设置的真实重建输出检查。补充 ceiling_check 原生高频扫描和全采样率/OS 压力检查。

## DISPLAY-012：Adjust 替代 Input，纯显示平移

2026-10-27 用户明确取消 Input 旋钮，改为 Adjust；用户同时确认“旧工程也取消 Input 增益，接受声音变化”。此条覆盖 DISPLAY-011 中 Input 移动波形并改变输入电平的设计。

- ADJUST 位于原 Input 位置；默认 0，正值向上、负值向下。波形、刻度、阈值线一起平移，实际 dB 数值、压缩量、响度、音频延迟均不变。中文悬停说明保持简明。
- Adjust 保存为工程显示状态，与现有显示缩放配合；不随处理模式或 A/B 改变。不复用旧 Input 自动化 ID，也不改其他参数的序号。
- 图表右键恢复完整视图并将 Adjust 归 0；旋钮 Alt/Option 只将 Adjust 归 0。保留框选、普通滚轮缩放、Shift 平移、Ctrl 时间缩放、Freeze、实时曲线重算。
- 全部模式的旧 Input 增益和 Input/Output LINK 停用，界面不再显示该 LINK；保留旧 ID 仅供工程/自动化结构兼容。任何旧状态、A/B 或宿主自动化都不得使 Input 再次进入音频。已有 Output、Makeup、阈值保持其保存值，不擅自补偿旧 Input。
- Limiter/Clipper 的 Output LINK 保留，继续遵守 LINK-001，不和 Adjust 关联。
- 验证：display-adjust（真实编辑器坐标/状态/缩放/复位），input-monitor（真实音频差值），input-match，QQSCContractCheck。

## THRESHOLD-002：全部阈值支持 −inf，已有默认值保留

2026-10-27 用户明确：Super、Classic，以及 Comp/Clipper/Limiter 的 Threshold、UP、DOWN 全部支持 −inf；已有默认值保持（−90 或 0）。本条覆盖 THRESHOLD-001。

- 旧 −120 宿主坐标作为 −inf 端点保留；其他有限阈值坐标和参数 ID/序号不变。默认不改成 −inf；原来 −90 dB 和 0 dB 的工厂默认值保持。阈值 Alt/Option 仍恢复原有 −90 dB，DOWN 的 0 dB 复位保持。
- Classic 的 −inf 下压遵守固定 dB 比例的极限：Ratio > 1 时对非零信号增益为 0，Ratio 1:1 仍直通。Super 保留其有理曲线的 −inf 形式。向上压缩取消下方门限时保留各自算法，不将 Classic 替换成 Super。
- 有限 Range 的 −90～0 dB 范围与 OFF 保留；Range 不随本条增加 −inf。双压上下边界相等时仍不做动态处理。
- Adjust 只改变有限阈值线的屏幕位置，不改阈值数值；−inf 无有限坐标，标在图表底边。所有显示与音频必须使用相同的实际阈值。
- 验证入口保留 finite-threshold 名称以兼容构建脚本，用例现检查 −inf 端点、默认值、文本输入、算法切换、工程/A-B、Classic/Super 极限与有限低阈值；不能恢复旧的“−inf 必须钳制到 −90”断言。

## THRESHOLD-003：Threshold、UP、DOWN、Range 上限 +24 dB

2026-10-27 用户确认全部阈值允许超过 0 dB，选择 +24 dB；随后明确 Range 也允许上限 +24 dB。本条覆盖之前的 0 dB 上限约定。

- Threshold / UP / DOWN 为 −inf 至 +24 dB，原 −90/0 工厂默认及 Alt 复位保持。有限 Range 为 −90 至 +24 dB，OFF 独立保留。
- 声音处理、Display/历史投影、推子/输入、LINK、工程与 A/B 均使用相同的真实正数边界；Adjust 仍只平移显示。单压上压有限 Range / 双压 DOWN 可作为正数提升终点；Range OFF 的原 0 dB 上压终点暂保持，不随此次范围扩展重定义。
- 参数 ID/顺序不变。工程保存绝对 dB，旧项目与 A/B 保持原数值；schema 33 起 Range OFF 改存 +25，旧 +1 OFF 迁移，不能与新有限 +1 dB 混淆。
- 宿主 0..1 自动化映射随范围扩展而改变；插件无法迁移 DAW 独立保存的归一化自动化曲线，升级后这类已有曲线需要复核。不得声称旧归一化自动化位置完全不变。
- 验证 positive-threshold、finite-threshold、unconditional-link 与完整 QQSCContractCheck。修改前 +24 dB 文本输入被截为 0 dB，red.log 已记录。

## DISPLAY-013：Shift 滚轮直接调整 Adjust

2026-10-27 用户要求 Shift + 上下滚轮与 Adjust 统一，替代旧的受 −90～0 dB 视图边界限制的平移。

- Shift 滚轮修改同一个 Adjust 值并同步旋钮；向上增加、内容向上，向下相反。沿用 Adjust 的 ±90 dB 范围，不修改存储的缩放上下界、时间跨度或停播浏览位置。
- 沿用细步幅（每单位 delta 为可见跨度的 15%），不额外增加刷新延迟。顶部和所有子图可用；Ctrl 同按仍优先时间缩放。
- 数值、波形、刻度、阈值线一致，保存/复位沿用 Adjust。声音、阈值实际值、LINK、Window/HPF 实时响应保持。
- display-adjust 修改前失败：Shift wheel must edit Adjust itself；display-scale / display-freeze 同步验证新规则，不恢复旧的全局视图平移限制。
