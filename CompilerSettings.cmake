cmake_minimum_required(VERSION 3.10...3.31)

function(EnableOption option)
	if( ${option} )
		add_definitions("-D${option}=1")
	else()
		add_definitions("-D${option}=0")
	endif()
endfunction()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DVKE_WINDOWS=1)
    set(VKE_WINDOWS 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DVKE_LINUX=1)
    set(VKE_LINUX 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    add_definitions(-DVKE_ANDROID=1)
    set(VKE_ANDROID 1)
else()
    message(FATAL_ERROR "Unknown system")
endif()

EnableOption(VKE_DEBUG_INFO)
EnableOption(VKE_USE_XINPUT)
EnableOption(VKE_COMPILE_VULKAN_RHI)
EnableOption(VKE_COMPILE_D3D12_RHI)
EnableOption(VKE_USE_DIRECTX_MATH)
EnableOption(VKE_RENDERER_DEBUG)
EnableOption(VKE_SCENE_DEBUG)
EnableOption(VKE_USE_SSE2)
EnableOption(VKE_USE_RIGHT_HANDED_COORDINATES)
EnableOption(VKE_USE_DEVIL)
EnableOption(VKE_USE_DIRECTXTEX)
EnableOption(VKE_USE_DIRECTX_SHADER_COMPILER)
EnableOption(VKE_USE_GLSL_COMPILER)
EnableOption(VKE_USE_HLSL_SYNTAX)
EnableOption(VKE_USE_GAINPUT)
EnableOption(VKE_USE_RAW_INPUT)
EnableOption(VKE_SCENE_TERRAIN_DEBUG)
EnableOption(VKE_ASSERT_ENABLE)
EnableOption(VKE_RENDER_SYSTEM_MEMORY_DEBUG)
EnableOption(VKE_MEMORY_DEBUG)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CLANG 1)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	set(GCC 1)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
	set(INTEL 1)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	set(MSVC 1)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
	add_definitions(-DVKE_COMPILER_VISUAL_STUDIO=1)

	add_definitions("/MP /W4 /WX /EHsc")
    # add_definitions("/std:c++latest")

    if(VKE_DEBUG_INFO)
        add_definitions("/Zi")
        add_definitions("/DEBUG")
    endif()

	# ignore warnings
	add_definitions("/wd4201") # nameless union/struct
	add_definitions("/wd4127") # conditional expression is constant
	add_definitions("/wd4533") # initialization of '' skipped by goto
	add_definitions("/wd4100") # unreferenced formal parameter
	add_definitions("/wd4505") # unreferenced local function has been removed
	add_definitions("/wd4221") # This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library

elseif(GCC)

	if(MINGW)
		add_definitions("-DVKE_COMPILER_MINGW=1")
	else()
		add_definitions("-DVKE_COMPILER_GCC=1")
	endif()
	
	add_definitions("-Wall") # Covers /W4
	add_definitions("-Wextra") # Covers /W4
	add_definitions("-Wfatal-errors") # Any warning/error/notice treat as fatal (fatal stops compilation)

	if(VKE_DEBUG_INFO)
		add_definitions("-g")
	endif()

	# ignore warnings
	add_definitions("-Wno-unused-function")
	add_definitions("-Wno-unused-variable")

endif()

set(PREPROCESSOR_DEFINITIONS IL_STATIC_LIB)
