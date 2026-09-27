@echo off
setlocal
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%
set "QQSC_CMAKE=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
if not defined QQSC_BUILD_PATH set "QQSC_BUILD_PATH=D:\Codex\Temp\QQSC-1.2.7-Stable-Reproduce"
if not defined QQSC_JUCE_PATH set "QQSC_JUCE_PATH=%~dp0..\Dependencies\JUCE-8.0.15"
if not exist "%QQSC_JUCE_PATH%\CMakeLists.txt" set "QQSC_JUCE_PATH=D:\Codex\Temp\JUCE-8.0.15-src\JUCE-8.0.15"
"%QQSC_CMAKE%" -S "%~dp0." -B "%QQSC_BUILD_PATH%" -G "Visual Studio 17 2022" -A x64 "-DJUCE_PATH=%QQSC_JUCE_PATH%" -DQQSC_FETCH_JUCE=OFF -DQQSC_BUILD_DYNAMICS_CHECK=ON
if errorlevel 1 exit /b %errorlevel%
"%QQSC_CMAKE%" --build "%QQSC_BUILD_PATH%" --config Release --target QQSuperCompression_VST3 QQSCUnityCheck QQSCLimiterCheck --parallel 6
exit /b %errorlevel%
