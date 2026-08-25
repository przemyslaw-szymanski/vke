cmake_minimum_required(VERSION 3.10...3.31)

# Try to find vcpkg in PATH if not already set.
# This allows using a globally installed vcpkg instead of a submodule.
# Submodule is easy to use for new users, but a global installation is often preferred for faster builds.
if(NOT VCPKG_EXECUTABLE)
    find_program(VCPKG_EXECUTABLE vcpkg)
endif()

if(VCPKG_EXECUTABLE)
    # Get the parent directory of the vcpkg executable
    get_filename_component(VCPKG_ROOT "${VCPKG_EXECUTABLE}" DIRECTORY)
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    message(STATUS "[VCPKG]: Using available exe: ${VCPKG_EXECUTABLE}")
    message(STATUS "[VCPKG]: Root directory: ${VCPKG_ROOT}")
else()
    # Check if vcpkg submodule exists
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake")
        message(STATUS "[VCPKG]: Using submodule: ${CMAKE_SOURCE_DIR}/src/ThirdParty/vcpkg")
        set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/src/ThirdParty/vcpkg")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/src/ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
    else()
        message(FATAL_ERROR "vcpkg not found. Please ensure vcpkg is in PATH or the submodule is initialized.")
    endif()
endif()


project(vkEngine C CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(APIS VULKAN D3D12)

add_definitions(
    -DVKE_VULKAN=1
    -DVKE_D3D12=2
    -DVKE_METAL=3)

set(VKE_CONFIGURATION_LIST)
set_property(GLOBAL PROPERTY VKE_CONFIGURATION_LIST ${VKE_CONFIGURATION_LIST})

function(GenOutputName buildType api compileFlags engineFlags out)
    message(STATUS "GenOutputName: ${buildType}, ${api}, ${compileFlags}, ${engineFlags}")
    set(world "")
    set(physics "")
    set(ai "")
    set(renderSystem "")
    set(autdio "")

    if(${api} STREQUAL "VULKAN")
        set(api "VK")
    elseif(${api} STREQUAL "D3D12")
        set(api "D3D12")
    else()
        message(ERROR "Unknown API")
    endif()
    if(${buildType} MATCHES ".*DEBUG.*")
        set(buildType "-dbg")
    elseif(${buildType} MATCHES ".*DEV.*")
        set(buildType "-dev")
    elseif(${buildType} MATCHES ".*EDITOR.*")
        set(buildType "-ed")
    else()
        set(buildType "")
    endif()
    if(${engineFlags} MATCHES ".*SCENE_DEBUG=1.*")
        set(scene "-worldDbg")
    endif()
    if(${engineFlags} MATCHES ".*AUDIO_DEBUG=1.*")
        set(audio "-audioDbg")
    endif()
    if(${engineFlags} MATCHES ".*RENDER_DEBUG=1.*")
        set(renderSystem "-rsDbg")
    elseif(${engineFlags} MATCHES ".*RENDER_SYSTEM_DEBUG=1.*")
        set(renderSystem "-rsDbg")
    endif()
    unset(${out})
    set(${out} "${api}${buildType}${renderSystem}${world}${physics}${ai}${audio}" PARENT_SCOPE)
endfunction()

function(SetBuildTypeProperties buildType outName)
    set_property( GLOBAL PROPERTY ${PROJECT_NAME}_OUT_NAME_${buildType} ${outName} )
endfunction()

function(AddBuildType ver)
    set(options OPTIONAL FAST)
    set(oneValueArgs API COMPILER_FLAG LINKER_FLAG ENGINE_FLAG)
    set(multiValueArgs COMPILER_FLAGS LINKER_FLAGS ENGINE_FLAGS)
    cmake_parse_arguments(PARSE_ARGV 1 arg "${options}" "${oneValueArgs}" "${multiValueArgs}")
    message(STATUS "BuildType: ${ver} with compile flags: ${arg_COMPILER_FLAGS} and linker flags: ${arg_LINKER_FLAGS} argv=${ARGV} argc=${ARGC} argn=${ARGN}")

    foreach(API ${APIS})
        get_property(tmp GLOBAL PROPERTY VKE_CONFIGURATION_LIST)
        list(APPEND tmp "${ver}_${API}")
        set(outName "")
        GenOutputName( ${ver} ${API} ${arg_COMPILER_FLAGS} ${arg_ENGINE_FLAGS} outName )
        set(CMAKE_CXX_FLAGS_${ver}_${API} "-DVKE_RENDER_SYSTEM=VKE_${API} ${arg_COMPILER_FLAGS}" CACHE STRING "C++ flags for ${ver} configuration" FORCE)
        set(CMAKE_C_FLAGS_${ver}_${API} "-DVKE_RENDER_SYSTEM=VKE_${API} ${arg_COMPILER_FLAGS}" CACHE STRING "C flags for ${ver} configuration" FORCE)
        set(CMAKE_EXE_LINKER_FLAGS_${ver}_${API} "${arg_LINKER_FLAGS}" CACHE STRING "Linker flags for ${ver} configuration" FORCE)
        set(CMAKE_SHARED_LINKER_FLAGS_${ver}_${API} "${arg_LINKER_FLAGS}" CACHE STRING "Shared linker flags for ${ver} configuration" FORCE)
        #message(STATUS "config list2: ${tmp}")
        set_property(GLOBAL PROPERTY VKE_CONFIGURATION_LIST ${tmp})
    endforeach()

endfunction()

set(WORLD_DEBUG_FLAGS)
set(RENDER_DEBUG_FLAGS)
set(PHYSICS_DEBUG_FLAGS)

AddBuildType(
    DEBUG
    OUT_NAME "dbg"
    ENGINE_FLAGS "a"
    COMPILER_FLAGS "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_C_FLAGS_DEBUG} -DDEBUG"
    LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${RELEASE_LINKER_DEBUG}")

AddBuildType(
    RELEASE
    OUT_NAME "rel"
    ENGINE_FLAGS "b"
    COMPILER_FLAGS "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -DRELEASE"
    LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${RELEASE_LINKER_SETTINGS}")

get_property(tmp GLOBAL PROPERTY VKE_CONFIGURATION_LIST)
message(STATUS "config list: ${tmp}")
set (CMAKE_CONFIGURATION_TYPES "Debug_Vulkan;Debug_D3D12;Release_Vulkan;Release_D3D12;" CACHE STRING "Configurations" FORCE)


if(CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Generator is multi-config: ${CMAKE_GENERATOR}")
  message(STATUS "Available configs: ${CMAKE_CONFIGURATION_TYPES}")
else()
  message(STATUS "Generator is single-config: ${CMAKE_GENERATOR}")
  message(STATUS "Current buld type: ${CMAKE_BUILD_TYPE}")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(VKE_WINDOWS 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(VKE_LINUX 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    set(VKE_ANDROID 1)
else()
    message(FATAL_ERROR "Unknown system")
endif()

include(CompileOptions.cmake NO_POLICY_SCOPE)
include(CompilerSettings.cmake NO_POLICY_SCOPE)

message(STATUS "source dir: ${CMAKE_SOURCE_DIR}")

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include/vke")
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(THIRD_PARTY_DIR "${SOURCE_DIR}/ThirdParty")
set(THIRD_PARTY_LIB_DIRS "${LIB_DIR}")
set(THIRD_PARTY_LIBS)
set(DLL_DEPENDENCIES )

find_package(glslang CONFIG REQUIRED)
set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper)

find_package(ZLIB REQUIRED)
set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" ZLIB::ZLIB)

if(VKE_USE_DIRECTX_SHADER_COMPILER)
    find_package(directx-dxc CONFIG REQUIRED)
    set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" Microsoft::DirectXShaderCompiler)
    set(DLL_DEPENDENCIES ${DLL_DEPENDENCIES} "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/dxcompiler.dll")
    set(DLL_DEPENDENCIES ${DLL_DEPENDENCIES} "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/dxil.dll")
endif()

find_package(directxtex CONFIG REQUIRED)
find_package(DevIL REQUIRED)

if(VKE_USE_DIRECTXTEX)
    set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} Microsoft::DirectXTex)
elseif(VKE_USE_DEVIL)
    set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} DevIL::ILU DevIL::ILUT)
endif()

find_package(directxmath CONFIG REQUIRED)
set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" Microsoft::DirectXMath)

if(VKE_COMPILE_VULKAN_RHI)
    find_package(Vulkan REQUIRED)
    message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")
    message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
    set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" Vulkan::Vulkan)
endif()

if(VKE_COMPILE_D3D12_RHI)
    # Lookout for D3D12 library. If not found then we need to add the Windows SDK Lib path to the search dirs.
    # This way CMake can find D3D12.LIB required by the DirectX12 Agility SDK without additional manual steps.
    find_library(D3D12_LIB NAMES d3d12)

    if("${D3D12_LIB}" STREQUAL "D3D12_LIB-NOTFOUND")
        # Resolve the Windows Kits 10 root. The Visual Studio generator provides the SDK version
        # (CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION) but not the root, and CMAKE_WINDOWS_KITS_10_DIR
        # is often empty. The SDK registers its root under the 32-bit WOW6432Node registry key, so
        # read that (a 64-bit CMake reading the plain key returns the wrong location).
        set(_WIN_KITS_ROOT "${CMAKE_WINDOWS_KITS_10_DIR}")
        if(NOT _WIN_KITS_ROOT)
            get_filename_component(_WIN_KITS_ROOT
                "[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows Kits\\Installed Roots;KitsRoot10]"
                ABSOLUTE)
        endif()

        set(_WIN_SDK_INCLUDE_PATH "${_WIN_KITS_ROOT}/Lib/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/um/x64")
        message(STATUS "[WINSDK]: Adding Lib dir to library search path: ${_WIN_SDK_INCLUDE_PATH}")

        # Add _WIN_SDK_INCLUDE_PATH to the library search path
        list(APPEND CMAKE_LIBRARY_PATH "${_WIN_SDK_INCLUDE_PATH}")
    endif()

    find_package(directx-headers CONFIG REQUIRED)
    find_package(directx12-agility CONFIG REQUIRED)
    find_package(winpixevent CONFIG REQUIRED)
    set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" dxgi dxguid Microsoft::DirectX-Headers Microsoft::DirectX-Guids Microsoft::DirectX12-Agility Microsoft::WinPixEventRuntime)

    set(_AGILITY_BIN_DIR     "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
    set(_AGILITY_DBG_BIN_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/bin")

    # D3D12Core.dll exists in both bin/ and debug/bin; pick by config at build time.
    # d3d12SDKLayers.dll only ships in debug/bin, so it is only copied for Debug builds.

endif()

message("Third Party Libs:")
foreach(LIB IN LISTS THIRD_PARTY_LIBS)
    message(" - ${LIB}")
endforeach()

add_subdirectory(include/vke/Core)
add_subdirectory(include/vke/RenderSystem)
add_subdirectory(include/vke/Scene)

include_directories(${INCLUDE_DIR} ${SOURCE_DIR})
link_directories(${LIB_DIR} ${OUTPUT_DIR} ${THIRD_PARTY_LIB_DIRS})

add_library(vkEngine SHARED
    include/vke/CVkEngine.h
    include/vke/CCommandLineArgs.h
    include/vke/CRuntimeOptions.h
    src/CVkEngine.cpp
    src/CCommandLineArgs.cpp
    src/DllMain.cpp
    .clang-format)

add_dependencies(vkEngine vkEngineRenderSystem vkEngineScene)

#set(PROJECT_LIBS debug vkEngineCore_d debug vkEngineRenderSystem_d debug vkEngineScene_d
#    optimized vkEngineCore optimized vkEngineRenderSystem optimized vkEngineScene)

#set(LINK_LIBS ${PROJECT_LIBS} ${THIRD_PARTY_LIBS})
set(LINK_LIBS ${THIRD_PARTY_LIBS})

# Handle libs that were previously in code used as #pragma comment lib.
if(WIN32)
    set(LINK_LIBS ${LINK_LIBS} dbghelp)
endif()

message("Linking vkEngine with libs:")
foreach(LIB IN LISTS LINK_LIBS)
    message(" - ${LIB}")
endforeach()

foreach(DLL_PATH IN LISTS DLL_DEPENDENCIES)
    add_custom_command(TARGET vkEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DLL_PATH}"
        $<TARGET_FILE_DIR:vkEngine>)
endforeach()

if(VKE_COMPILE_D3D12_RHI)
    foreach(DLL_PATH IN LISTS D3D12_AGILITY_DLLS)
        add_custom_command(TARGET vkEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:vkEngine>/D3D12"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_PATH}"
            "$<TARGET_FILE_DIR:vkEngine>/D3D12")
    endforeach()
endif()

target_link_libraries(vkEngine PRIVATE vkEngineRenderSystem vkEngineCore vkEngineScene ${LINK_LIBS})
include(LinkSettings.cmake NO_POLICY_SCOPE)

# Set Vulkan layer path for Visual Studio debugging
if(VKE_COMPILE_VULKAN_RHI)
    set(VULKAN_LAYER_PATH "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin")
    set_target_properties(vkEngine PROPERTIES
        VS_DEBUGGER_ENVIRONMENT "VK_ADD_LAYER_PATH=${VULKAN_LAYER_PATH}"
    )
endif()

if(BUILD_SAMPLES_ENABLED)
    add_subdirectory(samples)
endif()

target_compile_definitions(vkEngine PRIVATE
"$<$<CONFIG:DebugVulkan>:VKE_COMPILE_VULKAN_RHI=2>"
"$<$<CONFIG:ReleaseVulkan>:VKE_COMPILE_VULKAN_RHI=2>")