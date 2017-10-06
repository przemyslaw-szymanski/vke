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
        struct TextureTag {};
        struct TextureViewTag {};
        struct RenderTargetTag {};
        struct SamplerTag {};
        struct RenderPassTag {};
        struct RenderingPipelineTag {};
        struct FramebufferTag {};

        using TextureHandle = _STagHandle< TextureTag >;
        using TextureViewHandle = _STagHandle< TextureViewTag >;
        using RenderTargetHandle = _STagHandle< RenderTargetTag >;
        using RenderPassHandle = _STagHandle< RenderPassTag >;
        using RenderingPipelineHandle = _STagHandle< RenderingPipelineTag >;
        using SamplerHandle = _STagHandle< SamplerTag >;
        using FramebufferHandle = _STagHandle< FramebufferTag >;

        struct VKE_API SColor
        {
            union
            {
                struct { float r, g, b, a; };
                float       floats[ 4 ];
                uint32_t    uints[ 4 ];
                int32_t     ints[ 4 ];
                uint8_t     bytes[ 4 ];
            };

            SColor() {}

            SColor(const SColor& Other) :
                r(Other.r), g(Other.g), b(Other.b), a(Other.a) {}

            explicit SColor(uint32_t /*raw*/)
            {}
            SColor(float red, float green, float blue, float alpha) :
                r(red), g(green), b(blue), a(alpha) {}
            explicit SColor(float v) :
                r(v), g(v), b(v), a(v) {}


            void CopyToNative(void* pNativeArray) const;

            static const SColor ZERO;
            static const SColor ONE;
            static const SColor RED;
            static const SColor GREEN;
            static const SColor BLUE;
            static const SColor ALPHA;
        };

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
            WindowPtr       pWindow = WindowPtr();
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
                TRANSFER_SRC,
				TRANSFER_DST,
				SAMPLED,
				STORAGE,
				COLOR_RENDER_TARGET,
				DEPTH_STENCIL_RENDER_TARGET,
                _MAX_COUNT
            };
        };
        using TEXTURE_USAGE = TextureUsages::USAGE;

		struct TextureLayouts
		{
			enum LAYOUT : uint8_t
			{
				UNDEFINED,
				GENERAL,
				COLOR_RENDER_TARGET,
				DEPTH_STENCIL_RENDER_TARGET,
				DEPTH_BUFFER,
				SHADER_READ,
				TRANSFER_SRC,
				TRANSFER_DST,
				PRESENT,
				_MAX_COUNT
			};
		};
		using TEXTURE_LAYOUT = TextureLayouts::LAYOUT;

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
            TEXTURE_USAGE       usage = TextureUsages::SAMPLED;
            TEXTURE_TYPE        type = TextureTypes::TEXTURE_2D;
            MULTISAMPLING_TYPE  multisampling = MultisamplingTypes::SAMPLE_1;
            uint16_t            mipLevelCount = 0;
        };

        struct STextureViewDesc
        {
            TextureHandle       hTexture = NULL_HANDLE;
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

        struct RenderPassAttachmentUsages
        {
			enum USAGE
			{
				UNDEFINED,
				COLOR, // load = dont't care, store = don't care
				COLOR_CLEAR, // load = clear, store = dont't care
				COLOR_STORE, // load = don't care, store = store
				COLOR_CLEAR_STORE, // load = clear, store = store
				DEPTH_STENCIL,
				DEPTH_STENCIL_CLEAR,
				DEPTH_STENCIL_STORE,
				DEPTH_STENCIL_CLEAR_STORE,
				_MAX_COUNT
			};

            struct Write
            {
                enum USAGE
                {
                    UNDEFINED,
                    COLOR, // load = dont't care, store = don't care
                    COLOR_CLEAR, // load = clear, store = dont't care
                    COLOR_STORE, // load = don't care, store = store
                    COLOR_CLEAR_STORE, // load = clear, store = store
                    DEPTH_STENCIL,
                    DEPTH_STENCIL_CLEAR,
                    DEPTH_STENCIL_STORE,
                    DEPTH_STENCIL_CLEAR_STORE,
                    _MAX_COUNT
                };
            };

            struct Read
            {
                enum USAGE
                {
                    UNDEFINED,
                    COLOR,
                    COLOR_CLEAR,
                    COLOR_STORE,
                    COLOR_CLEAR_STORE,
                    DEPTH_STENCIL,
                    DEPTH_STENCIL_CLEAR,
                    DEPTH_STENCIL_STORE,
                    DEPTH_STENCIL_CLEAR_STORE,
                    _MAX_COUNT
                };
            };
        };
        using RENDER_PASS_WRITE_ATTACHMENT_USAGE = RenderPassAttachmentUsages::Write::USAGE;
        using RENDER_PASS_READ_ATTACHMENT_USAGE = RenderPassAttachmentUsages::Read::USAGE;
		using RENDER_PASS_ATTACHMENT_USAGE = RenderPassAttachmentUsages::USAGE;

        struct VKE_API SRenderPassDesc
        {
            struct VKE_API SSubpassDesc
            {
                struct VKE_API SAttachmentDesc
                {
                    TextureViewHandle hTextureView = NULL_HANDLE;
                    TEXTURE_LAYOUT layout = TextureLayouts::UNDEFINED;
                };

                using AttachmentDescArray = Utils::TCDynamicArray< SAttachmentDesc, 8 >;
                AttachmentDescArray vRenderTargets;
                AttachmentDescArray vTextures;
                SAttachmentDesc DepthBuffer;
            };

            struct VKE_API SAttachmentDesc
            {
                TextureViewHandle hTextureView;
                TEXTURE_LAYOUT beginLayout = TextureLayouts::UNDEFINED;
                TEXTURE_LAYOUT endLayout = TextureLayouts::UNDEFINED;
                RENDER_PASS_ATTACHMENT_USAGE usage = RenderPassAttachmentUsages::UNDEFINED;
                SColor ClearColor = SColor::ONE;
            };

			using SubpassDescArray = Utils::TCDynamicArray< SSubpassDesc, 8 >;
			using AttachmentDescArray = Utils::TCDynamicArray< SAttachmentDesc, 8 >;

			AttachmentDescArray vAttachments;
			SubpassDescArray vSubpasses;
            ExtentU16 Size;
        };
        using SRenderPassAttachmentDesc = SRenderPassDesc::SAttachmentDesc;
        using SSubpassAttachmentDesc = SRenderPassDesc::SSubpassDesc::SAttachmentDesc;

        struct SRenderingPipelineDesc
        {
            using RenderPassArray = Utils::TCDynamicArray< handle_t >;

            RenderPassArray     vRenderPassHandles;
        };

        namespace EventListeners
        {
            struct IGraphicsContext
            {
                bool OnBeginFrame(CGraphicsContext*) { return true; }
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
