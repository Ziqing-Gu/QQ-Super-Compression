# QQ Super Compression 1.2.28 — Verification Notes

## 新增验收：revision1228

`tests/revision1228_ceiling_mode_checks.inc` 验证：

1. Normal 26 ms 在 1x/8x/16x stored choice 下都只报告 26 ms，因为非 0 ms 不启用 compressor oversampling，更不包含 Limiter Ceiling latency。
2. Normal 0 ms / 1x 报告 0 samples。
3. Normal 0 ms / 8x / 16x 只随用户 oversampling choice 增加 PDC。
4. Limiter TP OFF 与 TP ON reported latency 完全相同。
5. 实时 Normal -> Limiter -> TP ON -> TP OFF -> Normal 序列中，只有 Limiter 边界改变 PDC；TP toggle 不改变 PDC。
6. `retrospectiveTpDisplayChecks` 同一 profile 继续检查历史不清空，并验证 TP OFF 的 8x Hard Clip 与 TP ON 的 Recovery projection。

## QQSCCeilingCheck 更新

- TP OFF 以 8x Hard Clip 方式检查 sample ceiling。
- TP ON 继续检查 16x reconstructed true-peak ceiling。
- Hard/TP `latencySamples()` 必须一致。
- TP switch 后 latency 不变，Display GR 保持有限并平滑改变。
- TP Recovery TIGHT/AUTO/SMOOTH 原有排序继续回归。

## 静态审计

`tests/revision1228_ceiling_mode_source_audit.py` 检查：

- Normal process path 对 OutputCeiling 有明确 Limiter gate；
- PDC 只在 Limiter ON 时加入 Ceiling latency；
- TP setter 内没有 prepare/reset/initProcessing；
- Hard Clip 位于 8x oversampled loop；
- Dynamic Display 的 TP OFF 分支包含 Hard Clip Ceiling GR；
- 1.2.27 Display Drag Timing source hooks 仍存在。

## Windows 构建

根目录 `BUILD_WINDOWS.cmd` 会构建：

- `QQSuperCompression_VST3`
- `QQSCLimiterCheck`
- `QQSCCeilingCheck`

并运行 revision1211 / 1218 / 1221 / 1225 / 1226 / 1227 / 1228 / continuity / dual。
