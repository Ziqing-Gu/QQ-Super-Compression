# Building QQ Super Compression 1.4.20

Use CMake 3.22+, C++17, JUCE 8.0.15 (pinned by CMake), Visual Studio 2022 C++ Build Tools and Windows SDK, or Xcode/Apple Clang for macOS. Start Windows commands from an x64 VS developer prompt. Build outside the source checkout; never use a frozen backup as a working tree.

```sh
cmake -S . -B build -DQQSC_FETCH_JUCE=ON -DQQSC_BUILD_DYNAMICS_CHECK=ON -DQQSC_BUILD_VISUAL_CHECK=ON
cmake --build build --config Release --target QQSuperCompression_VST3 QQSCContractCheck --parallel 3
```

Use `-DJUCE_PATH=<JUCE-8.0.15-directory> -DQQSC_FETCH_JUCE=OFF` for an existing dependency checkout. For macOS add `-DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_OSX_ARCHITECTURES=arm64` or `x86_64`; build AU with `QQSuperCompression_AU` and `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.

Windows VST3 is under `QQSuperCompression_artefacts/Release/VST3`; AU is under the corresponding `AU` directory. Copy complete bundles only. Chinese text is UTF-8 with platform font fallback.

For this release, reuse the already validated local Windows 1.4.20 bundle. Do not run Windows Actions by default. The existing macOS workflow creates arm64 VST3, x86_64 VST3 and Universal 2 AU, checks the current behavior contracts/Display/tooltips/transport, ad-hoc signs packages and runs auval. No Apple notarization is claimed. Package files must come from the exact public source commit; later documentation-only commits do not require rebuilding unchanged binaries.
