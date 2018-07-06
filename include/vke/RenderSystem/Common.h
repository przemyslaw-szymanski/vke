#pragma once

#include "Core/VKECommon.h"
#include "RenderSystem/Vulkan/VKEImageFormats.h"
//#include "Core/Platform/CWindow.h"
#include "Core/VKEForwardDeclarations.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/TCString.h"
#include "Core/Memory/Common.h"
#include "Core/Utils/CLogger.h"
#include "Core/Resources/CResource.h"
#include "Config.h"

namespace VKE
{

#if VKE_RENDER_SYSTEM_DEBUG || VKE_DEBUG
#   define VKE_RENDER_SYSTEM_DEBUG_CODE(_code) _code
#   define VKE_RENDER_SYSTEM_DEBUG_NAME cstr_t pDebugName = ""
#else
#   define VKE_RENDER_SYSTEM_DEBUG_CODE(_code)
#   define VKE_RENDER_SYSTEM_DEBUG_NAME
#endif // VKE_RENDER_SYSTEM_DEBUG

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
        struct ShaderTag {};
        struct ShaderProgramTag {};

        using TextureHandle = _STagHandle< TextureTag >;
        using TextureViewHandle = _STagHandle< TextureViewTag >;
        using RenderTargetHandle = _STagHandle< RenderTargetTag >;
        using RenderPassHandle = _STagHandle< RenderPassTag >;
        using RenderingPipelineHandle = _STagHandle< RenderingPipelineTag >;
        using SamplerHandle = _STagHandle< SamplerTag >;
        using FramebufferHandle = _STagHandle< FramebufferTag >;
        using ShaderHandle = _STagHandle< ShaderTag >;
        using ShaderProgramHandle = _STagHandle< ShaderProgramTag >;

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

        struct SViewportDesc
        {
            ExtentF32   Position;
            ExtentF32   Size;
            ExtentF32   MinMaxDepth;
        };

        struct SScissorDesc
        {
            ExtentI32   Position;
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
            enum SCOPE : uint8_t
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
            enum TYPE : uint8_t
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
            enum TYPE : uint8_t
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

        struct SampleCounts
        {
            enum COUNT : uint8_t
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
        using SAMPLE_COUNT = SampleCounts::COUNT;

        struct RenderQueueUsages
        {
            enum USAGE : uint8_t
            {
                STATIC,
                DYNAMIC,
                _MAX_COUNT
            };
        };
        using RENDER_QUEUE_USAGE = RenderQueueUsages::USAGE;

        struct GraphicsQueueTypes
        {
            static const uint32_t RENDER = VKE_BIT(0);
            static const uint32_t COMPUTE = VKE_BIT(1);
            static const uint32_t TRANSFER = VKE_BIT(2);
            static const uint32_t _MAX_COUNT = 3;
            static const uint32_t GENERAL = RENDER | COMPUTE | TRANSFER;
        };

        using GRAPHICS_QUEUE_TYPE = uint32_t;

        struct SAdapterLimits
        {

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
            TEXTURE_FORMAT  format = Formats::R8G8B8A8_UNORM;
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
            enum TYPE : uint8_t
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
            enum BITS
            {
                TRANSFER_SRC                = VKE_BIT( 1 ),
                TRANSFER_DST                = VKE_BIT( 2 ),
                SAMPLED                     = VKE_BIT( 3 ),
                STORAGE                     = VKE_BIT( 4 ),
                COLOR_RENDER_TARGET         = VKE_BIT( 5 ),
                DEPTH_STENCIL_RENDER_TARGET = VKE_BIT( 6 ),
                _MAX_COUNT                  = 6
            };
        };
        using TEXTURE_USAGES = uint32_t;

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

        struct MemoryUsages
        {
            enum BITS
            {
                NO_ALLOCATION           = VKE_BIT( 1 ),
                SEPARATE_ALLOCATION     = VKE_BIT(2),
                CPU_ACCESS              = VKE_BIT(3),
                CPU_NO_FLUSH            = VKE_BIT(4),
                CPU_CACHED              = VKE_BIT(5),
                GPU_ACCESS              = VKE_BIT(6),
                DYNAMIC                 = CPU_ACCESS | GPU_ACCESS,
                STATIC                  = GPU_ACCESS,
                DEFAULT                 = STATIC
            };
        };
        using MEMORY_USAGES = uint32_t;

        struct STextureDesc
        {
            ExtentU32           Size;
            TEXTURE_FORMAT      format = Formats::R8G8B8A8_UNORM;
            TEXTURE_USAGES      usage = TextureUsages::SAMPLED;
            TEXTURE_TYPE        type = TextureTypes::TEXTURE_2D;
            SAMPLE_COUNT        multisampling = SampleCounts::SAMPLE_1;
            uint16_t            mipLevelCount = 0;
            MEMORY_USAGES       memoryUsage = MemoryUsages::DEFAULT;
        };

        struct STextureViewDesc
        {
            TextureHandle       hTexture = NULL_HANDLE;
            TEXTURE_VIEW_TYPE   type = TextureViewTypes::VIEW_2D;
            TEXTURE_FORMAT      format = Formats::R8G8B8A8_UNORM;
            TEXTURE_ASPECT      aspect = TextureAspects::COLOR;
            uint8_t             beginMipmapLevel = 0;
            uint8_t             endMipmapLevel = 1;
        };

        struct SAttachmentDesc
        {
            SAMPLE_COUNT    multisampling = SampleCounts::SAMPLE_1;
            TEXTURE_FORMAT  format;
        };

        struct RenderPassAttachmentUsages
        {
            enum USAGE : uint8_t
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
                enum USAGE : uint8_t
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
                enum USAGE : uint8_t
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
                    VKE_RENDER_SYSTEM_DEBUG_NAME;
                };

                using AttachmentDescArray = Utils::TCDynamicArray< SAttachmentDesc, 8 >;
                AttachmentDescArray vRenderTargets;
                AttachmentDescArray vTextures;
                SAttachmentDesc DepthBuffer;
                VKE_RENDER_SYSTEM_DEBUG_NAME;
            };

            struct VKE_API SAttachmentDesc
            {
                TextureViewHandle hTextureView;
                TEXTURE_LAYOUT beginLayout = TextureLayouts::UNDEFINED;
                TEXTURE_LAYOUT endLayout = TextureLayouts::UNDEFINED;
                RENDER_PASS_ATTACHMENT_USAGE usage = RenderPassAttachmentUsages::UNDEFINED;
                SColor ClearColor = SColor::ONE;
                VKE_RENDER_SYSTEM_DEBUG_NAME;
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
            struct SPassDesc
            {
                using Callback = std::function< void(const SPassDesc&) >;
                RenderPassHandle    hPass = NULL_HANDLE;
                RenderPassHandle    hWaitForPass = NULL_HANDLE;
                Callback            OnRender = {};
                bool                isAsync = false;
                VKE_RENDER_SYSTEM_DEBUG_NAME;
            };
            using RenderPassArray = Utils::TCDynamicArray< SPassDesc >;

            RenderPassArray     vRenderPassHandles;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        namespace EventListeners
        {
            struct IGraphicsContext
            {
                virtual bool OnRenderFrame(CGraphicsContext*) { return true; }
                virtual void OnAfterPresent(CGraphicsContext*) {}
                virtual void OnBeforePresent(CGraphicsContext*) {}
                virtual void OnBeforeExecute(CGraphicsContext*) {}
                virtual void OnAfterExecute(CGraphicsContext*) {}
            };
        } // EventListeners

        struct SDeviceMemoryManagerDesc
        {
            SMemoryPoolManagerDesc  MemoryPoolDesc;
        };

        struct MemoryAccessTypes
        {
            enum TYPE
            {
                UNKNOWN,
                GPU,
                CPU,
                CPU_OPTIMAL,
                _MAX_COUNT
            };
        };
        using MEMORY_ACCESS_TYPE = MemoryAccessTypes::TYPE;

        struct SResourceManagerDesc
        {
            SDeviceMemoryManagerDesc    DeviceMemoryDesc;
            uint32_t aMemorySizes[ MemoryAccessTypes::_MAX_COUNT ] = { 0 };
        };

        struct ShaderTypes
        {
            enum TYPE
            {
                VERTEX,
                TESS_HULL,
                TESS_DOMAIN,
                GEOMETRY,
                PIXEL,
                COMPUTE,
                _MAX_COUNT
            };
        };
        using SHADER_TYPE = ShaderTypes::TYPE;

        struct SShaderDesc
        {
            using StringArray = Utils::TCDynamicArray< Utils::CString >;
            SResourceDesc   Base;
            SHADER_TYPE     type;
            cstr_t          pEntryPoint = "main";
            StringArray     vIncludes;
            StringArray     vPreprocessor;
            
            SShaderDesc() {}
            
            SShaderDesc(const SShaderDesc& Other)
            {
                this->operator=( Other );
            }

            SShaderDesc(SShaderDesc&& Other)
            {
                this->operator=( std::move( Other ) );
            }

            SShaderDesc& operator=(const SShaderDesc& Other)
            {
                Base = Other.Base;
                type = Other.type;
                pEntryPoint = Other.pEntryPoint;
                vIncludes = Other.vIncludes;
                vPreprocessor = Other.vPreprocessor;
                return *this;
            }

            SShaderDesc& operator=(SShaderDesc&& Other)
            {
                Base = Other.Base;
                type = Other.type;
                pEntryPoint = Other.pEntryPoint;
                vIncludes = std::move(Other.vIncludes);
                vPreprocessor = std::move(Other.vPreprocessor);
                return *this;
            }
        };

        struct VertexInputRates
        {
            enum RATE
            {
                VERTEX,
                INSTANCE,
                _MAX_COUNT
            };
        };
        using VERTEX_INPUT_RATE = VertexInputRates::RATE;

        struct PolygonModes
        {
            enum MODE
            {
                FILL,
                WIREFRAME,
                _MAX_COUNT
            };
        };
        using POLYGON_MODE = PolygonModes::MODE;

        struct CullModes
        {
            enum MODE
            {
                FRONT,
                BACK,
                FRONT_AND_BACK,
                _MAX_COUNT
            };
        };
        using CULL_MODE = CullModes::MODE;

        struct FrontFaces
        {
            enum FACE
            {
                CLOCKWISE,
                COUNTER_CLOCKWISE,
                _MAX_COUNT
            };
        };
        using FRONT_FACE = FrontFaces::FACE;

        struct CompareFunctions
        {
            enum FUNC
            {
                NEVER,
                LESS,
                EQUAL,
                LESS_EQUAL,
                GREATER,
                NOT_EQUAL,
                GREATER_EQUAL,
                ALWAYS,
                _MAX_COUNT
            };
        };
        using COMPARE_FUNCTION = CompareFunctions::FUNC;

        struct StencilOperations
        {
            enum OPERATION
            {
                KEEP,
                ZERO,
                REPLACE,
                INCREMENT_AND_CLAMP,
                DECREMENT_AND_CLAMP,
                INVERT,
                INCREMENT_AND_WRAP,
                DECREMENT_AND_WRAP,
                _MAX_COUNT
            };
        };
        using STENCIL_OPERATION = StencilOperations::OPERATION;



        struct SStencilOperationDesc
        {
            STENCIL_OPERATION   failOp;
            STENCIL_OPERATION   passOp;
            STENCIL_OPERATION   depthFailOp;
            COMPARE_FUNCTION    compareOp;
            uint32_t            compareMask;
            uint32_t            writeMask;
            uint32_t            reference;
        };

        struct LogicOperations
        {
            enum OPERATION
            {
                CLEAR,
                AND,
                AND_REVERSE,
                COPY,
                AND_INVERTED,
                NO_OPERATION,
                XOR,
                OR,
                NOR,
                EQUIVALENT,
                INVERT,
                OR_REVERSE,
                COPY_INVERTED,
                OR_INVERTED,
                NAND,
                SET,
                _MAX_COUNT
            };
        };
        using LOGIC_OPERATION = LogicOperations::OPERATION;

        struct BlendFactors
        {
            enum FACTOR
            {
                ZERO,
                ONE,
                SRC_COLOR,
                ONE_MINUS_SRC_COLOR,
                DST_COLOR,
                ONE_MINUS_DST_COLOR,
                SRC_ALPHA,
                ONE_MINUS_SRC_ALPHA,
                DST_ALPHA,
                ONE_MINUS_DST_ALPHA,
                CONSTANT_COLOR,
                ONE_MINUS_CONSTANT_COLOR,
                CONSTANT_ALPHA,
                ONE_MINUS_CONSTANT_ALPHA,
                SRC_ALPHA_SATURATE,
                SRC1_COLOR,
                ONE_MINUS_SRC1_COLOR,
                SRC1_ALPHA,
                ONE_MINUS_SRC1_ALPHA,
                _MAX_COUNT
            };
        };
        using BLEND_FACTOR = BlendFactors::FACTOR;

        struct BlendOperations
        {
            enum OPERATION
            {
                ADD,
                SUBTRACT,
                REVERSE_SUBTRACT,
                MIN,
                MAX,
                _MAX_COUNT
            };
        };
        using BLEND_OPERATION = BlendOperations::OPERATION;

        struct ColorComponents
        {
            enum COMPONENT : uint8_t
            {
                RED     = 1,
                GREEN   = 2,
                BLUE    = 4,
                ALPHA   = 8,
                ALL     = ( RED | GREEN | BLUE | ALPHA )
            };
        };
        using ColorComponent = uint8_t;

        struct SBlendState
        {
            struct SBlendFactors
            {
                BLEND_FACTOR    src = BlendFactors::ZERO;
                BLEND_FACTOR    dst = BlendFactors::ZERO;
                BLEND_OPERATION operation = BlendOperations::ADD;
            };
            
            SBlendFactors   Color;
            SBlendFactors   Alpha;
            bool            enable = false;
            ColorComponent  writeMask = ColorComponents::ALL;

        };

        struct SPipelineManagerDesc
        {
            uint32_t    maxPipelineCount = Config::RenderSystem::Pipeline::MAX_PIPELINE_COUNT;
        };

        struct PrimitiveTopologies
        {
            enum TOPOLOGY
            {

            };
        };
        using PRIMITIVE_TOPOLOGY = PrimitiveTopologies::TOPOLOGY;

        /*struct ShaderTypes
        {
            enum TYPE
            {
                VERTEX,
                HULL,
                DOMAIN,
                GEOMETRY,
                PIXEL,
                COMPUTE,
                _MAX_COUNT
            };
        };
        using SHADER_TYPE = ShaderTypes::TYPE;*/

        template<class T, uint32_t TASK_COUNT>
        struct TaskPoolHelper
        {
            using Pool = Utils::TSFreePool< T, T*, TASK_COUNT >;

            static T* GetTask(Pool* pPool)
            {
                T* pTask = nullptr;
                if (!pPool->vFreeElements.PopBack(&pTask))
                {
                    T Task;
                    uint32_t idx = pPool->vPool.PushBack(Task);
                    pTask = &pPool->vPool[idx];
                }
                return pTask;
            }
        };

        struct SPipelineDesc
        {
            SPipelineDesc()
            {
            }

            struct SShaders
            {
                ShaderPtr pVertexShader;
                ShaderPtr pTessHullShader;
                ShaderPtr pTessDomainShader;
                ShaderPtr pGeometryShader;
                ShaderPtr pPpixelShader;
                ShaderPtr pComputeShader;
            };

            struct SBlending
            {
                using BlendStateArray = Utils::TCDynamicArray< SBlendState, Config::RenderSystem::Pipeline::MAX_BLEND_STATE_COUNT >;
                BlendStateArray vBlendStates;
                LOGIC_OPERATION logicOperation = LogicOperations::NO_OPERATION;
                bool            enableLogicOperation = false;
                bool            enable = false;
            };

            struct SViewport
            {
                using ViewportArray = Utils::TCDynamicArray< SViewportDesc, Config::RenderSystem::Pipeline::MAX_VIEWPORT_COUNT >;
                using ScissorArray = Utils::TCDynamicArray< SScissorDesc, Config::RenderSystem::Pipeline::MAX_SCISSOR_COUNT >;
                ViewportArray   vViewports;
                ScissorArray    vScissors;
                bool            enable = false;
            };

            struct SRasterization
            {
                struct
                {
                    POLYGON_MODE    mode = PolygonModes::FILL;
                    CULL_MODE       cullMode = CullModes::BACK;
                    FRONT_FACE      frontFace = FrontFaces::COUNTER_CLOCKWISE;
                } Polygon;

                struct
                {
                    float   biasConstantFactor = 0.0f;
                    float   biasClampFactor = 0.0f;
                    float   biasSlopeFactor = 0.0f;
                    bool    enableClamp = false;
                } Depth;
            };

            struct SMultisampling
            {
                SAMPLE_COUNT    sampleCount = SampleCounts::SAMPLE_1;
                bool            enable = false;
            };

            struct SDepthStencil
            {
                bool                    enable = false;
                bool                    enableDepthTest = true;
                bool                    enableDepthWrite = true;
                bool                    enableStencilTest = false;
                bool                    enableStencilWrite = false;
                COMPARE_FUNCTION        depthFunction = CompareFunctions::LESS_EQUAL;
                SStencilOperationDesc   FrontFace;
                SStencilOperationDesc   BackFace;
                struct
                {
                    bool        enable = false;
                    float       min = 0.0f;
                    float       max = 0.0f;
                } DepthBounds;
            };

            struct SInputLayout
            {
                struct SVertexAttribute
                {
                    cstr_t              pName = "";
                    FORMAT              format;
                    uint16_t            binding;
                    uint16_t            location;
                    uint16_t            offset;
                    uint16_t            stride;
                    VERTEX_INPUT_RATE   inputRate = VertexInputRates::VERTEX;
                };
                using SVertexAttributeArray = Utils::TCDynamicArray< SVertexAttribute, Config::RenderSystem::Pipeline::MAX_VERTEX_ATTRIBUTE_COUNT >;
                SVertexAttributeArray   vVertexAttributes;
                PRIMITIVE_TOPOLOGY      topology;
                bool                    enablePrimitiveRestart = false;
            };

            struct STesselation
            {
                bool enable = false;
            };

            SShaders        Shaders;
            SBlending       Blending;
            SRasterization  Rasterization;
            SViewport       Viewport;
            SMultisampling  Multisampling;
            SDepthStencil   DepthStencil;
            SInputLayout    InputLayout;
            STesselation    Tesselation;
        };

        struct SPipelineCreateDesc
        {
            SResourceCreateDesc     Create;
            SPipelineDesc           Pipeline;
        };

        struct IndexTypes
        {
            enum TYPE
            {
                UINT16,
                UINT32,
                _MAX_COUNT
            };
        };
        using INDEX_TYPE = IndexTypes::TYPE;

    } // RenderSystem

    using SRenderSystemDesc = RenderSystem::SRenderSystemDesc;

} // VKE
