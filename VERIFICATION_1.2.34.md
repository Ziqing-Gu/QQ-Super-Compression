# QQ Super Compression 1.2.34 Verification

## 当前状态

**Candidate — 需要 Windows / Cubase 实机验收，不是 Stable。**

## 已完成源码级检查

- `project()` 版本已更新为 1.2.34。
- FULL / ECO 使用全局 `PropertiesFile` 偏好，不进入工程声音参数。
- ECO 分析 gate 是 per-instance，并跟随 editor `isShowing()`。
- Dynamic Display / MATCH / Meter / 4x TP Meter 等重计算均受 UI-analysis gate 控制。
- Audible DSP / PDC 不受 FULL / ECO gate 控制。
- Ceiling OS 参数为 append-only，旧工程缺失时默认 8x。
- TP + stored 1x 的 effective Ceiling OS = 8x，stored 值不被改写。
- Ceiling 1x 为 Native Hard Clip。
- 8x / 16x OutputCeiling 对象均在 prepare 阶段初始化。
- 0 ms Core 8x + Ceiling 8x、Core16x + Ceiling16x 存在 shared audio path。
- 1.2.27 Display Drag Timing、1.2.28 Ceiling/TP、TP Recovery 等源码审计继续通过。

## 本环境已运行

```text
python tests/revision1234_performance_architecture_source_audit.py
python tests/revision1227_display_drag_performance_audit.py
python tests/revision1228_ceiling_mode_source_audit.py
python tests/tp_recovery_source_audit.py
python tests/tp_display_gr_performance_audit.py
python tests/display_render_performance_selftest.py
```

上述检查均通过。

## Windows BUILD_WINDOWS.cmd 将执行

- QQSCLimiterCheck revision1211
- revision1218
- revision1221
- revision1225
- revision1226
- revision1227
- revision1228
- **revision1234**
- continuity
- dual
- QQSCCeilingCheck

revision1234 重点验证：

- Normal latency 不随隐藏 Ceiling OS 改变；
- Ceiling 1x / 8x / 16x PDC 顺序；
- TP + stored 1x 自动提升到 effective 8x；
- 固定 8x / 16x 下 TP 开关 PDC 不变；
- 0 ms 8x/8x 与 16x/16x shared path 去掉重复 Ceiling filter latency；
- FULL / ECO 在无 editor 情况下可听输出逐样本一致，PDC 一致；
- Ceiling 1x 为真正 Native Hard Clip。

## 必做 Cubase 实机验收

1. Normal FULL / ECO：声音与 PDC 完全不变。
2. ECO：UI 关闭后 ASIO-Guard 应明显下降；重新打开 UI 不应卡顿或爆音。
3. FULL：UI 开关不应改变后台分析连续性。
4. MATCH：FULL 维持原播放期间累计；ECO UI 关闭暂停，重新打开后开始新累计，但已经应用的 Match Gain 不变。
5. Limiter TP OFF：Ceiling OS 1x / 8x / 16x 均工作。
6. Limiter TP ON：1x stored 状态运行时提升为 8x；关闭 TP 恢复原 stored 1x。
7. TP 8x↔TP OFF 8x、TP16x↔TP OFF16x：切 TP 不应触发新的 PDC。
8. 0 ms Core 8x + Ceiling8x、Core16x + Ceiling16x：重点听瞬态、响度、切换稳定性，并观察 ASIO-Guard 是否优于两套独立 OS。
9. 特别验证用户长期使用的 **0 ms + 16x Limiter**，不能牺牲瞬态或响度优势。
10. 异倍率 8x/16x 组合：确认声音正确；本 Candidate 仍走安全独立 Ceiling path。

## 回滚

回滚基线：

```text
QQ Super Compression 1.2.28 Stable
```

冻结目录由用户 Plan B 保留，不应被本 Candidate 覆盖。
