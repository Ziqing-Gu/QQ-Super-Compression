# QQ Super Compression 1.2.0 — Single / Upward / Dual

2026-09-10 · Rev4 Stable · Qing Audio

## 中文

1.2.0 在原有 Lookahead 动态处理方式上加入向上与双向处理。它继续以类似手动画音量自动化的方式控制动态，没有传统 Attack / Release 控件。版本范围为 1.1.9 → 1.2.0，无遗漏的正式版本。

- **Range 边界修复（Rev4）**：有限 Range 内侧使用连续过渡，让动态增益在到达 Range 前回到 0 dB，修复原先边界硬跳产生的爆裂声。有限上压门槛内侧也加入连续过渡；Range OFF 保留原有向下处理曲线。这是随检测电平变化的静态增益曲线，没有新增 Attack / Release、检测窗口或延迟。
- **Dual 独立 ON/OFF（Rev4）**：Up / Down Ratio 旁各有小开关，支持 ST / LR / MS。可独立试听上压或下压，保留 Ratio、阈值和 LINK；关闭一支不会扩大另一支的作用范围。每次开关使用 10 ms Crossfade，快速反向切换从当前淡变位置继续。Display 随对应开关更新。开关默认 ON，可自动化，并随工程和 A/B 保存；旧工程缺失的开关默认为 ON。
- **Single / Dual**：Ratio 旁的小按钮切换单压和双压；双压提供独立 Up Ratio / Down Ratio。三种主题统一主旋钮的视觉尺寸和间距，并保留 3:2 可缩放界面。
- **Single 范围**：只处理高于 Threshold、低于有限 Range 的检测电平；达到或超过 Range，动态增益恢复 0 dB。Range 默认 OFF，表示没有上截止，有限 0 dB 与 OFF 不同。
- **向上处理**：提升门槛以上的部分。门槛以下保留原音量，不静音，也不是提升阈值以下的声音。
- **Dual 交接**：UP 以下或等于 UP 时不作动态处理；UP 与 DOWN 之间由 Up Ratio 提升；达到 DOWN 时动态增益为 0 dB；超过 DOWN 后只由 Down Ratio 处理，不叠加上压。Dual 不设置 Range。
- **范围与默认值**：Single Ratio 为 1/32–32，Up 为 1/32–1，Down 为 1–32，全部默认 1:1。Dual 默认 UP=-inf、DOWN=0 dB。边界相撞会推着另一根移动，重合时动态处理停止。
- **Ratio LINK**：默认开启并记住上次选择，已保存工程的状态优先。Up / Down 按相反倍数联动，保留原有相对关系；例如 Down=4、Up=1/2 时，把 Down 调到 8，Up 变为 1/4。开关本身不改变数值，到共同范围边界时一起停止。它与 LR / MS 的声道 LINK 分开工作。
- **Display**：加入含 Mix 的 Boost / Cut 和正负动态增益，保留 480 点历史及参数变化后的历史更新。
- **新版手册**：中文与英文各 23 页，沿用原有配色、图案与文字风格，包含上压、双压、LINK、参数速查和排错。沿用已完成的两本手册原文件；Rev4 的边界修复与独立开关补充见本更新说明。

升级时会保留已有工程保存的 Ratio 数值。但旧版 Single Ratio 的归一化自动化映射因范围扩展而改变，请保留旧工程副本并核对自动化。有限边界保持上述处理范围；0 ms 的染色/过采样选项保留。多段版不在此次更新范围内。

## English

1.2.0 adds upward and dual processing to the existing lookahead dynamics approach. It continues to control dynamics in a way inspired by hand-drawn volume automation, without conventional Attack / Release controls. Release coverage is 1.1.9 → 1.2.0; no intervening official version is omitted.

- **Range boundary fix (Rev4):** a continuous transition inside finite Range brings dynamic gain back to 0 dB before the boundary, removing the previous hard gain step responsible for crackling. A finite upward gate also has an inner transition. Range OFF keeps the original downward curve. These are static detector-level gain curves, with no added Attack / Release, detector window or latency.
- **Independent Dual ON/OFF (Rev4):** compact switches beside Up / Down Ratio work in ST / LR / MS. Audition either branch while retaining Ratios, thresholds and LINK; disabling one does not expand the other branch's interval. Each toggle uses a 10 ms crossfade, and rapid reversals continue from the current fade position. The Display follows the branch switches. They default ON, are automatable, save with projects and A/B, and default ON when missing from older projects.
- **Single / Dual:** a compact switch beside Ratio selects the mode. Dual provides separate Up Ratio and Down Ratio controls. All three themes share consistent primary knob sizes and spacing within the resizable 3:2 editor.
- **Single interval:** only detector levels above Threshold and below a finite Range are processed. At or above Range, dynamic gain returns to 0 dB. Range defaults to OFF, with no upper cutoff; finite 0 dB is a separate setting.
- **Upward processing:** boosts eligible material above the lower gate. Below it, the original level remains; audio is not muted and below-threshold material is not boosted.
- **Dual handoff:** at or below UP there is no dynamic processing; between UP and DOWN, Up Ratio boosts; at DOWN dynamic gain is 0 dB; above DOWN only Down Ratio applies. Upward and downward gains never stack. Dual has no Range control.
- **Ranges and defaults:** Single Ratio spans 1/32–32, Up 1/32–1 and Down 1–32. All default to 1:1. Dual starts at UP=-inf and DOWN=0 dB. Faders push their partner on collision; coincident boundaries disable dynamic processing.
- **Ratio LINK:** initially on, remembers the last choice, with saved project state taking priority. Up / Down change in opposite proportions while retaining their existing relationship: Down=4 and Up=1/2 become Down=8 and Up=1/4. Toggling does not change values, and shared range limits stop both. This is separate from LR / MS channel LINK.
- **Display:** adds Mix-aware Boost / Cut and signed dynamic gain while retaining the 480-point history and history updates after parameter changes.
- **New manuals:** Chinese and English editions each have 23 pages in the established visual and writing style, covering upward / Dual operation, LINK, parameter reference and troubleshooting. The completed PDF editions are retained unchanged; this release note supplements them with the Rev4 boundary fixes and branch switches.

Saved Ratio values are retained when upgrading. However, extending Single Ratio's range changes the mapping of older normalized automation. Keep a copy of existing projects and check their automation. Finite boundaries retain the processing intervals described above, and the 0 ms coloring / oversampling option remains. The multiband edition is outside this release.

## 成品与许可证 / Packages and license

Windows x64 VST3 reuses the verified 1.2.0 Rev4 build. Mac VST3 arm64 / x86_64 and Universal 2 AU are built from the release tag. The Release contains four platform ZIPs, two installation guides and both 23-page manuals. Build results, measured platform requirements and asset hashes are recorded on the Release page.

Windows 沿用已验证的 Rev4 成品；Mac 两种 VST3 与 Universal 2 AU 从版本标签构建。Release 共 8 个附件：4 个平台 ZIP、2 份安装说明、2 本新版手册。构建结果、实际系统要求和附件哈希见 Release 页面。

Qing Audio Non-Commercial Source-Share License 1.0 remains unchanged. Commercial use is prohibited. Complete corresponding source and license: [v1.2.0 source](https://github.com/Ziqing-Gu/QQ-Super-Compression/tree/v1.2.0), [LICENSE](https://github.com/Ziqing-Gu/QQ-Super-Compression/blob/v1.2.0/LICENSE).
