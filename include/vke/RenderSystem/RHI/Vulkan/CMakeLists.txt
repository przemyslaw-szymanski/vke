set(RHI_NAME "Vulkan")
set(RHI_LIBRARY_NAME "vkEngineRHI_${RHI_NAME}")

file(GLOB_RECURSE VKE_RHI_FILES CONFIGURE_DEPENDS
    "${INCLUDE_DIR}/RenderSystem/RHI/${RHI_NAME}/*.h"
    "${INCLUDE_DIR}/RenderSystem/RHI/${RHI_NAME}/*.inl"
    "${SOURCE_DIR}/RenderSystem/RHI/${RHI_NAME}/*.h"
    "${SOURCE_DIR}/RenderSystem/RHI/${RHI_NAME}/*.inl"
    "${SOURCE_DIR}/RenderSystem/RHI/${RHI_NAME}/*.cpp")

add_library(${RHI_LIBRARY_NAME} STATIC ${VKE_RHI_FILES})
source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${VKE_RHI_FILES})
target_include_directories(${RHI_LIBRARY_NAME} PRIVATE "${INCLUDE_DIR}" "${SOURCE_DIR}")

find_package(Vulkan REQUIRED)

target_link_libraries(${RHI_LIBRARY_NAME} PRIVATE vkEngineCore Vulkan::Vulkan)
target_compile_definitions(${RHI_LIBRARY_NAME} PRIVATE ${PREPROCESSOR_DEFINITIONS} VKE_API_EXPORT)
set_target_properties(${RHI_LIBRARY_NAME} PROPERTIES
    FOLDER "vkEngine/RHI"
    POSITION_INDEPENDENT_CODE ON)
