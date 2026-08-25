# QQ Super Compression 0.1.8 构建说明 / Build Guide

## 1. 项目状态 / Project status

- 产品 / Product: QQ Super Compression
- 厂商 / Vendor: Qing Audio
- 版本 / Version: 0.1.8
- 状态 / Status: Candidate / Test；尚未由用户确认 Stable / not yet user-confirmed Stable
- 构建系统 / Build system: CMake 3.22+
- 语言标准 / Language standard: C++17
- 固定框架 / Pinned framework: JUCE 8.0.15
- Windows 格式 / Windows format: x64 VST3
- macOS 格式 / macOS formats: arm64 VST3, x86_64 VST3, Universal 2 AU

本文件只描述可复现构建和验证。0.1.8 的发布文档纠正不改变 DSP、参数 ID、状态结构或二进制目标。

This file documents reproducible build and validation only. The 0.1.8 release-documentation correction does not alter DSP, parameter IDs, state schema, or binary targets.

## 2. 依赖获取 / Dependency resolution

CMake 按以下顺序寻找 JUCE：

CMake resolves JUCE in this order:

1. 如果提供有效的 `JUCE_PATH`，使用该本地 JUCE checkout。/ If a valid `JUCE_PATH` is supplied, use that local JUCE checkout.
2. 否则，当 `QQSC_FETCH_JUCE=ON` 时，从 JUCE 官方 Git 仓库获取固定标签 `8.0.15`。/ Otherwise, with `QQSC_FETCH_JUCE=ON`, fetch pinned tag `8.0.15` from the official JUCE Git repository.
3. 两者均不可用时配置失败。/ Configuration fails if neither source is available.

网络可用的标准配置 / Standard network-enabled configuration:

```powershell
cmake -S . -B <build-dir> -G "Visual Studio 17 2022" -A x64 -DQQSC_FETCH_JUCE=ON
```

已有 JUCE 的离线/固定路径配置 / Offline or fixed-path configuration with an existing JUCE checkout:

```powershell
cmake -S . -B <build-dir> -G "Visual Studio 17 2022" -A x64 -DJUCE_PATH="<path-to-JUCE>" -DQQSC_FETCH_JUCE=OFF
```

## 3. Windows x64 VST3

### 配置与编译 / Configure and build

应在 Visual Studio 2022 Developer PowerShell 或已经运行 `VsDevCmd.bat` 的环境中执行，以确保 MSVC 标准头文件、SDK 和链接工具完整可见。

Run inside Visual Studio 2022 Developer PowerShell or after `VsDevCmd.bat`, so MSVC standard headers, the Windows SDK, and linker tools are visible.

```powershell
cmake -S . -B <build-dir> -G "Visual Studio 17 2022" -A x64 -DQQSC_FETCH_JUCE=ON
cmake --build <build-dir> --config Release --target QQSuperCompression_VST3
```

预期 bundle / Expected bundle:

```text
<build-dir>/QQSuperCompression_artefacts/Release/VST3/QQ Super Compression.vst3
```

系统安装位置只应放最终 bundle / The system install location should contain only the final bundle:

```text
C:\Program Files\Common Files\VST3\QQ Super Compression.vst3
```

安装或覆盖前必须关闭 Cubase 和其他 DAW。/ Close Cubase and every other DAW before installing or overwriting the bundle.

### Windows 验证 / Windows validation

最低验证集 / Minimum validation set:

1. Release 目标成功编译。/ Release target builds successfully.
2. `QQ Super Compression.vst3` bundle 和内部模块存在。/ The bundle and internal module exist.
3. `InitDll`、`GetPluginFactory`、`ExitDll` 入口点烟雾测试通过。/ Entry-point smoke test passes.
4. Steinberg `vst3effectsvalidator.exe` 返回 0。/ Steinberg validator returns exit code 0.
5. 安装后源 bundle 与系统 bundle 的 SHA-256 一致。/ Installed and source bundle hashes match.
6. 用户在 Cubase 中完成扫描、打开界面、输入文字、缩放、自动化、声音与延迟验证。/ The user completes Cubase scan, GUI, text entry, scaling, automation, sound, and latency checks.

## 4. BS.1770 / LUFS 自测 / BS.1770 / LUFS self-test

此测试不依赖 JUCE，可独立验证 K-weighting、门限与匹配基准。

This JUCE-free test independently checks K-weighting, gating, and match reference behaviour.

使用 MSVC / With MSVC:

```powershell
cl /std:c++17 /EHsc /O2 tests\bs1770_match_selftest.cpp /Fe:<temp-output>\bs1770_match_selftest.exe
& <temp-output>\bs1770_match_selftest.exe
```

预期结果 / Expected result:

- 1 kHz 满刻度单声道约为 -3.0036 LUFS。/ A 1 kHz full-scale mono signal is approximately -3.0036 LUFS.
- 相差 6 dB 的两路测量得到 6 dB Match。/ Two measurements differing by 6 dB produce a 6 dB Match.
- 程序输出 PASS 并返回 0。/ The program prints PASS and returns 0.

## 5. macOS VST3 与 AU / macOS VST3 and AU

GitHub Actions 使用两个 VST3 runner 和一个 Universal AU job：

GitHub Actions uses two VST3 runners and one Universal AU job:

| 目标 / Target | Runner | 架构 / Architecture |
|---|---|---|
| Apple Silicon VST3 | `macos-14` | arm64 |
| Intel VST3 | `macos-15-intel` | x86_64 |
| Universal 2 AU | `macos-14` | arm64 + x86_64 |

单架构配置示例 / Single-architecture configuration example:

```bash
cmake -S . -B <build-dir> -G Xcode \
  -DQQSC_FETCH_JUCE=ON \
  -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build <build-dir> --config Release --target QQSuperCompression_VST3
```

Intel 将 `arm64` 改为 `x86_64`。AU 分别构建两个架构，再用 `lipo` 合成 `arm64 x86_64` 的 Universal 2 模块，并通过 `auval -v aufx Qscp Qing`。

For Intel, replace `arm64` with `x86_64`. The AU job builds both architectures, combines the modules with `lipo` into an `arm64 x86_64` Universal 2 binary, and runs `auval -v aufx Qscp Qing`.

预期 bundle / Expected bundles:

```text
QQ Super Compression.vst3
QQ Super Compression.component
```

预期 AU 标识 / Expected AU identity:

```text
Type: aufx
Subtype: Qscp
Manufacturer: Qing
```

## 6. 公开 CI / Public CI

工作流 / Workflows:

- `.github/workflows/build-windows-vst3.yml`
- `.github/workflows/build-macos-vst3-au.yml`

每个正式 Plan D 候选必须从同一个公开 commit 生成以下四个包：

Every formal Plan D candidate must generate these four packages from the same public commit:

```text
QQ-Super-Compression-0.1.8-Windows-x64.zip
QQ-Super-Compression-0.1.8-macOS-Apple-Silicon-VST3.zip
QQ-Super-Compression-0.1.8-macOS-Intel-VST3.zip
QQ-Super-Compression-0.1.8-macOS-Universal-AU.zip
```

内部验证应记录 commit、run/job、包名、大小、SHA-256、架构、bundle 名、版本和验证结果。/ Internal verification must record the commit, run/job, package name, size, SHA-256, architecture, bundle name, version, and validation result.

## 7. 已知警告与影响 / Known warning and impact

Windows 编译可能出现 `PluginEditor.cpp` 中 `constrainer` 名称遮蔽的 C4458 警告。它是局部名称与基类成员同名的编译期可读性警告，不改变解析到的对象、生成的二进制行为、音频处理或 UI 约束；当前为非致命。后续可通过重命名局部变量清理，但不能在未经回归验证时借机改动界面逻辑。

The Windows build may emit C4458 for a `constrainer` name shadow in `PluginEditor.cpp`. This is a compile-time readability warning caused by a local name matching a base-class member. It does not change object resolution, binary behaviour, audio processing, or UI constraints and is currently non-fatal. A future cleanup may rename the local variable, but must not alter UI behaviour without regression testing.

## 8. 行为约束 / Behavioural constraints

- 不得把 0 ms 偷偷平滑或加入隐藏 Lookahead。/ Do not secretly smooth 0 ms or add hidden lookahead.
- Lookahead 必须同时控制分析窗口、真实延迟、Bypass 延迟路径和宿主 PDC。/ Lookahead must control the analysis window, real delay, bypass delay path, and host PDC together.
- Match 必须测量延迟 Dry 与 Makeup/Mix 前的压缩 Wet。/ Match must measure delayed Dry against compressed Wet before Makeup and Mix.
- Hold 必须保持纯显示逻辑。/ Hold must remain display-only.
- 保持参数 ID、状态兼容、UTF-8 与 CJK 字体回退。/ Preserve parameter IDs, state compatibility, UTF-8, and CJK font fallback.
- 只有用户可以确认 Stable。/ Only the user can confirm Stable.
