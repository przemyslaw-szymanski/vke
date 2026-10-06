cmake_minimum_required(VERSION 3.10...3.31)

project(vkEngineRenderSystem C CXX)

# ---------------------------------------------------------------------------
# Code generation: enum -> string conversions
# ---------------------------------------------------------------------------
set(VKE_ENUM_STRINGS_SCRIPT   "${CMAKE_SOURCE_DIR}/tools/gen_enum_strings.py")
set(VKE_ENUM_STRINGS_INPUT    "${INCLUDE_DIR}/RenderSystem/Common.h")
set(VKE_ENUM_STRINGS_OUTPUT   "${INCLUDE_DIR}/RenderSystem/CommonEnumStrings.h")

# Execute CommonEnumStrings generation on configuration. If this step is omitted then CMake
# will never find CommonEnumStrings.h on configure, ending with CMake complaining on missing file.
execute_process(
    COMMAND ${Python3_EXECUTABLE}
            "${VKE_ENUM_STRINGS_SCRIPT}"
            "${VKE_ENUM_STRINGS_INPUT}"
            "${VKE_ENUM_STRINGS_OUTPUT}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Scan for all files
set(VKE_RENDER_SYSTEM_INCLUDE_ROOT "${INCLUDE_DIR}/RenderSystem")
set(VKE_RENDER_SYSTEM_SOURCE_ROOT "${SOURCE_DIR}/RenderSystem")

file(GLOB_RECURSE RENDER_SYSTEM_INCLUDE_FILES CONFIGURE_DEPENDS
    "${VKE_RENDER_SYSTEM_INCLUDE_ROOT}/*.h"
    "${VKE_RENDER_SYSTEM_INCLUDE_ROOT}/*.inl"
)

file(GLOB_RECURSE RENDER_SYSTEM_SOURCE_FILES CONFIGURE_DEPENDS
    "${VKE_RENDER_SYSTEM_SOURCE_ROOT}/*.h"
    "${VKE_RENDER_SYSTEM_SOURCE_ROOT}/*.inl"
    "${VKE_RENDER_SYSTEM_SOURCE_ROOT}/*.cpp"
)

# Drop all RHI out of sources
list(FILTER RENDER_SYSTEM_INCLUDE_FILES EXCLUDE REGEX "/RenderSystem/RHI/")
list(FILTER RENDER_SYSTEM_SOURCE_FILES EXCLUDE REGEX "/RenderSystem/RHI/")

# Add RHI dependencies
set(RENDER_SYSTEM_INCLUDE_FILES "${RENDER_SYSTEM_INCLUDE_FILES}"
    "${VKE_RENDER_SYSTEM_INCLUDE_ROOT}/RHI/RHI.h"
    "${VKE_RENDER_SYSTEM_INCLUDE_ROOT}/RHI/RHITypes.h"
    "${VKE_RENDER_SYSTEM_INCLUDE_ROOT}/RHI/TCRHI.h"
)

set(INC_DIRS ${RENDER_SYSTEM_INCLUDE_FILES} )
set(SRC_DIRS ${RENDER_SYSTEM_SOURCE_FILES} )

set(LINK_RHI_LIBS)

if(VKE_COMPILE_VULKAN_RHI)
    add_subdirectory(RHI/Vulkan)
    set(LINK_RHI_LIBS ${LINK_RHI_LIBS} vkEngineRHI_Vulkan)
endif()

if(VKE_COMPILE_D3D12_RHI)
    add_subdirectory(RHI/D3D12)
    set(LINK_RHI_LIBS ${LINK_RHI_LIBS} vkEngineRHI_D3D12)
endif()

include(../../../IncludeFiles.cmake NO_POLICY_SCOPE)

add_library(vkEngineRenderSystem STATIC
    ${INCLUDE_FILES}
    ${SOURCE_FILES}
    "${VKE_ENUM_STRINGS_OUTPUT}")

# Register CommonEnumStrings generation on PRE_BUILD.
add_custom_command(
    TARGET vkEngineRenderSystem PRE_BUILD
    COMMAND ${Python3_EXECUTABLE}
            "${VKE_ENUM_STRINGS_SCRIPT}"
            "${VKE_ENUM_STRINGS_INPUT}"
            "${VKE_ENUM_STRINGS_OUTPUT}"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Generating CommonEnumStrings.h from Common.h"
    VERBATIM
)

add_dependencies(vkEngineRenderSystem vkEngineCore)
    
include(../../../LinkSettings.cmake NO_POLICY_SCOPE)

target_link_libraries(vkEngineRenderSystem PUBLIC
    debug vkEngineCore_d
    optimized vkEngineCore
    ${THIRD_PARTY_LIBS}
     ${LINK_RHI_LIBS})
