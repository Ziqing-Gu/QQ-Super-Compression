# QQ Super Compression 1.2.34 — Performance Architecture Candidate

## 基线

- 以 **1.2.28 Stable** 为声音回滚基线。
- 1.2.29–1.2.33 仅作为性能诊断依据，诊断开关不进入正式产品 UI。

## 1. FULL / ECO

新增全局、持久化的 FULL / ECO 性能偏好。

### FULL

保持原有后台分析行为。

### ECO

当某个实例自己的 UI 不可见时，暂停该实例的纯分析工作：

- Dynamic Display 证据采集；
- 4x True-Peak Meter；
- Meter / Hold；
- BS.1770 MATCH loudness accumulation；
- Limiter LUFS 等 UI/分析统计。

声音处理、PDC、Lookahead、Core OS、Ceiling / TP 等完全不受影响。

UI 显示状态按 `isShowing()` 周期同步，因此宿主即使保留隐藏的 editor 对象，ECO 也会进入分析休眠。

FULL / ECO 选择通过 `PropertiesFile` 作为 Qing Audio 全局偏好保存，新实例与下次启动沿用最后选择。

## 2. Ceiling OS 1x / 8x / 16x

新增 append-only 参数 `ceilingOversampling`，默认 8x 以兼容 1.2.28。

TP OFF：

- 1x Native Hard Clip
- 8x Hard Clip
- 16x Hard Clip

TP ON：

- 8x True Peak
- 16x True Peak

如果 stored Ceiling OS 为 1x，开启 TP 时 effective OS 自动提升到 8x；关闭 TP 后 stored 1x 不被改写，因此自动恢复。

同一 Ceiling OS 下切 TP 不改变 reported PDC。

## 3. 0 ms Core / Ceiling Oversampling Planner

当 0 ms Core OS 与 Ceiling OS 完全一致时：

- 8x + 8x 共用同一 8x reconstructed audio domain；
- 16x + 16x 共用同一 16x reconstructed audio domain；
- 不再让 Ceiling 重复执行第二套完整 audio up/down filter。

因此重点保护并优化用户确认的重要：

```text
Limiter + 0 ms Lookahead + 16x Core + 16x Ceiling
```

Core / Ceiling 异倍率时，本 Candidate 仍保持安全独立 Ceiling path，精确保留用户请求的 8x 或 16x 语义，不静默替换质量倍率。跨倍率 stage-sharing 需要独立验证后再合并。

## 4. PDC

- Normal 不承担 Ceiling latency。
- Ceiling 1x 不增加 Ceiling oversampling latency。
- Ceiling 8x / 16x 按实际质量报告 PDC。
- 同倍率 Core/Ceiling shared path 会去掉重复的 Ceiling audio-filter latency。
- 固定 Ceiling OS 下 TP OFF / ON PDC 相同。
- 用户主动切 1x / 8x / 16x 允许更新宿主 PDC。

## 5. UI

- Lookahead 与 0 ms OS 紧凑排在 MODE 下方。
- Limiter 显示新的 CEILING OS。
- 顶部加入 FULL / ECO。

## 暂不包含

- Zero-State Ceiling Sleep：本版优先保证有声状态与 oversampling 重构稳定，静音自动 sleep 后续单独实现。
- 异倍率 Core 8x↔Ceiling16x / Core16x↔Ceiling8x 的 stage tap 共享：本版先保留安全独立路径。
