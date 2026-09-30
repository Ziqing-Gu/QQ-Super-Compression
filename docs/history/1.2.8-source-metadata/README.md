# QQ Super Compression 1.2.8 — Limiter Single/Dual continuity

**Source candidate. Windows VST3 build, Cubase validation, installation and Plan B have NOT been completed for this version.**

Based solely on the user-confirmed `QQSuperCompression-1.2.7-Strict-Output-Link.zip`. That baseline includes the independent Dual Up/Down algorithms. The uploaded 1.2.7 source and verified VST3 are unchanged.

## Change / 本次修改

Limiter 下切换 Single / Dual 时，目标模式接续当前向下阈值，不再召回目标模式此前保存的旧向下阈值。Single 的 Threshold 与 Dual 的 DOWN Threshold 对应，双向切换遵循同一规则。

Makeup、Mix、Output Gain 沿用当前值，不因切换重新计算、归零或召回另一组值；Limiter LINK 开关不影响此规则。五个 ST / L / R / M / S 域分别接续各自的当前值，不把声道强制设为相同值。

**正常压缩模式保留原有的 Single / Dual 独立阈值记忆。** Normal / Limiter 两组设置、A/B 比较、Single / Dual 各自 Ratio 和算法选择均保留原有边界。没有修改压缩算法、检测器、Lookahead、Ceiling、True Peak 或 LUFS 处理。

In Limiter mode, switching Single/Dual carries the current downward threshold into the destination mode. Makeup, Mix and Output are shared already and remain unchanged, independently of all Link flags. Normal compression keeps separate Single/Dual boundary memories. Whole-state recall (A/B, project load, Undo/Redo) is distinct from a fresh mode-switch request.

Existing parameter IDs, order and schema 22 are retained for session compatibility. The legacy Single and Dual threshold parameters remain serialized; on a fresh Limiter mode switch the target's old downward threshold is not recalled. UP-gate/Range collisions keep the existing boundary-push/equality rule. No new Output compensation is introduced.

## Build on Windows

Use the original JUCE 8.0.15 checkout and Visual Studio 2022 C++ tools. This complete project source package, like the supplied workspace ZIP, does **not** include JUCE or a new plugin binary.

Double-click `BUILD_WINDOWS.cmd` if JUCE still exists at the original build path. For another location:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\BUILD_WINDOWS.ps1 -JucePath "D:\path\to\JUCE-8.0.15"
```

The helper builds from its own directory into a new `build-1.2.8-windows` directory, builds the Windows VST3 and runs the new full-JUCE regression. It never installs a plugin, executes Plan B, changes GitHub or replaces your 1.2.7 working directory. The script itself has not been executed on Windows in this environment.

## Verification

- Completed here: actual extracted core-function isolation tests with simplified APVTS/Undo/parameter test doubles; C++17 with AddressSanitizer and UndefinedBehaviorSanitizer; baseline negative control; source/identity invariants.
- Not completed here: full JUCE compilation, actual editor/Undo/A/B serialization tests, Windows VST3 build, DAW/audio validation and hardware listening. The included full-JUCE test is `QQSCLimiterCheck <directory> continuity`.
- Details and reproducible commands: `VERIFICATION_1.2.8.md`; raw logs: `Verification/1.2.8/`.

The previous build/install records and scripts are preserved under `docs/history/1.2.7-verified-baseline/`. They describe 1.2.7, not this candidate. Historical PDFs in `docs/` remain unchanged and are not updated 1.2.8 manuals. No formal Plan B is claimed.

License unchanged: Qing Audio Non-Commercial Source-Share License 1.0; see `LICENSE`.
