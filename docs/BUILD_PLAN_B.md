# Offline Windows rebuild from the 1.2.8 Plan B snapshot

The formal snapshot contains `Source/`, `Dependencies/JUCE-8.0.15/`, `REPRODUCE.md`, `PLAN_B_MANIFEST.sha256`, `PLAN_B_VERIFICATION.md` and `PLAN_B_COMPLETION.json`. The dependency folder is the complete JUCE source tree used by the verified build, including its licenses and embedded VST3 SDK. No dependency download is required.

Prerequisites: Windows x64, Visual Studio 2022 C++ Build Tools with Windows SDK, and CMake 3.22 or newer. Open the x64 Native Tools Command Prompt, set the snapshot path, and keep generated files under `D:/Codex/Temp`:

```bat
set "QQSC_SNAPSHOT=D:\path\to\QQ Super Compression 1.2.8-PlanB-Release-YYYYMMDD-HHMMSS"
set "QQSC_REBUILD=D:\Codex\Temp\QQSC-1.2.8-PlanB-Rebuild"
cmake -S "%QQSC_SNAPSHOT%\Source" -B "%QQSC_REBUILD%" -G "Visual Studio 17 2022" -A x64 "-DJUCE_PATH=%QQSC_SNAPSHOT%\Dependencies\JUCE-8.0.15" -DQQSC_FETCH_JUCE=OFF -DQQSC_BUILD_DYNAMICS_CHECK=ON
cmake --build "%QQSC_REBUILD%" --config Release --target QQSuperCompression_VST3 QQSCDynamicsCheck QQSCLimiterCheck QQSCUnityCheck QQSCLoudnessCheck QQSCLimiterVSTCheck --parallel 6
```

The existing `build-unity.cmd` records the original development paths; use the explicit commands above when rebuilding a relocated backup. CMake output stays on D: and the plugin is not automatically installed.

Run the release regressions in the same shell:

```bat
"%QQSC_REBUILD%\QQSCDynamicsCheck_artefacts\Release\QQSCDynamicsCheck.exe"
"%QQSC_REBUILD%\QQSCLimiterCheck_artefacts\Release\QQSCLimiterCheck.exe" "%QQSC_REBUILD%\verification\limiter"
"%QQSC_REBUILD%\QQSCLimiterCheck_artefacts\Release\QQSCLimiterCheck.exe" "%QQSC_REBUILD%\verification\dual" dual
"%QQSC_REBUILD%\QQSCLimiterCheck_artefacts\Release\QQSCLimiterCheck.exe" "%QQSC_REBUILD%\verification\continuity" continuity
"%QQSC_REBUILD%\QQSCUnityCheck_artefacts\Release\QQSCUnityCheck.exe" "%QQSC_REBUILD%\verification\unity"
"%QQSC_REBUILD%\QQSCLoudnessCheck_artefacts\Release\QQSCLoudnessCheck.exe" "%QQSC_REBUILD%\verification\lufs"
```

The accepted Windows main VST3 binary SHA-256 is `9A5E39848AB2691AFF7F70AAF3781D6FDBE7004B90D62C3CF57B9A0327F8B7E3`. Rebuilding with another toolchain or source path can produce a different binary hash; compare behavior with the recorded regressions rather than requiring a byte-identical rebuild.

The 1.2.7 bilingual manuals under `Source/docs/manuals/` are preserved historical feature references. They do not document the 1.2.8 Single / Dual threshold-continuity change; use `Source/README.md`, `Source/RELEASE_NOTES_1.2.8.md` and `Source/VERIFICATION_1.2.8.md` for the current behavior.
