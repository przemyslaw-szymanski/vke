cmake_minimum_required(VERSION 2.6)

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

source_group(include FILES include/vke/CVkEngine.h)
source_group(src FILES src/CVkEngine.cpp src/DllMain.cpp)

add_subdirectory(include/vke/Core)
add_subdirectory(include/vke/RenderSystem)
add_subdirectory(include/vke/Scene)

include_directories(${INCLUDE_DIR} ${SOURCE_DIR})
link_directories(${LIB_DIR} ${OUTPUT_DIR})

add_library(vkEngine SHARED
	include/vke/CVkEngine.h
	src/CVkEngine.cpp
	src/DllMain.cpp)

add_dependencies(vkEngine vkEngineScene)

set(PROJECT_LIBS debug vkEngineCore_d debug vkEngineRenderSystem_d debug vkEngineScene_d
	optimized vkEngineCore optimized vkEngineRenderSystem optimized vkEngineScene)

set(GLSLANG_LIBS
	debug glslangd debug HLSLd debug SPIRVd debug OSDependentd debug OGLCompilerd
	optimized glslang optimized HLSL optimized SPIRV optimized OSDependent optimized OGLCompiler)
	
set(OTHER_LIBS  optimized zlib debug zlibd)

if(VKE_USE_DIRECTXTEX)
    set(IMAGE_LIBS optimized DirectXTex debug DirectXTex_d runtimeobject.lib)
elseif(VKE_USE_DEVIL)
    set(IMAGE_LIBS ${IMAGE_LIBS} optimized DevIL optimized ILU optimized ILUT debug DevIL_d debug ILU_d debug ILUT_d)
endif()
                
set(LINK_LIBS ${PROJECT_LIBS} ${OTHER_LIBS} ${GLSLANG_LIBS} ${IMAGE_LIBS})
target_link_libraries(vkEngine ${LINK_LIBS})
	
include(LinkSettings.cmake NO_POLICY_SCOPE)

if(BUILD_SAMPLES_ENABLED)
	add_subdirectory(samples)
endif()