#pragma once

#include "Core/VKECommon.h"
#include "RenderSystem/Vulkan/VKEImageFormats.h"
#include "Core/Platform/CWindow.h"

namespace VKE
{
    class CRenderSystem;

    template<typename T>
    struct TSArray
    {
        uint32_t    count = 0;
        const T*  pData = nullptr;

        vke_force_inline
        const T& operator[](uint32_t idx) const { return pData[idx]; }
    };

    namespace RenderSystem
    {
        struct SViewportInfo
        {
            ExtentU32   Size;
        };

        struct SPipelineInfo
        {
            uint32_t bEnableDepth : 1;
            uint32_t bEnableStencil : 1;
            uint32_t bEnableBlending : 1;
        };

        struct ContextScopes
        {
            enum SCOPE
            {
                ALL,
                WAIT,
                MEMORY,
                SYNC_OBJECT,
                QUERY,
                BUFFER,
                BUFFER_VIEW,
                TEXTURE,
                TEXTURE_VIEW,
                SHADER,
                PIPELINE,
                SAMPLER,
                DESCRIPTOR,
                FRAMEBUFFER,
                RENDER_PASS,
                COMMAND_BUFFER,
                SWAPCHAIN,
                _MAX_COUNT,
            };
        };
        using CONTEXT_SCOPE = ContextScopes::SCOPE;

        struct ResourceTypes
        {
            enum TYPE
            {
                PIPELINE,
                VERTEX_BUFFER,
                INDEX_BUFFER,
                CONSTANT_BUFFER,
                IMAGE,
                SAMPLER,
                VERTEX_SHADER,
                HULL_SHADER,
                DOMAIN_SHADER,
                GEOMETRY_SHADER,
                PIXEL_SHADER,
                COMPUTE_SHADER,
                FRAMEBUFFER,
                _MAX_COUNT
            };
        };

        struct DeviceTypes
        {
            enum TYPE
            {
                UNKNOWN,
                INTEGRATED,
                DISCRETE,
                VIRTUAL,
                CPU,
                _MAX_COUNT
            };
        };

        using ADAPTER_TYPE = DeviceTypes::TYPE;

        struct MultisamplingTypes
        {
            enum TYPE
            {
                SAMPLE_1,
                SAMPLE_2,
                SAMPLE_4,
                SAMPLE_8,
                SAMPLE_16,
                _MAX_COUNT
            };
        };
        using MULTISAMPLING_TYPE = MultisamplingTypes::TYPE;

        struct RenderQueueUsages
        {
            enum USAGE
            {
                STATIC,
                DYNAMIC,
                _MAX_COUNT
            };
        };
        using RENDER_QUEUE_USAGE = RenderQueueUsages::USAGE;

        struct GraphicsQueueTypes
        {
            static const uint32_t RENDER = VKE_SET_BIT(0);
            static const uint32_t COMPUTE = VKE_SET_BIT(1);
            static const uint32_t TRANSFER = VKE_SET_BIT(2);
            static const uint32_t _MAX_COUNT = 3;
            static const uint32_t GENERAL = RENDER | COMPUTE | TRANSFER;
        };

        using GRAPHICS_QUEUE_TYPE = uint32_t;

        struct SAdapterLimits
        {

        };

        struct SResourceCreateDesc
        {
            CGraphicsContext*   pCtx = nullptr;
            void*       pDesc = nullptr;
        };

        struct SAdapterInfo
        {
            char            name[Constants::MAX_NAME_LENGTH];
            SAdapterLimits  limits;
            uint32_t        apiVersion;
            uint32_t        driverVersion;
            uint32_t        deviceID;
            uint32_t        vendorID;
            ADAPTER_TYPE    type;
            handle_t        handle;
        };

        struct SComputeContextDesc
        {

        };

        struct SDataTransferContextDesc
        {

        };

        struct SSwapChainDesc
        {
            handle_t        hWnd = NULL_HANDLE;
            handle_t        hProcess = NULL_HANDLE;
            ExtentU32       Size = { 800, 600 };
            TEXTURE_FORMAT  format = TextureFormats::R8G8B8A8_UNORM;
            uint16_t        elementCount = Constants::OPTIMAL;
            void*           pPrivate = nullptr;
        };

        struct SGraphicsContextDesc
        {
            SSwapChainDesc  SwapChainDesc;
            void*           pPrivate = nullptr;
        };

        struct SDeviceContextDesc
        {
            const SAdapterInfo* pAdapterInfo = nullptr;
            const void*         pPrivate = nullptr;
        };



        struct SRenderSystemMemoryInfo
        {
            SRenderSystemMemoryInfo() { memset(aResourceTypes, UNDEFINED, sizeof(aResourceTypes)); }
            uint16_t    aResourceTypes[ResourceTypes::_MAX_COUNT];
        };

        struct SRenderSystemDesc
        {
            SRenderSystemMemoryInfo     Memory;
            TSArray< SWindowDesc >      Windows;

            SRenderSystemDesc()
            {
            }
        };

        struct SRendersystemLimits
        {

        };

        struct SRenderQueueDesc
        {
            RENDER_QUEUE_USAGE      usage = RenderQueueUsages::DYNAMIC;
            uint16_t                priority = 0;
        };

        struct SFramebufferDesc
        {
            ExtentU32           Size;
            TSArray<handle_t>   aTextureViews;
            handle_t            hRenderPass;
        };

        struct STextureDesc
        {

        };

        struct STextureViewDesc
        {

        };

        struct SAttachmentDesc
        {
            MULTISAMPLING_TYPE  multisampling;
            TEXTURE_FORMAT      format;
        };

        struct SSubpassDesc
        {

        };

        struct SRenderPassDesc
        {
            TSArray< SAttachmentDesc >  aAttachmentDescs;
            TSArray< SSubpassDesc >     aSubpassDescs;
        };

        namespace EventListeners
        {
            struct IGraphicsContext
            {
                void OnBeginFrame(CGraphicsContext*) {}
                void OnEndFrame(CGraphicsContext*) {}
            };
        } // EventListeners

    } // RenderSystem

    using SRenderSystemDesc = RenderSystem::SRenderSystemDesc;

} // VKE
