# VKE - Vulkan Engine
Multithreaded graphics engine based on Vulkan API with DirectX support! :)

## Quick start
Engine uses CMake as a build system. To build the engine, you need to have CMake and a C++ compiler installed on your system.
[CMake Download](https://cmake.org/download/)
[Compiler Download](https://visualstudio.microsoft.com/downloads/) (for Windows, Visual Studio is recommended)

### Prerequisites
- CMake 3.16 or higher
- C++20 compatible compiler
- Git

### Clone the repository
To clone the repository with all submodules, use the following command:
```bash
git clone https://github.com/przemyslaw-szymanski/vke.git ./vke --recurse-submodules
```

### vcpkg
Project use vcpkg to handle third-party dependencies. You can use it asn submodule or install it separately.
[VCPKG download](https://github.com/microsoft/vcpkg)

#### System wide installation
```bash
git clone https://github.com/microsoft/vcpkg.git c:\vcpkg
cd c:\vcpkg
.\bootstrap-vcpkg.bat
.\vcpkg integrate install
```

#### vcpkg as submodule
To use vcpkg as a submodule, clone the repository with `--recurse-submodules` or just initialize and update the submodules:
```bash
git submodule update --init --recursive
```

### Build steps
Configure the project using one of predefined scripts for your platform:
- Windows: `Run-Windows.bat`
- Linux: `Run-Linux.sh`

On Windows, this will create a `solution` directory with a Visual Studio solution file `vkEngine.sln` inside. Open this file with Visual Studio to build and run the project. `Run-Windows.bat` will try to open solution automatically.`

Or you can also create a build directory manually or via cmake GUI. Decide which API you want to use with one of the options:
- `-DVKE_VULKAN_RENDER_SYSTEM=ON` to use Vulkan API
- `-DVKE_D3D12_RENDER_SYSTEM=ON` to use DirectX API

```bash
mkdir solution
cd solution
cmake ../ -DVKE_VULKAN_RENDER_SYSTEM=ON
```

### Visual Studio Solution in folder view
If you prefer to use folder view in Visual Studio instead of solution explorer, you can open the `solution` folder directly in Visual Studio.
CMake generation will perform automatically and will be handled by Visual Studio.
There are some advantages of using folder view:
- No need to regenerate solution files when CMakeLists.txt files are changed.
- Easier navigation through the project files.
- Vulkan/D3D12 render system can be selected from configuration (VK-x64-Debug/Release, D3D12-Debug/Release)

## Development guidelines
- Currently project uses C++20 standard.
- Pull requests should have meaningfull descriptions.
- When adding new third-party libraries, please use vcpkg to manage dependencies.

## Clang-Format
The project uses modern `clang-format` to maintain consistent code style. A `.clang-format` file is provided in the root directory of the project.
Visual Studio uses old version, so to avoid issues, please download the latest version of clang-format from LLVM releases (eg.: 21.x.x):
[LLVM Releases](https://releases.llvm.org/download.html)
After downloading, you can set up Visual Studio to use the downloaded `clang-format.exe`:
1. Open Visual Studio and go to `Tools` > `Options`.
2. Navigate to `Text Editor` > `C/C++` > `Formatting` > `General`.
3. Check the box for `Enable ClangFormat support`.
4. Set the path to the downloaded `clang-format.exe` in the `ClangFormat Path` field.

## Troubleshooting
### vcpkg long path issues
If you encounter problems with building like:
error: building llvm:x64-windows failed with: BUILD_FAILED

This may possible be caused because of long paths on Windows. Enabling long paths may not be enough as theres still some limit applied.
To fix this issue, you can try to clone the repository in a directory with a shorter path, for example directly on C: drive or just have a system wide vcpkg under C:\vcpkg.

### No Vulkan validation layer
If you are running the engine with Vulkan and you see a warning about missing validation layers. There are few possible solutions:
1. If you are using vcpkg you have to manually set the path to the validation layers. You can do this by setting the `VK_ADD_LAYER_PATH` environment variable to point to the directory where vcpkg installs the Vulkan layers. For example:
    - The directory is located at your CMake build folder under `vcpkg_installed\<triplet>\bin`.
	- After setting the environment variable, restart your IDE / terminal / CMake GUI to ensure the changes take effect.
2. Alternatively install Vulkan SDK from LunarG: [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.