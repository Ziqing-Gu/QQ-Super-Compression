@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
cmake -S "D:\Codex\Workspaces\QQSC-1.2.42-Full-Idle-Candidate" -B "D:\Codex\Temp\QQSC1242-Full-Idle-Build" -G "Visual Studio 17 2022" -A x64 -DJUCE_PATH="D:\Codex\Temp\JUCE-8.0.15-src\JUCE-8.0.15" -DQQSC_FETCH_JUCE=OFF -DQQSC_BUILD_DYNAMICS_CHECK=ON
if errorlevel 1 exit /b %errorlevel%
cmake --build "D:\Codex\Temp\QQSC1242-Full-Idle-Build" --target QQSCReviewCheck QQSuperCompression_VST3 QQSCSilenceVSTCheck --config Release --parallel 3
exit /b %errorlevel%
