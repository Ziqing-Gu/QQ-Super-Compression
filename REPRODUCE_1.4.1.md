# Build QQ Super Compression 1.4.1

Requirements: CMake 3.22+, C++17, JUCE 8.0.15 (pinned in CMakeLists.txt), Visual Studio 2022 C++/Windows SDK or Xcode Command Line Tools. Set JUCE_PATH to an existing checkout, or leave QQSC_FETCH_JUCE=ON. Keep build directories outside the source tree when practical.

Windows developer shell:

```sh
cmake -S . -B <build-directory> -G "Visual Studio 17 2022" -A x64 -DQQSC_FETCH_JUCE=ON -DQQSC_BUILD_DYNAMICS_CHECK=ON
cmake --build <build-directory> --config Release --target QQSuperCompression_VST3 QQSCContractCheck --parallel 3
```

macOS VST3: configure with `-DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 -DCMAKE_OSX_ARCHITECTURES=arm64` or `x86_64`, and enable QQSC_BUILD_DYNAMICS_CHECK. Build QQSuperCompression_VST3 and QQSCContractCheck. For Universal 2 AU configure a separate directory with `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` and build QQSuperCompression_AU.

The required behavior-contract target runs real processor/editor checks for unconditional Output LINK and finite thresholds. Preserve docs/BEHAVIOR_CONTRACTS.md. Mac CI also checks Display, Chinese hover help, transport, binary architecture, bundle version, ad-hoc signatures and AU validation.

Release policy: Windows is built and verified locally in Plan A and reused unchanged. Plan C builds only the missing three Mac products through the manual macOS workflow. The Windows workflow is available only for an explicitly requested cloud reproduction; do not dispatch it by default. Current Mac deployment target is 11.0. Developer ID notarization is not provided.

The editable bilingual manual sources, translation maps and production UI captures are in docs/manuals/source-1.4.1-zh and source-1.4.1-en. Do not regenerate or re-read completed frozen source backups for documentation or publication work.
