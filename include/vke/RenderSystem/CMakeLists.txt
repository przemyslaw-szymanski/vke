cmake_minimum_required(VERSION 2.6)

if(VULKAN_RENDER_SYSTEM_ENABLED)
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

if(VULKAN_RENDER_SYSTEM_ENABLED)
	set(INC_DIRS ${INC_DIRS}
		"${INCLUDE_DIR}/RenderSystem/Vulkan/*.h"
		"${INCLUDE_DIR}/RenderSystem/Vulkan/*.inl"
	)
	
	set(SRC_DIRS ${SRC_DIRS}
		"${SOURCE_DIR}/RenderSystem/Vulkan/*.h"
		"${SOURCE_DIR}/RenderSystem/Vulkan/*.inl"
		"${SOURCE_DIR}/RenderSystem/Vulkan/*.cpp"
	)
endif()

set(INC_DIRS ${INC_DIRS}
    "${INCLUDE_DIR}/RenderSystem/Context.h"
	"${INCLUDE_DIR}/RenderSystem/CCommandBuffer.h"
	"${INCLUDE_DIR}/RenderSystem/CComputeContext.h"
	"${INCLUDE_DIR}/RenderSystem/CDataTransferContext.h"
	"${INCLUDE_DIR}/RenderSystem/CDeviceContext.h"
	"${INCLUDE_DIR}/RenderSystem/CGraphicsContext.h"
	"${INCLUDE_DIR}/RenderSystem/CComputeContext.h"
	"${INCLUDE_DIR}/RenderSystem/CTransferContext.h"
	"${INCLUDE_DIR}/RenderSystem/Common.h"
	"${INCLUDE_DIR}/RenderSystem/CRenderingPipeline.h"
	"${INCLUDE_DIR}/RenderSystem/CRenderPass.h"
	"${INCLUDE_DIR}/RenderSystem/CRenderSubPass.h"
	"${INCLUDE_DIR}/RenderSystem/CRenderSystem.h"
	"${INCLUDE_DIR}/RenderSystem/CRenderTarget.h"
	"${INCLUDE_DIR}/RenderSystem/CSwapChain.h"
    "${INCLUDE_DIR}/RenderSystem/CDDI.h"
    "${INCLUDE_DIR}/RenderSystem/CDDITypes.h"
    "${INCLUDE_DIR}/RenderSystem/CQueue.h"
)

set(SRC_DIRS ${SRC_DIRS}
	"${SOURCE_DIR}/RenderSystem/Common.cpp"
    "${SOURCE_DIR}/RenderSystem/CQueue.cpp"
    "${SOURCE_DIR}/RenderSystem/CContextBase.cpp"
    "${SOURCE_DIR}/RenderSystem/CTransferContext.cpp"
    "${SOURCE_DIR}/RenderSystem/CComputeContext.cpp"
    "${SOURCE_DIR}/RenderSystem/Managers/*.h"
    "${SOURCE_DIR}/RenderSystem/Managers/*.cpp"
)

include(../../../IncludeFiles.cmake NO_POLICY_SCOPE)

add_library(vkEngineRenderSystem STATIC
	${INCLUDE_FILES}
	${SOURCE_FILES})
	
add_dependencies(vkEngineRenderSystem vkEngineCore)
	
include(../../../LinkSettings.cmake NO_POLICY_SCOPE)

find_library(FIND_LIB glslangd.lib ${LIB_DIR})

set_target_properties(vkEngineRenderSystem PROPERTIES IMPORTED_LOCATION ${LIB_DIR}/glslangd.lib)

if(VULKAN_RENDER_SYSTEM_ENABLED)
target_link_libraries(vkEngineRenderSystem
	debug vkEngineCore_d
	optimized vkEngineCore
	debug glslangd debug HLSLd debug SPIRVd
	optimized glslang optimized HLSL optimized SPIRV)
else()

endif()
