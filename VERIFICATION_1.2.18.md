# QQ Super Compression 1.2.18 — Verification Notes

## 基线

开发基线为用户已设为 Stable 并完成 Plan B 的 **1.2.17 Build Fix 1**。本候选没有修改该 Plan B 备份。

## 设计契约

TP Recovery 只改变 True-Peak Ceiling 的 release character：

```text
TIGHT  = fast recovery / loudness first
AUTO   = exact 1.2.17 adaptive timing / default
SMOOTH = slower conservative recovery / smoother envelope
```

所有模式共用同一 Ceiling target、lookahead safety、8x TP 主保护、16x reconstruction validation 与 residual correction。Recovery 不改变 Ceiling 数值，也不向信号自动补增益。

AUTO 分支保留 1.2.17 的原公式：

```text
hold = adaptive 1.5–30 ms (repeated envelope => 0)
release = adaptive 15–75 ms (repeated envelope => 60 ms)
```

TIGHT 使用原 lookahead moving minimum，但取消额外 hold，release 为 5–25 ms 的频率自适应范围。

SMOOTH 使用较长 conservative minimum，并采用 60–180 ms 的频率自适应 release。

## Retrospective Display

Recovery 是当前参数，因此必须和 Ratio / Threshold / Mix / TP 等一样重投影整个现有历史。Display 不保存“当时的 Recovery GR”。它在 60 Hz 历史证据上轻量重演不同 Recovery 的释放特征：TIGHT 15 ms、AUTO 50 ms、SMOOTH 140 ms 的视觉时常，用来表现当前选择的相对包络，不重新运行完整 8x/16x DSP。

实时 GAIN +/- meter 与 Hold 不使用这个视觉近似，而是继续读取真实 OutputCeiling envelope。

## 已在当前环境执行

- `tests/strict_one_to_one_link_isolated_test.py`：100,000 + 100,000 随机 1:1 Link 检查通过。
- `tests/limiter_mode_continuity_isolated_test.py`：864 场景、100 次 Normal Single/Dual、252,021 次边界策略检查通过。
- `tests/tp_display_gr_performance_audit.py`：每 sample telemetry 保持线性；每 host block 才做一次 dB 转换。
- `tests/true_peak_ceiling_target_source_audit.py`：TP Ceiling 主目标、16x reconstruction 和 0.01 dB residual safety 规则通过。
- `tests/tp_recovery_source_audit.py`：Recovery 参数/UI/state hook、AUTO 原公式、100,000 组显示 release 排序与 Ceiling 独立性通过。

## 仍需 Windows 实机验证

`BUILD_WINDOWS.cmd` 将运行：

- `QQSCLimiterCheck revision1211`
- `QQSCLimiterCheck revision1217`
- `QQSCLimiterCheck revision1218`
- `QQSCLimiterCheck continuity`
- `QQSCLimiterCheck dual`
- `QQSCCeilingCheck`

`revision1218` 检查 Recovery UI 循环、TP OFF/ON 保留、A/B 独立记忆、工程保存恢复，以及切换 Recovery 后旧 Display 历史的 retrospective 变化。

`QQSCCeilingCheck` 额外检查真实 OutputCeiling 在 44.1 / 48 / 96 kHz 下 TIGHT / AUTO / SMOOTH 的恢复排序，同时保留原 TP Ceiling 上限测试。

完整 Windows JUCE/MSVC 构建、Cubase 听感和多实例 CPU 观察尚未在当前环境执行。
