cmake_minimum_required(VERSION 2.6)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_definitions(-DVKE_WINDOWS)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    add_definitions(-DVKE_LINUX)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Android")
    add_definitions(-DVKE_ANDROID)
else()
    message(FATAL_ERROR "Unknown system")
endif()

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
	add_definitions("/MP /W4")
	# ignore warnings
	add_definitions("/wd4201") #nameless union/struct
	add_definitions("/wd4127") #conditional expression is constant
endif()

set(PREPROCESSOR_DEFINITIONS IL_STATIC_LIB)

if(VULKAN_RENDER_SYSTEM_ENABLED)
	set(PREPROCESSOR_DEFINITIONS ${PREPROCESSOR_DEFINITIONS} VKE_VULKAN_RENDERER)
endif()
