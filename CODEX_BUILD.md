# Reproduce QQ Super Compression1.2.3 Stable on Windows

Requires VS2022 C++ Build Tools/Windows SDK, CMake>=3.22 and JUCE8.0.15. Do not build inside a frozen backup. Copy to a new D-drive workspace first; use D:\Codex\Temp for build output.

In an x64 VS developer prompt, set SOURCE and BUILD to the copied source and build directory, then:

```bat
cmake -S "%SOURCE%" -B "%BUILD%" -G "Visual Studio 17 2022" -A x64 -DJUCE_PATH="%SOURCE%\_PlanB\Dependencies\JUCE-8.0.15" -DQQSC_FETCH_JUCE=OFF -DQQSC_BUILD_DYNAMICS_CHECK=ON -DQQSC_BUILD_VISUAL_CHECK=ON
cmake --build "%BUILD%" --config Release --target QQSuperCompression_VST3 QQSCDynamicsCheck QQSCVisualCheck --parallel 6
"%BUILD%\QQSCDynamicsCheck_artefacts\Release\QQSCDynamicsCheck.exe"
"%BUILD%\QQSCVisualCheck_artefacts\Release\QQSCVisualCheck.exe" "%BUILD%\Preview"
```

For an active-source copy without _PlanB, point JUCE_PATH at JUCE8.0.15, or enable QQSC_FETCH_JUCE to fetch the pinned version. Final product: QQSuperCompression_artefacts/Release/VST3/QQ Super Compression.vst3. Install the whole bundle, never just rename the comparison plugin binary.

The Plan B snapshot includes exact dependency sources/licenses, accepted Windows binary and validation evidence. This checkpoint validates Windows x64 only. Different toolchain installations may produce different binary hashes.
