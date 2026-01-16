@echo off
REM Compile GLSL shaders to SPIR-V bytecode
REM Requires Vulkan SDK to be installed

echo Compiling Vulkan shaders...

if "%VULKAN_SDK%"=="" (
    echo ERROR: VULKAN_SDK environment variable not set
    echo Please install Vulkan SDK from https://vulkan.lunarg.com/
    exit /b 1
)

set GLSLC=%VULKAN_SDK%\Bin\glslc.exe

if not exist "%GLSLC%" (
    echo ERROR: glslc.exe not found at %GLSLC%
    exit /b 1
)

REM Compile vertex shader
echo Compiling triangle.vert...
"%GLSLC%" triangle.vert -o triangle_vert.spv
if errorlevel 1 (
    echo ERROR: Failed to compile vertex shader
    exit /b 1
)

REM Compile fragment shader
echo Compiling triangle.frag...
"%GLSLC%" triangle.frag -o triangle_frag.spv
if errorlevel 1 (
    echo ERROR: Failed to compile fragment shader
    exit /b 1
)

echo.
echo Shader compilation successful!
echo Output files:
echo   - triangle_vert.spv
echo   - triangle_frag.spv
echo.
