cmake_minimum_required(VERSION 2.6)

function(EnableOption option)
	if( ${option} )
		add_definitions("-D${option}=1")
	endif()
endfunction()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DVKE_WINDOWS)
    set(VKE_WINDOWS 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DVKE_LINUX)
    set(VKE_LINUX 1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    add_definitions(-DVKE_ANDROID)
    set(VKE_ANDROID 1)
else()
    message(FATAL_ERROR "Unknown system")
endif()

EnableOption(VKE_USE_XINPUT)
EnableOption(VKE_VULKAN_RENDERER)
EnableOption(VKE_USE_DIRECTX_MATH)
EnableOption(VKE_RENDERER_DEBUG)
EnableOption(VKE_SCENE_DEBUG)
EnableOption(VKE_USE_SSE2)
EnableOption(VKE_VULKAN_RENDERER)
EnableOption(VKE_USE_RIGHT_HANDED_COORDINATES)
EnableOption(VKE_USE_DEVIL)
EnableOption(VKE_USE_DIRECTXTEX)
EnableOption(VKE_USE_DIRECTXSHADERCOMPILER)

if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
	set(CLANG 1)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
	set(GCC 1)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL "Intel")
	set(INTEL 1)
elseif (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
	set(MSVC 1)
endif()

if(MSVC)
	add_definitions("/MP /W4 /WX")
	# ignore warnings
	add_definitions("/wd4201") # nameless union/struct
	add_definitions("/wd4127") # conditional expression is constant
	add_definitions("/wd4533") # initialization of '' skipped by goto
	add_definitions("/wd4100") # unreferenced formal parameter
	add_definitions("/wd4505") # unreferenced local function has been removed
	add_definitions("/wd4221") # This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library
endif()

set(PREPROCESSOR_DEFINITIONS IL_STATIC_LIB)

if(VULKAN_RENDER_SYSTEM_ENABLED)
	set(PREPROCESSOR_DEFINITIONS ${PREPROCESSOR_DEFINITIONS} VKE_VULKAN_RENDERER)
endif()
