cmake_minimum_required(VERSION 3.5)

# https://stackoverflow.com/questions/29397349/cmake-convert-unix-to-windows-path
macro(msys_to_cmake_path MsysPath ResultingPath)
  string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
endmacro()

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

set(DXC_PATH "${THIRD_PARTY_DIR}/dxc")
set(DX_SHADER_COMPILER_PATH "${THIRD_PARTY_DIR}/DirectXShaderCompiler")

if(NOT IS_DIRECTORY ${DXC_PATH})
    message("Extracting dxc.zip")
    file(ARCHIVE_EXTRACT INPUT "${DXC_PATH}.zip" DESTINATION "${THIRD_PARTY_DIR}" )
endif()

if(NOT IS_DIRECTORY ${DX_SHADER_COMPILER_PATH})
    message("Extracting DirectXShaderCompiler.zip")
    file(ARCHIVE_EXTRACT INPUT "${DX_SHADER_COMPILER_PATH}.zip" DESTINATION "${THIRD_PARTY_DIR}" )
endif()

set(PROJECT_LIBS debug vkEngineCore_d debug vkEngineRenderSystem_d debug vkEngineScene_d
    optimized vkEngineCore optimized vkEngineRenderSystem optimized vkEngineScene)

set(GLSLANG_LIBS
    debug glslangd debug HLSLd debug SPIRVd debug OSDependentd debug OGLCompilerd
    optimized glslang optimized HLSL optimized SPIRV optimized OSDependent optimized OGLCompiler)
    
set(DXC_LIBS
    debug dxclib
    optimized dxclib)
    
set(GAINPUT_LIBS
    optimized gainputstatic debug gainputstatic-d)
    
set(OTHER_LIBS  optimized zlib debug zlibd)

if(VKE_USE_DIRECTXTEX)
    set(THIRD_PARTY_LIBS ${IMAGE_LIBS} optimized DirectXTex debug DirectXTex_d runtimeobject.lib)
elseif(VKE_USE_DEVIL)
    set(THIRD_PARTY_LIBS ${IMAGE_LIBS} optimized DevIL optimized ILU optimized ILUT debug DevIL_d debug ILU_d debug ILUT_d)
endif()

if(VKE_USE_DIRECTX_SHADER_COMPILER)
    set(THIRD_PARTY_LIB_DIRS "${THIRD_PARTY_LIB_DIRS}" "${THIRD_PARTY_DIR}/dxc/lib")
    set(THIRD_PARTY_LIBS "${THIRD_PARTY_LIBS}" "${DXC_LIBS}")
endif()
if(VKE_USE_GLSL_COMPILER)
    #set(THIRD_PARTY_LIB_DIRS ${THIRD_PARTY_LIB_DIRS})
    set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${GLSLANG_LIBS})
endif()

if(VKE_USE_GAINPUT)
    set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${GAINPUT_LIBS})
endif()

set(THIRD_PARTY_LIBS ${THIRD_PARTY_LIBS} ${OTHER_LIBS})


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
message(${THIRD_PARTY_LIBS})
set(LINK_LIBS ${PROJECT_LIBS} ${THIRD_PARTY_LIBS})
target_link_libraries(vkEngine ${LINK_LIBS})
    
include(LinkSettings.cmake NO_POLICY_SCOPE)

if(BUILD_SAMPLES_ENABLED)
    add_subdirectory(samples)
endif()

cmake_path( CONVERT "${DXC_PATH}/bin/dxcompiler.dll" TO_NATIVE_PATH_LIST SRC_TMP )
cmake_path( CONVERT "${OUTPUT_DIR}/dxcompiler.dll" TO_NATIVE_PATH_LIST DST_TMP )
add_custom_command(
    TARGET vkEngine
    POST_BUILD
    COMMAND copy
    ARGS /y /b  ${SRC_TMP} ${DST_TMP} )

find_package( Vulkan )

if(Vulkan_FOUND)
    message( "Vulkan SDK found : ${Vulkan_VERSION}" )
else()
    message( "Vulkan SDK was not found or is not properly configured." )
    message( "Debug builds uses VK_LAYER_KHRONOS_validation layer which is part of the SDK." ) 
    message( "Download VK SDK from: https://www.lunarg.com/vulkan-sdk/" )
endif()