# Reproduce the Windows1.2.2 build

Do not build inside the frozen Plan B snapshot. Use a fresh working copy under
`D:\Codex\Workspaces` and a build directory under `D:\Codex\Temp`.
The snapshot contains `_PlanB/Dependencies/JUCE-8.0.15` with the exact pinned
dependency source and original licenses, plus accepted Win bundle and evidence.

Requires Visual Studio2022 C++ Build Tools/Windows SDK and CMake3.22 or newer.
From a VS2022 x64 Native Tools shell, replace the example copied-source path:

```powershell
$sourceRoot = 'D:\Codex\Workspaces\QQSC122-Reproduce'
$buildRoot = 'D:\Codex\Temp\QQSC122-Reproduce'
$juceSource = Join-Path $sourceRoot '_PlanB\Dependencies\JUCE-8.0.15'
cmake -S $sourceRoot -B $buildRoot -G 'Visual Studio 17 2022' -A x64 "-DJUCE_PATH=$juceSource" -DQQSC_FETCH_JUCE=OFF -DQQSC_BUILD_DYNAMICS_CHECK=ON -DQQSC_BUILD_VISUAL_CHECK=ON
cmake --build $buildRoot --config Release --target QQSuperCompression_VST3 QQSCDynamicsCheck QQSCVisualCheck --parallel 6
& "$buildRoot\QQSCDynamicsCheck_artefacts\Release\QQSCDynamicsCheck.exe"
& "$buildRoot\QQSCVisualCheck_artefacts\Release\QQSCVisualCheck.exe" "$buildRoot\Preview"
```

13 applicable Python selftests were validated. Historical dark_skin_contract_selftest.py
and warm_skin_contract_selftest.py are excluded source-string tests already known to
fail on untouched1.2.0; real production UI geometry/material tests replace them.
Use Python3 with UTF-8 and keep generated logs/previews in the D-drive build directory.
The current machine's original successful build wrapper is included under `_PlanB/BuildScripts`.
Portable commands above avoid relying on its fixed local source paths.

Original binary identities and hashes are recorded in STABLE_1.2.2.md and external
Plan B proof. Different toolchain installations may produce different binary bytes;
do not infer a failed semantic reproduction from that alone.
