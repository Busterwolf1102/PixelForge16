@echo off
setlocal
cd /d "%~dp0"
set "EXE=build\PixelForge16.exe"
if not exist "%EXE%" if exist build\Release\PixelForge16.exe set "EXE=build\Release\PixelForge16.exe"
if not exist "%EXE%" (
    call build.bat
    if errorlevel 1 exit /b 1
    set "EXE=build\PixelForge16.exe"
    if not exist "%EXE%" if exist build\Release\PixelForge16.exe set "EXE=build\Release\PixelForge16.exe"
)
start "" "%EXE%"
