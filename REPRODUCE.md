# Reproduce 1.2.8

Requires Windows x64, Visual Studio 2022 C++ Build Tools and Windows SDK, CMake 3.22+, and JUCE 8.0.15. The exact JUCE source and licenses are included alongside Source in the formal Plan B snapshot.

Copy Source into a new D-drive workspace before rebuilding a frozen Plan B snapshot. Run `BUILD_WINDOWS.ps1 -JucePath <snapshot>/Dependencies/JUCE-8.0.15 -BuildDirectory D:/Codex/Temp/<new-build-dir>`. The helper configures the 1.2.8 source, builds the Windows VST3 and runs the real JUCE Limiter continuity regression.

For broader local verification, build and run `QQSCLimiterCheck`, `QQSCUnityCheck` and `QQSCLoudnessCheck` with output directories on D:. The 1.2.8 Plan A record is `PLAN_A_WINDOWS_1.2.8.json`. The actual 1.2.7-to-1.2.8 VST3 comparison is recorded in the external Plan A evidence; historical binaries are not required to compile the release.

The accepted Windows binary was built in `D:/Codex/Temp/QQSuperCompression-1.2.8-Limiter-Mode-Continuity`. Toolchain changes may produce different binary hashes. macOS packages are built from the tagged public commit by the versioned GitHub Actions workflow.

Historical manuals and screenshots are retained as revision sources. The 1.2.7 bilingual manuals describe the established feature set but do not include the 1.2.8 continuity fix; README and `RELEASE_NOTES_1.2.8.md` are the current supplement.
