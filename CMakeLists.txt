cmake_minimum_required(VERSION 3.20...3.31)

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

# Python3 required for code generation
find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(WORLD_DEBUG_FLAGS)
set(RENDER_DEBUG_FLAGS)
set(PHYSICS_DEBUG_FLAGS)

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

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set(OUTPUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
set(LIB_DIR "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include/vke")
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(THIRD_PARTY_DIR "${SOURCE_DIR}/ThirdParty")
set(THIRD_PARTY_LIB_DIRS "${LIB_DIR}")
set(THIRD_PARTY_LIBS)
set(DLL_DEPENDENCIES )

set(VKE_DATA_DIR "${CMAKE_SOURCE_DIR}/bin" CACHE PATH "Root directory of runtime assets")

message(STATUS "Output configuration:")
message(STATUS "- source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "- bin dir: ${OUTPUT_DIR}")
message(STATUS "- lib dir: ${LIB_DIR}")
message(STATUS "- asset data dir: ${VKE_DATA_DIR}")

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
    set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} DevIL::IL DevIL::ILU)
endif()

find_package(directxmath CONFIG REQUIRED)
set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" Microsoft::DirectXMath)

#message("Third Party Libs:")
#foreach(LIB IN LISTS THIRD_PARTY_LIBS)
#    message(" - ${LIB}")
#endforeach()

add_subdirectory(include/vke/Core)
add_subdirectory(include/vke/RenderSystem)
add_subdirectory(include/vke/Scene)

target_compile_definitions(vkEngineCore PRIVATE VKE_DEFAULT_DATA_DIR="${VKE_DATA_DIR}")

include_directories(${INCLUDE_DIR} ${SOURCE_DIR})
link_directories(${LIB_DIR} ${OUTPUT_DIR} ${THIRD_PARTY_LIB_DIRS})

add_library(vkEngine SHARED
    include/vke/CVkEngine.h
    include/vke/CCommandLineArgs.h
    include/vke/CRuntimeOptions.h
    src/CVkEngine.cpp
    src/CCommandLineArgs.cpp
    .clang-format)

# DllMain is the Windows DLL entry point and includes <windows.h>; only build it on Windows.
if(VKE_WINDOWS)
    target_sources(vkEngine PRIVATE src/DllMain.cpp)
endif()

add_dependencies(vkEngine vkEngineRenderSystem vkEngineScene)

#set(PROJECT_LIBS debug vkEngineCore_d debug vkEngineRenderSystem_d debug vkEngineScene_d
#    optimized vkEngineCore optimized vkEngineRenderSystem optimized vkEngineScene)

#set(LINK_LIBS ${PROJECT_LIBS} ${THIRD_PARTY_LIBS})
set(LINK_LIBS ${THIRD_PARTY_LIBS})

# Handle libs that were previously in code used as #pragma comment lib.
if(WIN32)
    set(LINK_LIBS ${LINK_LIBS} dbghelp)
endif()

#message("Linking vkEngine with libs:")
#foreach(LIB IN LISTS LINK_LIBS)
#    message(" - ${LIB}")
#endforeach()

if(DLL_DEPENDENCIES)
    add_custom_command(TARGET vkEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DLL_DEPENDENCIES}
                $<TARGET_FILE_DIR:vkEngine>
        COMMAND_EXPAND_LISTS
        VERBATIM)
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
