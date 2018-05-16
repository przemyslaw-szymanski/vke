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

set(PREPROCESSOR_DEFINITIONS IL_STATIC_LIB)

if(VULKAN_RENDER_SYSTEM_ENABLED)
	set(PREPROCESSOR_DEFINITIONS ${PREPROCESSOR_DEFINITIONS} VKE_VULKAN_RENDERER)
endif()
