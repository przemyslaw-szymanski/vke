cmake_minimum_required(VERSION 2.6)

project(vkEngine C CXX)

# https://stackoverflow.com/questions/29397349/cmake-convert-unix-to-windows-path
macro(msys_to_cmake_path MsysPath ResultingPath)
  string(REGEX REPLACE "^/([a-zA-Z])/" "\\1:/" ${ResultingPath} "${MsysPath}")
endmacro()

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
set(DXC_PATH "${THIRD_PARTY_DIR}\\dxc")


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

source_group(include FILES
    include/vke/CVkEngine.h)
source_group(src FILES
    src/CVkEngine.cpp
    src/DllMain.cpp)

add_subdirectory(include/vke/Core)
add_subdirectory(include/vke/RenderSystem)
add_subdirectory(include/vke/Scene)

include_directories(${INCLUDE_DIR} ${SOURCE_DIR})
link_directories(${LIB_DIR} ${OUTPUT_DIR} ${THIRD_PARTY_LIB_DIRS})

add_library(vkEngine SHARED
	include/vke/CVkEngine.h
	src/CVkEngine.cpp
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