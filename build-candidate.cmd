@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%
set "QQSC_CMAKE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
"%QQSC_CMAKE%" -S "%~dp0." -B "D:\Codex\Temp\QQSC-1.2.5-ThresholdFloor" -G "Visual Studio 17 2022" -A x64 "-DJUCE_PATH=D:\Codex\Temp\JUCE-8.0.15-src\JUCE-8.0.15" -DQQSC_FETCH_JUCE=OFF -DQQSC_BUILD_DYNAMICS_CHECK=ON -DQQSC_BUILD_VISUAL_CHECK=ON
if errorlevel 1 exit /b %errorlevel%
"%QQSC_CMAKE%" --build "D:\Codex\Temp\QQSC-1.2.5-ThresholdFloor" --config Release --target QQSuperCompression_VST3 --parallel 6
exit /b %errorlevel%
