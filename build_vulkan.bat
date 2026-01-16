@echo off
REM Complete Vulkan Build Script
REM Builds the Vulkan library and compiles shaders

setlocal enabledelayedexpansion

echo ========================================
echo Building Vulkan Graphics Library
echo ========================================
echo.

REM Check Vulkan SDK
if "%VULKAN_SDK%"=="" (
    echo ERROR: VULKAN_SDK not set
    echo Please install Vulkan SDK and restart your command prompt
    exit /b 1
)

REM Find MSBuild
set MSBUILD_FOUND=0
set MSBUILD_PATH=

REM Try common locations
for %%P in (
    "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe"
) do (
    if exist %%P (
        set MSBUILD_PATH=%%P
        goto :found_msbuild
    )
)

echo ERROR: MSBuild not found
echo Please install Visual Studio 2019/2022
exit /b 1

:found_msbuild
echo Using MSBuild: !MSBUILD_PATH!
echo.

REM Change to vulkan directory
cd /d "%~dp0src\graphics\vulkan"
if errorlevel 1 (
    echo ERROR: Could not change to vulkan directory
    exit /b 1
)

REM Build Debug configuration
echo ========================================
echo Building Vulkan library (Debug/Win32)...
echo ========================================
echo.

!MSBUILD_PATH! vulkan.vcxproj -p:Configuration=Debug -p:Platform=Win32 -v:minimal

if errorlevel 1 (
    echo.
    echo ========================================
    echo BUILD FAILED
    echo ========================================
    echo.
    echo Save the detailed log with:
    echo !MSBUILD_PATH! vulkan.vcxproj -p:Configuration=Debug -p:Platform=Win32 -v:detailed ^> build_error.txt 2^>^&1
    echo.
    exit /b 1
)

echo.
echo ========================================
echo BUILD SUCCESSFUL
echo ========================================
echo.

REM Check output
set OUTPUT_LIB=..\..\..\build\x86\debug_win32\graphics\vulkan\vulkan.lib
if exist "%OUTPUT_LIB%" (
    echo Output library created:
    echo   %OUTPUT_LIB%
    echo.
) else (
    echo WARNING: Expected output library not found at:
    echo   %OUTPUT_LIB%
    echo.
)

REM Compile shaders
echo ========================================
echo Compiling SPIR-V Shaders...
echo ========================================
echo.

cd shaders
call compile_shaders.bat

if errorlevel 1 (
    echo.
    echo WARNING: Shader compilation failed
    echo Shaders must be compiled for runtime testing
    cd ..
    exit /b 1
)

cd ..

echo.
echo ========================================
echo ALL DONE!
echo ========================================
echo.
echo Vulkan library and shaders built successfully.
echo.
echo Next steps:
echo   1. Library: %OUTPUT_LIB%
echo   2. Shaders: src\graphics\vulkan\shaders\*.spv
echo   3. Ready for integration testing
echo.
