cmake_minimum_required(VERSION 3.5)

if(VKE_RENDER_SYSTEM_VULKAN)
    set(RENDER_SYSTEM_NAME vkEngineRenderSystem_Vulkan)
endif()

project(vkEngineRenderSystem C CXX)

set(INC_DIRS
    "${INCLUDE_DIR}/RenderSystem/Managers/*.h"
    "${INCLUDE_DIR}/RenderSystem/Managers/*.inl"
    "${INCLUDE_DIR}/RenderSystem/Resources/*.h"
    "${INCLUDE_DIR}/RenderSystem/Resources/*.inl"
    "${INCLUDE_DIR}/RenderSystem/Tasks/*.h"
    "${INCLUDE_DIR}/RenderSystem/Tasks/*.inl"
)
set(SRC_DIRS
    "${SOURCE_DIR}/RenderSystem/Managers/*.h"
    "${SOURCE_DIR}/RenderSystem/Managers/*.inl"
    "${SOURCE_DIR}/RenderSystem/Managers/*.cpp"
    "${SOURCE_DIR}/RenderSystem/Resources/*.h"
    "${SOURCE_DIR}/RenderSystem/Resources/*.inl"
    "${SOURCE_DIR}/RenderSystem/Resources/*.cpp"
    "${SOURCE_DIR}/RenderSystem/Tasks/*.h"
    "${SOURCE_DIR}/RenderSystem/Tasks/*.inl"
    "${SOURCE_DIR}/RenderSystem/Tasks/*.cpp"
)

if(VKE_RENDER_SYSTEM_VULKAN)
    set(INC_DIRS ${INC_DIRS}
        "${INCLUDE_DIR}/RenderSystem/Vulkan/*.h"
        "${INCLUDE_DIR}/RenderSystem/Vulkan/*.inl"
        "${INCLUDE_DIR}/RenderSystem/Vulkan/Managers/*.h"
        "${INCLUDE_DIR}/RenderSystem/Vulkan/Managers/*.inl"
    )
    
    set(SRC_DIRS ${SRC_DIRS}
        "${SOURCE_DIR}/RenderSystem/Vulkan/*.h"
        "${SOURCE_DIR}/RenderSystem/Vulkan/*.inl"
        "${SOURCE_DIR}/RenderSystem/Vulkan/*.cpp"
        "${SOURCE_DIR}/RenderSystem/Vulkan/Managers/*.h"
        "${SOURCE_DIR}/RenderSystem/Vulkan/Managers/*.inl"
        "${SOURCE_DIR}/RenderSystem/Vulkan/Managers/*.cpp"
    )
endif()

if(VKE_RENDER_SYSTEM_D3D12)
    set(INC_DIRS ${INC_DIRS}
        "${INCLUDE_DIR}/RenderSystem/D3D12/*.h"
        "${INCLUDE_DIR}/RenderSystem/D3D12/*.inl"
        "${INCLUDE_DIR}/RenderSystem/D3D12/Managers/*.h"
        "${INCLUDE_DIR}/RenderSystem/D3D12/Managers/*.inl"
    )
    
    set(SRC_DIRS ${SRC_DIRS}
        "${SOURCE_DIR}/RenderSystem/D3D12/*.h"
        "${SOURCE_DIR}/RenderSystem/D3D12/*.inl"
        "${SOURCE_DIR}/RenderSystem/D3D12/*.cpp"
        "${SOURCE_DIR}/RenderSystem/D3D12/Managers/*.h"
        "${SOURCE_DIR}/RenderSystem/D3D12/Managers/*.inl"
        "${SOURCE_DIR}/RenderSystem/D3D12/Managers/*.cpp"
    )
endif()

set(INC_DIRS ${INC_DIRS}
    "${INCLUDE_DIR}/RenderSystem/*.h"
)

# ---------------------------------------------------------------------------
# Code generation: enum -> string conversions
# ---------------------------------------------------------------------------
set(VKE_ENUM_STRINGS_SCRIPT   "${CMAKE_SOURCE_DIR}/tools/gen_enum_strings.py")
set(VKE_ENUM_STRINGS_INPUT    "${INCLUDE_DIR}/RenderSystem/Common.h")
set(VKE_ENUM_STRINGS_OUTPUT   "${INCLUDE_DIR}/RenderSystem/CommonEnumStrings.h")

add_custom_command(
    OUTPUT  "${VKE_ENUM_STRINGS_OUTPUT}"
    COMMAND Python3::Interpreter
            "${VKE_ENUM_STRINGS_SCRIPT}"
            "${VKE_ENUM_STRINGS_INPUT}"
            "${VKE_ENUM_STRINGS_OUTPUT}"
    DEPENDS "${VKE_ENUM_STRINGS_INPUT}"
            "${VKE_ENUM_STRINGS_SCRIPT}"
    COMMENT "Generating CommonEnumStrings.h from Common.h"
    VERBATIM
)

set(SRC_DIRS ${SRC_DIRS}
    "${SOURCE_DIR}/RenderSystem/Common.cpp"
    "${SOURCE_DIR}/RenderSystem/CComputeContext.cpp"
    "${SOURCE_DIR}/RenderSystem/CCommandBuffer.cpp"
    "${SOURCE_DIR}/RenderSystem/CContextBase.cpp"
    "${SOURCE_DIR}/RenderSystem/CSwapChain.cpp"
    "${SOURCE_DIR}/RenderSystem/CDescriptorSet.cpp"
    "${SOURCE_DIR}/RenderSystem/CDeviceContext.cpp"
    "${SOURCE_DIR}/RenderSystem/CDrawcall.cpp"
    "${SOURCE_DIR}/RenderSystem/CFrameBuffer.cpp"
    "${SOURCE_DIR}/RenderSystem/CFrameGraph.cpp"
    "${SOURCE_DIR}/RenderSystem/CFrameGraphNode.cpp"
    "${SOURCE_DIR}/RenderSystem/CGraphicsContext.cpp"
    "${SOURCE_DIR}/RenderSystem/CIndexBuffer.cpp"
    "${SOURCE_DIR}/RenderSystem/CPipeline.cpp"
    "${SOURCE_DIR}/RenderSystem/CRenderingPipeline.cpp"
    "${SOURCE_DIR}/RenderSystem/CRenderPass.cpp"
    "${SOURCE_DIR}/RenderSystem/CRenderQueue.cpp"
    "${SOURCE_DIR}/RenderSystem/CRenderSubPass.cpp"
    "${SOURCE_DIR}/RenderSystem/CRenderSystem.cpp"
    "${SOURCE_DIR}/RenderSystem/CRenderTarget.cpp"
    "${SOURCE_DIR}/RenderSystem/CRenderTree.cpp"
    "${SOURCE_DIR}/RenderSystem/CSubmitManager.cpp"
    "${SOURCE_DIR}/RenderSystem/CTransferContext.cpp"
    "${SOURCE_DIR}/RenderSystem/CQueue.cpp"
    "${SOURCE_DIR}/RenderSystem/CVertexBuffer.cpp"
    "${SOURCE_DIR}/RenderSystem/Managers/*.h"
    "${SOURCE_DIR}/RenderSystem/Managers/*.cpp"
    "${SOURCE_DIR}/RenderSystem/FrameGraph/*"
    "${SOURCE_DIR}/RenderSystem/CShaderCompiler.cpp"
    "${SOURCE_DIR}/RenderSystem/CShaderCompiler.h"
)

include(../../../IncludeFiles.cmake NO_POLICY_SCOPE)

add_library(vkEngineRenderSystem STATIC
    ${INCLUDE_FILES}
    ${SOURCE_FILES}
    "${VKE_ENUM_STRINGS_OUTPUT}")

add_dependencies(vkEngineRenderSystem vkEngineCore)
    
include(../../../LinkSettings.cmake NO_POLICY_SCOPE)

target_link_libraries(vkEngineRenderSystem
    debug vkEngineCore_d
    optimized vkEngineCore
    ${THIRD_PARTY_LIBS} )
