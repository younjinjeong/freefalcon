@echo off
REM Local Vulkan Build Test Script
REM Run this script after installing Vulkan SDK to verify your setup

echo ========================================
echo Vulkan Local Build Setup Verification
echo ========================================
echo.

REM Step 1: Check Vulkan SDK
echo [1/4] Checking Vulkan SDK installation...
if "%VULKAN_SDK%"=="" (
    echo [FAIL] VULKAN_SDK environment variable not set
    echo.
    echo Please install Vulkan SDK 1.3.296.0 from:
    echo https://vulkan.lunarg.com/
    echo.
    echo After installation, restart your command prompt and try again.
    exit /b 1
)
echo [OK] VULKAN_SDK = %VULKAN_SDK%

REM Verify Vulkan headers
if not exist "%VULKAN_SDK%\Include\vulkan\vulkan.h" (
    echo [FAIL] Vulkan headers not found
    echo Expected at: %VULKAN_SDK%\Include\vulkan\vulkan.h
    exit /b 1
)
echo [OK] Vulkan headers found

REM Verify Vulkan library
if not exist "%VULKAN_SDK%\Lib32\vulkan-1.lib" (
    echo [FAIL] Vulkan library not found
    echo Expected at: %VULKAN_SDK%\Lib32\vulkan-1.lib
    exit /b 1
)
echo [OK] Vulkan library found
echo.

REM Step 2: Check GLM
echo [2/4] Checking GLM library...
if not exist "src\extlibs\glm\glm.hpp" (
    echo [FAIL] GLM library not found
    echo Expected at: src\extlibs\glm\glm.hpp
    exit /b 1
)
echo [OK] GLM library found
echo.

REM Step 3: Find MSBuild
echo [3/4] Searching for MSBuild...

set MSBUILD_FOUND=0

REM Try Visual Studio 2022 Community
set MSBUILD_PATH="C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
if exist %MSBUILD_PATH% (
    echo [OK] Found MSBuild: Visual Studio 2022 Community
    goto :msbuild_found
)

REM Try Visual Studio 2022 Professional
set MSBUILD_PATH="C:\Program Files\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\amd64\MSBuild.exe"
if exist %MSBUILD_PATH% (
    echo [OK] Found MSBuild: Visual Studio 2022 Professional
    goto :msbuild_found
)

REM Try Visual Studio 2019 Community
set MSBUILD_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
if exist %MSBUILD_PATH% (
    echo [OK] Found MSBuild: Visual Studio 2019 Community
    goto :msbuild_found
)

REM Try Build Tools 2019
set MSBUILD_PATH="C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe"
if exist %MSBUILD_PATH% (
    echo [OK] Found MSBuild: Build Tools 2019
    goto :msbuild_found
)

echo [FAIL] MSBuild not found in common locations
echo.
echo Please install Visual Studio 2019/2022 or Build Tools from:
echo https://visualstudio.microsoft.com/downloads/
echo.
echo Or use PowerShell to find MSBuild:
echo Get-ChildItem "C:\Program Files*" -Recurse -Filter MSBuild.exe -ErrorAction SilentlyContinue ^| Where-Object { $_.FullName -match "amd64" } ^| Select-Object -First 1 -ExpandProperty FullName
exit /b 1

:msbuild_found
echo MSBuild path: %MSBUILD_PATH%
echo.

REM Step 4: Test MSBuild version
echo [4/4] Testing MSBuild...
%MSBUILD_PATH% -version >nul 2>&1
if errorlevel 1 (
    echo [FAIL] MSBuild test failed
    exit /b 1
)
echo [OK] MSBuild is working
echo.

REM All checks passed
echo ========================================
echo ALL CHECKS PASSED!
echo ========================================
echo.
echo Your system is ready to build the Vulkan library.
echo.
echo To build, run these commands:
echo.
echo   cd src\graphics\vulkan
echo   %MSBUILD_PATH% vulkan.vcxproj -p:Configuration=Debug -p:Platform=Win32
echo.
echo To compile shaders:
echo   cd shaders
echo   compile_shaders.bat
echo.
echo Or run the full build script: build_vulkan.bat
echo.
