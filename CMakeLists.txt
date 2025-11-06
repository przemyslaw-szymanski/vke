cmake_minimum_required(VERSION 3.5)

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

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin")
set(LIB_DIR "${CMAKE_SOURCE_DIR}/lib")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include/vke")
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(THIRD_PARTY_DIR "${SOURCE_DIR}/ThirdParty")
set(THIRD_PARTY_LIB_DIRS "${LIB_DIR}")
set(THIRD_PARTY_LIBS)

#set(DXC_PATH "${THIRD_PARTY_DIR}/dxc")
#set(DX_SHADER_COMPILER_PATH "${THIRD_PARTY_DIR}/DirectXShaderCompiler")

set(PROJECT_LIBS debug vkEngineCore_d debug vkEngineRenderSystem_d debug vkEngineScene_d
    optimized vkEngineCore optimized vkEngineRenderSystem optimized vkEngineScene)

#set(GLSLANG_LIBS
#debug glslangd debug HLSLd debug SPIRVd debug OSDependentd debug OGLCompilerd
#optimized glslang optimized HLSL optimized SPIRV optimized OSDependent optimized OGLCompiler)
find_package(glslang CONFIG REQUIRED)
set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper)

find_package(ZLIB REQUIRED)
set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" ZLIB::ZLIB)

#set(DXC_LIBS
#    debug dxclib
#    optimized dxclib)
if(VKE_USE_DIRECTX_SHADER_COMPILER)
    find_package(directx-dxc CONFIG REQUIRED)
    set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" Microsoft::DirectXShaderCompiler)
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

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan: ${Vulkan_LIBRARIES}")
message(STATUS "Vulkan include dirs: ${Vulkan_INCLUDE_DIRS}")
set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" Vulkan::Vulkan)

# Is this actually used?
#set(GAINPUT_LIBS
#    optimized gainputstatic debug gainputstatic-d)

#if(VKE_USE_GAINPUT)
#    set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${GAINPUT_LIBS})
#endif()

#if(VKE_USE_DIRECTX_SHADER_COMPILER)
#    set(THIRD_PARTY_LIB_DIRS "${THIRD_PARTY_LIB_DIRS}" "${THIRD_PARTY_DIR}/dxc/lib")
#    set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" "${DXC_LIBS}")
#endif()

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

add_dependencies(vkEngine vkEngineScene)

set(LINK_LIBS ${PROJECT_LIBS} ${THIRD_PARTY_LIBS})
target_link_libraries(vkEngine ${LINK_LIBS})
include(LinkSettings.cmake NO_POLICY_SCOPE)

if(BUILD_SAMPLES_ENABLED)
    add_subdirectory(samples)
endif()
