@echo off
setlocal enabledelayedexpansion

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

if not exist "%VSWHERE%" (
    echo Error: vswhere.exe not found. Make sure Visual Studio is installed.
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
    set "VS_PATH=%%i"
)

set "VCVARSALL=%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat"

if not exist "%VCVARSALL%" (
    echo Error: No vcvarsall.bat found in: %VCVARSALL%
    exit /b 1
)

echo Found: "%VCVARSALL%"

call "%VCVARSALL%" x64

call cmake-gui.exe