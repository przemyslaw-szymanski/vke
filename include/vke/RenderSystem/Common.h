#pragma once

#include "Core/VKECommon.h"
#include "RenderSystem/Vulkan/VKEImageFormats.h"
#include "Core/Platform/CWindow.h"
#include "Core/VKEForwardDeclarations.h"
#include "core/Utils/TCDynamicArray.h"

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
                TEXTURE,
                TEXTURE_VIEW,
                SAMPLER,
                VERTEX_SHADER,
                HULL_SHADER,
                DOMAIN_SHADER,
                GEOMETRY_SHADER,
                PIXEL_SHADER,
                COMPUTE_SHADER,
                FRAMEBUFFER,
                RENDERPASS,
                _MAX_COUNT
            };
        };
        using RESOURCE_TYPE = ResourceTypes::TYPE;

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
                SAMPLE_32,
                SAMPLE_64,
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
            using AttachmentArray = Utils::TCDynamicArray< handle_t, 8 >;
            ExtentU32           Size;
            AttachmentArray     vAttachments;
            handle_t            hRenderPass;
        };

        struct TextureTypes
        {
            enum TYPE
            {
                TEXTURE_1D,
                TEXTURE_2D,
                TEXTURE_3D,
                _MAX_COUNT
            };
        };
        using TEXTURE_TYPE = TextureTypes::TYPE;

        struct TextureViewTypes
        {
            enum TYPE : uint8_t
            {
                VIEW_1D,
                VIEW_2D,
                VIEW_3D,
                VIEW_CUBE,
                VIEW_1D_ARRAY,
                VIEW_2D_ARRAY,
                VIEW_CUBE_ARRAY,
                _MAX_COUNT
            };
        };
        using TEXTURE_VIEW_TYPE = TextureViewTypes::TYPE;

        struct TextureUsages
        {
            enum USAGE : uint8_t
            {
                RENDER_TARGET_WRITE_COLOR,
                RENDER_TARGET_WRITE_DEPTH_STENCIL,
                RENDER_TARGET_READ_COLOR,
                RENDER_TARGET_READ_DEPTH_STENCIL,
                RENDER_TARGET_WRITE_READ_COLOR,
                RENDER_TARGET_WRITE_READ_DEPTH_STENCIL,
                SAMPLED_COLOR,
                SAMPLED_DEPTH_STENCIL,
                _MAX_COUNT
            };
        };
        using TEXTURE_USAGE = TextureUsages::USAGE;

        struct TextureAspects
        {
            enum ASPECT : uint8_t
            {
                UNKNOWN,
                COLOR,
                DEPTH,
                STENCIL,
                DEPTH_STENCIL,
                _MAX_COUNT
            };
        };
        using TEXTURE_ASPECT = TextureAspects::ASPECT;

        struct STextureDesc
        {
            ExtentU32           Size;
            TEXTURE_FORMAT      format = TextureFormats::R8G8B8A8_UNORM;
            TEXTURE_USAGE       usage = TextureUsages::SAMPLED_COLOR;
            TEXTURE_TYPE        type = TextureTypes::TEXTURE_2D;
            MULTISAMPLING_TYPE  multisampling = MultisamplingTypes::SAMPLE_1;
            uint16_t            mipLevelCount = 0;
        };

        struct STextureViewDesc
        {
            handle_t            hTexture = NULL_HANDLE;
            ExtentU32           Size;
            TEXTURE_VIEW_TYPE   type = TextureViewTypes::VIEW_2D;
            TEXTURE_FORMAT      format = TextureFormats::R8G8B8A8_UNORM;
            TEXTURE_ASPECT      aspect = TextureAspects::COLOR;
            uint8_t             beginMipmapLevel = 0;
            uint8_t             endMipmapLevel = 0;
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
            using AttachmentArray = Utils::TCDynamicArray< SAttachmentDesc >;
            using SubpassArray = Utils::TCDynamicArray< SSubpassDesc >;

            AttachmentArray vAttachmentDescs;
            SubpassArray    vSubpassDescs;
        };

        struct SRenderTargetDesc
        {
            using TextureDescArray = Utils::TCDynamicArray< STextureDesc >;
            using HandleArray = Utils::TCDynamicArray< handle_t >;

            ExtentU32           Size;
            TextureDescArray    vTextureDescs;
            HandleArray         vTextures;
            HandleArray         vTextureViews;
        };

        namespace EventListeners
        {
            struct IGraphicsContext
            {
                void OnBeginFrame(CGraphicsContext*) {}
                void OnEndFrame(CGraphicsContext*) {}
                void OnAfterPresent(CGraphicsContext*) {}
                void OnBeforePresent(CGraphicsContext*) {}
                void OnBeforeExecute(CGraphicsContext*) {}
                void OnAfterExecute(CGraphicsContext*) {}
            };
        } // EventListeners


        class CRenderTarget;
        class CRenderSystem;
        class CGraphicsContext;
        class CDeviceContext;
        class CSwapChain;

    } // RenderSystem

    using SRenderSystemDesc = RenderSystem::SRenderSystemDesc;

} // VKE
