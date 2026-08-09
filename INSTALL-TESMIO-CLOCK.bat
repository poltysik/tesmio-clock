@echo off
setlocal EnableExtensions
chcp 65001 >nul

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" -Game "%~1"
set "INSTALL_RESULT=%ERRORLEVEL%"

echo.
if not "%INSTALL_RESULT%"=="0" (
    echo Installation failed. See the message above.
) else (
    echo Installation completed successfully.
)
echo.
pause
exit /b %INSTALL_RESULT%

