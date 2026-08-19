@echo off
setlocal
cd /d "%~dp0"

set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if exist "%VCVARS%" (
    call "%VCVARS%" >nul
)

where cmake >nul 2>nul
if %errorlevel%==0 (
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    if errorlevel 1 exit /b 1
    cmake --build build --config Release
    if errorlevel 1 exit /b 1
    if exist "build\Release\PixelForge16.exe" (
        set "EXE=build\Release\PixelForge16.exe"
    ) else (
        set "EXE=build\PixelForge16.exe"
    )
) else (
    where cl >nul 2>nul
    if errorlevel 1 (
        echo MSVC cl.exe was not found. Install Visual Studio 2022 Build Tools or put CMake/MSVC on PATH.
        exit /b 1
    )
    if not exist build mkdir build
    cl /nologo /std:c++20 /utf-8 /EHsc /O2 /W4 /DWIN32_LEAN_AND_MEAN /DNOMINMAX /Fo:build\ src\main.cpp /Febuild\PixelForge16.exe /link user32.lib gdi32.lib comdlg32.lib shell32.lib /SUBSYSTEM:WINDOWS
    if errorlevel 1 exit /b 1
    set "EXE=build\PixelForge16.exe"
)

if not exist dist mkdir dist
if not exist dist\PixelForge16 mkdir dist\PixelForge16
if not exist dist\PixelForge16\samples mkdir dist\PixelForge16\samples
if not exist dist\PixelForge16\screenshots mkdir dist\PixelForge16\screenshots
copy "%EXE%" dist\PixelForge16\PixelForge16.exe >nul
copy README.md dist\PixelForge16\README.txt >nul
copy docs\tutorial_ko.md dist\PixelForge16\TUTORIAL_KO.txt >nul
copy samples\* dist\PixelForge16\samples\ >nul 2>nul

echo Built dist\PixelForge16\PixelForge16.exe
