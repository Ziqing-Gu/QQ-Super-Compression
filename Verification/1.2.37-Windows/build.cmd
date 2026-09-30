@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64
if errorlevel 1 exit /b %errorlevel%
set "TEMP=D:\Codex\Temp\QQSC-Toolchain-Temp"
set "TMP=D:\Codex\Temp\QQSC-Toolchain-Temp"
"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build "D:\Codex\Temp\QQSC1234-Review-Fixes" --config Release --target QQSCReviewCheck QQSuperCompression_VST3 --parallel 4
exit /b %errorlevel%
