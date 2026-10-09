# Reproduce 1.3.2 Stable

Use JUCE 8.0.15, CMake 3.22+, VS 2022 C++/Windows SDK on Windows or Xcode tools on macOS. Use a fresh build directory. The macOS workflow defines architectures and the 11.0 deployment target. In a VS developer shell:

```powershell
cmake -S . -B build-1.3.2 -G "Visual Studio 17 2022" -A x64 -DQQSC_FETCH_JUCE=ON -DQQSC_BUILD_DYNAMICS_CHECK=ON -DQQSC_BUILD_VISUAL_CHECK=ON
cmake --build build-1.3.2 --config Release --target QQSuperCompression_VST3 QQSCLimiterCheck QQSCReviewCheck QQSCVisualCheck --parallel 3
```

Current profiles: `QQSCLimiterCheck <output> distort`, `lookahead-presets`, `overall-os`; `QQSCVisualCheck <output> --detector-zero-display`; `QQSCReviewCheck <output> --transport-1241`, `--full-idle-1242`. Historical profiles including detector-window and split oversampling are retained for context, not the current acceptance contract.

Verify both the Windows binary ProductVersion and moduleinfo.json show 1.3.2. Do not overwrite a loaded plugin. The release reuses its previously verified Windows binary; production input hashes are in `Verification/1.3.2/release-source-identity.json`. Mac builds must use the tagged source. No hidden Attack/Release or main-output transport crossfade is added.
