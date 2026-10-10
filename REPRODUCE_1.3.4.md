# Reproduce QQ Super Compression 1.3.4 Stable

Dependencies: CMake 3.22+, C++17, JUCE 8.0.15 (pinned FetchContent), Visual Studio 2022 C++ tools and Windows SDK on Windows; Xcode command-line tools on macOS. macOS deployment target: 11.0. Plugin formats: Windows x64 VST3, macOS arm64 and x86_64 VST3, Universal 2 AU.

## Windows (explicit reproduction only)

From a VS2022 developer shell:

```powershell
cmake -S . -B build-win -G "Visual Studio 17 2022" -A x64 -DQQSC_FETCH_JUCE=ON -DQQSC_BUILD_DYNAMICS_CHECK=ON -DQQSC_BUILD_VISUAL_CHECK=ON
cmake --build build-win --config Release --target QQSuperCompression_VST3 QQSCLimiterCheck QQSCReviewCheck QQSCVisualCheck --parallel 3
```

The official Windows delivery reuses the locally accepted Plan A binary. Plan C copies that verified output; it does not rebuild Windows. The manual Windows workflow is retained only for explicitly requested cloud reproduction. Toolchain changes may change binary hashes.

## macOS

```sh
cmake -S . -B build-mac -DQQSC_FETCH_JUCE=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_OSX_ARCHITECTURES=arm64 -DQQSC_BUILD_DYNAMICS_CHECK=ON -DQQSC_BUILD_VISUAL_CHECK=ON
cmake --build build-mac --target QQSuperCompression_VST3 QQSCLimiterCheck QQSCReviewCheck QQSCVisualCheck --parallel 3
```

Use x86_64 for Intel VST3. For Universal 2 AU use a separate build directory, `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`, and target `QQSuperCompression_AU`. The manual macOS workflow builds those three products from one commit. It checks architecture, bundle version and ad-hoc signing; AU is checked with `auval -v aufx Qscp Qing`. Ad-hoc signing is not Apple Developer ID notarization.

Current targeted limiter checks: `limiter-45`, `shared-controls`, `cumulative-match`, `window-link`, `lookahead-presets`, `overall-os` (pass an output directory then the profile to QQSCLimiterCheck). QQSCVisualCheck uses `--detector-zero-display`; QQSCReviewCheck uses `--transport-1241` and `--full-idle-1242`. Historical Distort UI tests are superseded by Window checks. Automated checks do not replace auditioning in a user's DAW.

See SOURCE_IDENTITY_1.3.4.json for accepted production-file hashes and Windows identity. Frozen source backups are not build inputs and are not reopened by later release stages.

## Manuals

The English and approved Chinese 44-page PDFs are under docs/manuals. Their ReportLab scripts take `--output`, `--captures`, `--theme-images` and `--fonts`. Use source-1.3.4-zh/assets for both image arguments. Dependencies: Python, ReportLab, Pillow and pypdf; fonts: Segoe UI and Microsoft YaHei. Preserve licensed fonts locally; font files are not redistributed here.
