@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0BUILD_WINDOWS.ps1" %*
set "QQSC_RESULT=%ERRORLEVEL%"
echo.
echo Build helper exit code: %QQSC_RESULT%
pause
exit /b %QQSC_RESULT%
