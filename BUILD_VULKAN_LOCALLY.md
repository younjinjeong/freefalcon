# Build Vulkan Renderer Locally

After struggling with GitHub Actions CI builds, we're testing locally first to validate the Vulkan renderer code compiles correctly.

## Prerequisites

- **Vulkan SDK 1.3.296.0** - Download from https://vulkan.lunarg.com/
- **Visual Studio 2019/2022** or Build Tools (for MSBuild)
- **Windows 10/11**

## Quick Start

### 1. Install Vulkan SDK

1. Download Vulkan SDK 1.3.296.0 from https://vulkan.lunarg.com/
2. Run the installer: `VulkanSDK-1.3.296.0-Installer.exe`
3. Install to default location: `C:\VulkanSDK\1.3.296.0\`
4. **Restart your command prompt** after installation

### 2. Test Your Setup

```cmd
test_local_setup.bat
```

This verifies:
- Vulkan SDK is installed
- GLM library is present
- MSBuild is available

### 3. Build

```cmd
build_vulkan.bat
```

This will:
- Build the Vulkan graphics library (Debug/Win32)
- Compile SPIR-V shaders
- Report success or provide error details

## What Gets Built

**Library**: `build\x86\debug_win32\graphics\vulkan\vulkan.lib`

**Shaders**:
- `src\graphics\vulkan\shaders\triangle_vert.spv`
- `src\graphics\vulkan\shaders\triangle_frag.spv`

## If Build Fails

Save the detailed error log:

```cmd
cd src\graphics\vulkan
"C:\Path\To\MSBuild.exe" vulkan.vcxproj -p:Configuration=Debug -p:Platform=Win32 -v:detailed > build_error.txt 2>&1
```

Then share the error log for debugging.

## Manual Build

If you prefer manual control:

```cmd
cd src\graphics\vulkan

REM Build library
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\amd64\MSBuild.exe" vulkan.vcxproj -p:Configuration=Debug -p:Platform=Win32

REM Compile shaders
cd shaders
compile_shaders.bat
```

## What's Already Fixed

All these issues are resolved in the code:

✅ Type name collisions (VkDevice → VulkanDevice, etc.)
✅ GLM include paths
✅ Windows.h conflicts
✅ Struct packing issues
✅ Preprocessor defines (VK_USE_PLATFORM_WIN32_KHR)
✅ Missing includes (std::unordered_map, std::array)

## After Success

Once the build works:
1. We know the code is correct
2. We can update CI to match your working setup
3. CI will build automatically on future commits

## Need Help?

Just let me know what errors you encounter!
