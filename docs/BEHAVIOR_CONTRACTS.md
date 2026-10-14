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
- 本次证据目录：`the local 1.4.0 Limiter Link verification directory`。保留失败复现和最终验证日志，不能用重新生成的基线掩盖回归。

## THRESHOLD-001：取消 Super 的 −inf 阈值

用户于 2026-10-14 要求：Super 的默认阈值为 −90 dB，并明确“不需要 −inf”。

- Super、Classic 的 Threshold / UP / DOWN 都采用 −90～0 dB 的有限范围；有限 Range 同样最低为 −90 dB，Range OFF 保留。
- 原先为 −inf 的新实例默认值与阈值 Alt 复位改为 −90 dB；原先为 0 dB 的 DOWN、Limiter/Clipper 单压默认值保持 0 dB。
- 旧工程、预置与 A/B 中低于 −90 dB 的边界统一钳制到 −90 dB，实际声音使用有限阈值；界面不得显示 Threshold OFF 或 −inf。
- 保留参数 ID、顺序，以及 −90～0 dB 原有宿主自动化位置；原自动化低于 −90 dB 的区间钳制到 −90 dB。
- 音量表的静音 −inf 显示仍然有效；本规则只改变动态边界，不改变 LINK、Ratio、Lookahead、Window、Safe 和过采样规则。

验证：tests/finite_threshold_checks.inc；已纳入 QQSCContractCheck 必跑门禁。旧实现首次运行失败：默认值实际 −120（−inf），期望 −90。
