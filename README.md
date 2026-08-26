# QQ Super Compression 0.1.10

**Qing Audio 开源动态处理器 / Open-source dynamics processor by Qing Audio**

QQ Super Compression 面向一个具体的混音问题：素材需要动态收敛，但工程师不希望传统压缩器的 Attack / Release 同时重塑原始瞬态、音头和演奏表情。

QQ Super Compression addresses a specific mixing problem: the source needs dynamic control, but the engineer does not want conventional Attack / Release behaviour to reshape its original transient, onset, or articulation.

| 项目 / Item | 内容 / Value |
|---|---|
| 当前版本 / Current version | 0.1.10 |
| 状态 / Status | Candidate / Test — 尚未由用户确认 Stable / not yet user-confirmed Stable |
| 厂商 / Vendor | Qing Audio |
| 格式 / Formats | Windows x64 VST3; macOS Apple Silicon VST3; macOS Intel VST3; macOS Universal 2 AU |
| 框架 / Framework | JUCE 8.0.15 / CMake / C++17 |
| 许可证 / License | MIT |

## 为什么设计它 / Why it exists

传统压缩器的 Attack 和 Release 不只是控制“压多少”，也会改变增益开始下降和恢复的时间。慢 Attack 可能让音头穿过，快 Attack 可能更明显地削弱音头，Release 又会改变事件之后的恢复形状。这些都是很有价值的声音设计手段，但并不适合所有任务。

Conventional compressor Attack and Release do more than determine “how much” compression occurs. They shape when gain reduction begins and recovers. A slow Attack may let the onset through, a fast Attack may reduce it more strongly, and Release changes the recovery after the event. These are valuable sound-design tools, but they are not desirable in every situation.

QQ Super Compression 的目标是：

> **在需要压缩动态范围时，尽量保留素材原本的瞬态与起音特征。**

The design target is:

> **Control dynamic range while preserving the source's original transient and onset character as much as practical.**

| 素材 / Source | 传统时间包络可能改变的部分 / What conventional timing may reshape |
|---|---|
| 吉他 / Guitar | 拨片触弦与音符前沿 / pick attack and note front edge |
| 人声 / Vocals | 辅音、爆破音与字头 / consonants, plosives, and word onsets |
| 钢琴 / Piano | 琴槌敲击和起音辨识度 / hammer strike and onset identity |
| 贝斯 / Bass | 指弹或拨片的清晰度与律动 / finger or pick articulation and groove |

这不是在否定传统压缩器，也不是声称能够在所有信号上“完美保留瞬态”。它提供的是另一种工作方向：当动态控制是必要的，而 Attack / Release 式瞬态塑形不是目标时，使用未来窗口预读来减少对传统时间包络的依赖。

This is not an argument against conventional compressors, nor a claim of perfect transient preservation on every signal. It offers another direction: when dynamic control is needed but Attack / Release transient shaping is not the goal, future-window analysis reduces dependence on a conventional time envelope.

## 工作原理 / How it works

插件先延迟可听音频，同时查看对应样本之后的一段未来波形。未来窗口给出即将到来的峰值电平，Ratio 根据该电平直接计算增益，再把增益应用到时间对齐的原始波形。

The plug-in delays the audible path while examining a future waveform window. That window estimates the upcoming peak level, Ratio derives gain directly from the estimate, and the gain is applied to the time-aligned original waveform.

```text
Future waveform window
        -> estimate the level for a delayed sample
        -> derive threshold-free Ratio gain
        -> apply gain to the corresponding delayed waveform
        -> Makeup
        -> compensated Dry/Wet Mix
        -> Output
```

Lookahead 不只是“让压缩更快”。它让处理器在对应音频到达输出前获得未来信息，从而不必依靠 Attack 在瞬态已经发生后追赶它。

Lookahead is not merely a way to make compression “faster.” It gives the processor information before the corresponding audio reaches the output, so it does not need an Attack envelope to chase a transient after it has already arrived.

### 无阈值 Ratio / Threshold-free Ratio

```text
gain = 1 / (1 + (Ratio - 1) * level)
```

`level` 是归一化为 0..1 的未来窗口峰值；在 0 ms 时则是瞬时样本幅度。Ratio 1:1 在 Makeup/Mix 前为 unity，Ratio 增大时 Gain Reduction 增加。

`level` is the future-window peak normalized to 0..1, or instantaneous sample magnitude at 0 ms. Ratio 1:1 is unity before Makeup/Mix; increasing Ratio increases Gain Reduction.

这不是传统“超过 Threshold 后 4:1”的 dB 斜率。插件没有传统 Threshold，也没有隐藏的用户 Attack / Release 包络。

This is not a conventional “4:1 above Threshold” dB slope. The plug-in has no conventional Threshold and no hidden user Attack / Release envelope.

## 快速开始 / Quick start

1. 先选 Lookahead。26/40/80/100 ms 更接近插件主要的“动态控制但减少瞬态重塑”方向；0 ms 是额外保留的非线性色彩模式。
2. 调整 Ratio，观察 Gain Reduction 和听感变化。
3. 选择 ST、LR 或 MS 工作域。
4. 播放一段有代表性的素材后使用 Match，消除响度偏差。
5. 用 Makeup 做必要修正，再用 Mix 混合补偿延迟后的 Dry 与 Wet。

1. Choose Lookahead first. The 26/40/80/100 ms settings are closer to the main “dynamic control with less transient reshaping” direction; 0 ms is an additional nonlinear colour mode.
2. Adjust Ratio while watching Gain Reduction and listening to the result.
3. Choose the ST, LR, or MS processing domain.
4. Play representative material and use Match to remove loudness bias.
5. Fine-tune Makeup, then use Mix to blend time-aligned Dry and Wet.

## 0.1.10：只在 0 ms 使用 Oversampling / 0 ms-only Oversampling

0.1.9 曾把 1x/2x/4x/8x 作为所有 Lookahead 的实验选项。PluginDoctor 与宿主测试后，0.1.10 收敛为更明确的产品逻辑：

Version 0.1.9 exposed 1x/2x/4x/8x experimentally at every Lookahead. After PluginDoctor and host testing, 0.1.10 converges on a more focused design:

- **Lookahead = 0 ms：**显示单个按钮，循环 `1x -> 8x -> 16x -> 1x`；新实例记忆值默认 **8x**。
- **Lookahead = 10/26/40/80/100 ms：**隐藏 Oversampling 控件并强制 DSP 使用 **1x**，但保留用户上一次 0 ms 选择。
- **2x 和 4x 被有意移除：**用户实测两者在 0 ms 下仍有严重混叠，而 8x/16x 增加的延迟较小，中间倍率没有足够实际价值。
- **1x：**最原始的 0 ms 色彩与最强混叠；**8x：**默认实用平衡；**16x：**进一步降低 alias fold-back。
- Oversampling 只减少 0 ms 色彩产生的折返混叠，不负责消灭该模式本身的谐波与染色。

- **Lookahead = 0 ms:** one button cycles `1x -> 8x -> 16x -> 1x`; the remembered default for a new instance is **8x**.
- **Lookahead = 10/26/40/80/100 ms:** the control is hidden and DSP is forced to **1x**, while the user's previous 0 ms choice is preserved.
- **2x and 4x are intentionally absent:** user testing found severe aliasing remained at both factors, while the additional latency of 8x/16x was small enough that the intermediate factors offered little practical value.
- **1x:** rawest 0 ms colour and strongest aliasing; **8x:** default practical balance; **16x:** further alias-foldback reduction.
- Oversampling reduces fold-back from the 0 ms colour; it is not intended to remove that mode's harmonic character.

### PDC、Dry、Mix 与 Bypass

宿主报告的总延迟始终是活动 Lookahead 加上活动 0 ms FIR Oversampling 延迟。Dry、Mix 和 Bypass 使用完全相同的整数延迟，因此保持在同一时间线上。

Host-reported latency is always active Lookahead plus active 0 ms FIR Oversampling latency. Dry, Mix, and Bypass use the identical integer delay, keeping all paths on the same timeline.

- 0 ms / 1x：无 Oversampling FIR 延迟。
- 0 ms / 8x 或 16x：报告 FIR 延迟。
- 10 ms 以上：Oversampling 实际为 1x，只报告 Lookahead 延迟。

切换 Lookahead 或活动 Oversampling 倍率会改变真实插件延迟，宿主可能进行一次 PDC 重对齐。

Changing Lookahead or the active Oversampling factor changes real plug-in latency, so the host may perform a one-time PDC realignment.

## Lookahead 的声音意义 / What Lookahead means sonically

用户 PluginDoctor 测试观察到未来窗口长度与稳定低频信号的检测稳定性存在清晰关系：5 ms 约对应 99.6 Hz、10 ms 约 49.8 Hz、20 ms 约 24.9 Hz；26 ms 把边界推到约 20 Hz，40 ms 和 80 ms 对 20 Hz 继续更干净。

User PluginDoctor testing found a clear relationship between future-window length and stable low-frequency analysis: approximately 99.6 Hz at 5 ms, 49.8 Hz at 10 ms, and 24.9 Hz at 20 ms. A 26 ms window moved the boundary to roughly 20 Hz, while 40 ms and 80 ms became progressively cleaner at 20 Hz.

批准的预设为 `0 / 10 / 26 / 40 / 80 / 100 ms`。

The approved presets are `0 / 10 / 26 / 40 / 80 / 100 ms`.

0 ms 把 detector 折叠为瞬时样本幅度，产生有意保留的非线性/谐波色彩。未来开发不得以“修复”为名偷偷加入 smoothing、隐藏 Lookahead 或补偿 EQ。

At 0 ms the detector collapses to instantaneous sample magnitude, producing deliberately retained nonlinear/harmonic colour. Future development must not silently add smoothing, hidden Lookahead, or compensating EQ in the name of a “fix.”

## 处理模式 / Processing modes

- **ST — Stereo Linked：**L/R 分析独立，较强衰减控制共同增益，一个共享 Makeup。/ L/R are analysed independently; the stronger reduction drives one common gain and one shared Makeup.
- **LR — Left/Right Independent：**L/R 独立压缩与独立 Makeup。/ L/R compression and Makeup are independent.
- **MS — Mid/Side Independent：**`M=(L+R)*0.5`、`S=(L-R)*0.5`，M/S 独立处理后解码。/ M/S are processed independently before decoding.
- 模式按钮循环 `ST -> MS -> LR -> ST`。/ The Mode button cycles `ST -> MS -> LR -> ST`.

## 严格 Integrated LUFS Match / Strict Integrated LUFS Match

Match 使用 BS.1770 / EBU R128 Integrated Loudness 结构，而不是 RMS：K-weighting、400 ms blocks、75% overlap / 100 ms hop、-70 LUFS absolute gate、-10 LU relative gate；未完成的最后一个 block 不参与计算。

Match uses a BS.1770 / EBU R128 Integrated Loudness structure rather than RMS: K-weighting, 400 ms blocks, 75% overlap / 100 ms hop, a -70 LUFS absolute gate, and a -10 LU relative gate. An incomplete final block is ignored.

Match 比较延迟 Dry 与压缩后、Makeup 前、Mix 前的 Wet，并把结果写入 Makeup。它用于公平比较，不是持续运行的 Auto Gain。

Match compares delayed Dry with compressed Wet before Makeup and Mix, then writes the result to Makeup. It exists for fair comparison and is not a continuously operating Auto Gain stage.

- ST：一个 stereo programme 结果写入共同 Makeup。
- LR：L/R 独立测量并写入。
- MS：M/S 各自按 mono gated-LUFS 数学测量并写入。
- 静音或无有效 gated data 的通道保持原 Makeup。

## A/B、界面与工作流 / A/B, UI, and workflow

- A/B 保存 Ratio、全部 Makeup、Mix、Lookahead、0 ms Oversampling 记忆值和 Mode；Bypass 保持全局。
- A→B / B→A 参数复制。
- Shift-drag 精调，Alt+左键恢复默认，Ctrl/Cmd+Z Undo，Ctrl/Cmd+Shift+Z Redo。
- Dynamic Display：灰色为延迟 Dry/Input，青色为 Makeup 前 Wet，黄色为 Makeup/Mix 后 Output。
- Input、Output、Gain Reduction 三组双通道表；ST/LR 显示 L/R，MS 显示 M/S。
- 每路 Gain Reduction 有 2 秒自动 Peak Hold，仅用于显示。
- 完整 1020x670 设计空间按固定比例统一缩放。

- A/B stores Ratio, all Makeup values, Mix, Lookahead, the remembered 0 ms Oversampling choice, and Mode; Bypass remains global.
- A→B / B→A parameter copy.
- Shift-drag fine adjustment, Alt+left-click reset, Undo, and Redo.
- Dynamic Display: grey delayed Dry/Input, cyan pre-Makeup Wet, yellow post-Makeup/post-Mix Output.
- Dual-channel Input, Output, and Gain Reduction meters; ST/LR show L/R and MS shows M/S.
- Each Gain Reduction meter has a display-only 2-second automatic Peak Hold.
- The complete 1020x670 design space scales uniformly at a fixed aspect ratio.

## 状态迁移 / State migration

- 新实例的 0 ms Oversampling 记忆值默认为 8x。
- 0.1.8 或更早工程没有 Oversampling 参数时迁移到 8x。
- 0.1.9：旧 1x -> 新 1x；旧 2x/4x/8x -> 新 8x。
- 离开 0 ms 不会覆盖记忆值；返回 0 ms 时恢复。
- A/B、Undo/Redo 和工程状态都包含该记忆值。

- New instances remember 8x as the default 0 ms Oversampling choice.
- Version 0.1.8-or-earlier states without Oversampling migrate to 8x.
- Version 0.1.9: old 1x -> new 1x; old 2x/4x/8x -> new 8x.
- Leaving 0 ms does not overwrite the remembered choice; returning restores it.
- A/B, Undo/Redo, and project state all include the remembered choice.

## 下载包 / Packages

| 包 / Package | 内容 / Contents |
|---|---|
| `QQ-Super-Compression-0.1.10-Windows-x64.zip` | Windows x64 VST3 |
| `QQ-Super-Compression-0.1.10-macOS-Apple-Silicon-VST3.zip` | macOS arm64 VST3 |
| `QQ-Super-Compression-0.1.10-macOS-Intel-VST3.zip` | macOS x86_64 VST3 |
| `QQ-Super-Compression-0.1.10-macOS-Universal-AU.zip` | macOS Universal 2 AU, arm64 + x86_64 |

macOS 包使用 ad-hoc 签名，没有 Apple Developer ID 公证。安装与 quarantine 处理见下方说明。

macOS bundles are ad-hoc signed and are not Apple Developer ID notarized. See the installation guides for installation and quarantine handling.

## 安装说明 / Installation guides

- [中文安装与使用说明](docs/QQ%20Super%20Compression%200.1.10%20Windows%E4%B8%8EmacOS%E5%AE%89%E8%A3%85%E4%BD%BF%E7%94%A8%E8%AF%B4%E6%98%8E%EF%BC%88%E4%B8%AD%E6%96%87%EF%BC%89.txt)
- [English installation and usage guide](docs/QQ-Super-Compression-0.1.10-Windows-macOS-INSTALL.txt)

## 验证状态 / Validation status

0.1.10 Windows x64 Release 已使用 JUCE 8.0.15 完成真实构建；源码 manifest、DLL/VST3 入口、x64 PE、moduleinfo、Steinberg validator 和 BS.1770 自测均通过。系统安装文件与交付文件 SHA-256 一致。

The 0.1.10 Windows x64 Release was built with JUCE 8.0.15. Source-manifest verification, DLL/VST3 entry points, x64 PE inspection, moduleinfo, Steinberg validator, and the BS.1770 self-test all passed. Installed and delivered module SHA-256 values match.

Cubase 的 0 ms 1x/8x/16x 听感、CPU、PDC、50% Mix、Bypass、自动化、状态迁移与最终用户确认仍需手动完成，因此本版本保持 Candidate/Test。

Manual Cubase validation of 0 ms 1x/8x/16x sound, CPU, PDC, 50% Mix, Bypass, automation, state migration, and final user acceptance is still required, so this release remains Candidate/Test.

## 构建 / Build

```text
cmake -S . -B build -DJUCE_PATH=/path/to/JUCE -DQQSC_FETCH_JUCE=OFF
cmake --build build --config Release --target QQSuperCompression_VST3
```

未指定本地 JUCE 时，可使用固定的 JUCE 8.0.15 FetchContent。Windows 使用 MSVC `/utf-8`；macOS 构建会同时启用 AU target。

When a local JUCE checkout is not supplied, the project can fetch pinned JUCE 8.0.15. Windows uses MSVC `/utf-8`; macOS configuration also enables the AU target.

完整构建与验证要求见 [CODEX_BUILD.md](CODEX_BUILD.md)。

See [CODEX_BUILD.md](CODEX_BUILD.md) for the complete build and validation requirements.

## 设计与开发文档 / Design and development documents

- [PRODUCT_DESIGN_NOTES.md](PRODUCT_DESIGN_NOTES.md) — 产品存在的原因、适用场景与不过度宣传边界 / why the product exists, use cases, and claim boundaries.
- [OVERSAMPLING_DESIGN_NOTES.md](OVERSAMPLING_DESIGN_NOTES.md) — 为什么只在 0 ms 使用 1x/8x/16x / why only 0 ms uses 1x/8x/16x.
- [CHANGELOG.md](CHANGELOG.md) — 完整中英双语版本记录 / complete bilingual changelog.
- [DEVELOPMENT_HISTORY.md](DEVELOPMENT_HISTORY.md) — 算法演进、失败方案和回滚路径 / algorithm evolution, rejected approaches, and rollback paths.
- [AI_DEVELOPMENT_HANDOFF.md](AI_DEVELOPMENT_HANDOFF.md) — 后续开发交接与不可破坏的设计约束 / future-development handoff and protected design constraints.
- [docs/TEST_CHECKLIST.md](docs/TEST_CHECKLIST.md) — DAW、PluginDoctor、PDC、状态与 UI 测试清单 / DAW, PluginDoctor, PDC, state, and UI checklist.

## 项目边界 / Project boundaries

- 不把传统压缩器描述成错误方案；本插件服务于不同的混音目标。
- 不声称在所有信号上完美保留瞬态。
- 不把 Ratio 描述成标准 threshold-slope Ratio。
- 不把 0 ms 描述成透明模式。
- 不在没有新测量与用户明确批准时重新加入 2x/4x、给 10 ms+ 开启 Oversampling、加入隐藏 smoothing 或补偿 EQ。
- 只有用户明确确认后，Candidate 才能标记为 Stable。

- Do not describe conventional compressors as incorrect; this plug-in serves a different mixing goal.
- Do not claim perfect transient preservation on every signal.
- Do not describe Ratio as a standard threshold-slope ratio.
- Do not describe 0 ms as a transparent mode.
- Do not restore 2x/4x, enable Oversampling at 10 ms+, add hidden smoothing, or add compensating EQ without new measurements and explicit user approval.
- A Candidate becomes Stable only after explicit user confirmation.

## License

MIT — see [LICENSE](LICENSE).
