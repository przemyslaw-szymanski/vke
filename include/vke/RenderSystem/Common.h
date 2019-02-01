#pragma once

#include "Core/VKECommon.h"
#include "RenderSystem/Vulkan/VKEImageFormats.h"
//#include "Core/Platform/CWindow.h"
#include "Core/VKEForwardDeclarations.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/TCConstantArray.h"
#include "Core/Utils/TCString.h"
#include "Core/Memory/Common.h"
#include "Core/Utils/CLogger.h"
#include "Core/Resources/CResource.h"
#include "Config.h"
#include "RenderSystem/CDDITypes.h"

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
        /*struct TextureTag {};
        struct TextureViewTag {};
        struct RenderTargetTag {};
        struct SamplerTag {};
        struct RenderPassTag {};
        struct RenderingPipelineTag {};
        struct FramebufferTag {};
        struct ShaderTag {};
        struct ShaderProgramTag {};*/

        //using TextureHandle = _STagHandle< TextureTag >;
        //using TextureViewHandle = _STagHandle< TextureViewTag >;
        //using RenderTargetHandle = _STagHandle< RenderTargetTag >;
        //using RenderPassHandle = _STagHandle< RenderPassTag >;
        //using RenderingPipelineHandle = _STagHandle< RenderingPipelineTag >;
        //using SamplerHandle = _STagHandle< SamplerTag >;
        //using FramebufferHandle = _STagHandle< FramebufferTag >;
        //using ShaderHandle = _STagHandle< ShaderTag >;
        //using ShaderProgramHandle = _STagHandle< ShaderProgramTag >;

        template<uint32_t DEFAULT_COUNT = 32>
        using UintVec = Utils::TCDynamicArray< uint32_t, DEFAULT_COUNT >;

#define VKE_DECLARE_HANDLE(_name) \
    struct _name##Tag {}; \
    using _name##Handle = _STagHandle< _name##Tag >

        VKE_DECLARE_HANDLE( Pipeline );
        VKE_DECLARE_HANDLE( PipelineLayout );
        VKE_DECLARE_HANDLE( DescriptorSet );
        VKE_DECLARE_HANDLE( DescriptorSetLayout );
        VKE_DECLARE_HANDLE( Buffer );
        VKE_DECLARE_HANDLE( Texture );
        VKE_DECLARE_HANDLE( TextureView );
        VKE_DECLARE_HANDLE( BufferView );
        VKE_DECLARE_HANDLE( RenderPass );
        VKE_DECLARE_HANDLE( Sampler );
        VKE_DECLARE_HANDLE( Framebuffer );
        VKE_DECLARE_HANDLE( Shader );
        VKE_DECLARE_HANDLE( RenderTarget );

        struct SAPIAppInfo
        {
            uint32_t    applicationVersion;
            uint32_t    engineVersion;
            cstr_t      pEngineName;
            cstr_t      pApplicationName;
        };

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

        struct VKE_API SDepthStencilValue
        {
            float       depth;
            uint32_t    stencil;
        };

        struct SClearValue
        {
            SColor              Color;
            SDepthStencilValue  DepthStencil;
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

        /*struct SRenderPassInfo
        {
            using ClearValueArray = Utils::TCDynamicArray< SClearValue, 8 >;

            ClearValueArray     vClearValues;
            RenderPassHandle    hRenderPass;
            FramebufferHandle   hFramebuffer;
            ExtentU16           Size;
            PIPELINE_TYPE       type;
        };*/

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
                BUFFER,
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
            handle_t        hDDIAdapter;
        };

        struct SComputeContextDesc
        {

        };

        struct PresentModes
        {
            enum MODE
            {
                UNDEFINED,
                IMMEDIATE,
                MAILBOX,
                FIFO,
                _MAX_COUNT
            };
        };
        using PRESENT_MODE = PresentModes::MODE;

        struct ColorSpaces
        {
            enum COLOR_SPACE
            {
                SRGB,
                _MAX_COUNT
            };
        };
        using COLOR_SPACE = ColorSpaces::COLOR_SPACE;

        struct SPresentSurfaceFormat
        {
            FORMAT      format;
            COLOR_SPACE colorSpace;
        };

        struct SSwapChainDesc
        {
            WindowPtr           pWindow = WindowPtr();
            CGraphicsContext*   pCtx = nullptr;
            void*               pPrivate = nullptr;
            uint32_t            queueFamilyIndex = 0;
            ExtentU16           Size = { 800, 600 };
            COLOR_SPACE         colorSpace = ColorSpaces::SRGB;
            TEXTURE_FORMAT      format = Formats::UNDEFINED;
            PRESENT_MODE        mode = PresentModes::UNDEFINED;
            uint16_t            elementCount = Constants::OPTIMAL;
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

        struct PipelineStages
        {
            enum TYPE
            {
                VERTEX = VKE_BIT(1),
                TESS_HULL = VKE_BIT(2),
                TESS_DOMAIN = VKE_BIT(3),
                GEOMETRY = VKE_BIT(4),
                PIXEL = VKE_BIT(5),
                COMPUTE = VKE_BIT(6),
                _MAX_COUNT = 6
            };
        };
        using PIPELINE_STAGES = uint32_t;

        struct BindingTypes
        {
            enum TYPE
            {
                SAMPLER,
                COMBINED_IMAGE_SAMPLER,
                SAMPLED_TEXTURE,
                STORAGE_TEXTURE,
                UNIFORM_TEXEL_BUFFER,
                STORAGE_TEXEL_BUFFER,
                UNIFORM_BUFFER,
                STORAGE_BUFFER,
                UNIFORM_BUFFER_DYNAMIC,
                STORAGE_BUFFER_DYNAMIC,
                INPUT_ATTACHMENT,
                _MAX_COUNT
            };
        };
        using BINDING_TYPE = BindingTypes::TYPE;
        using DESCRIPTOR_SET_TYPE = BINDING_TYPE;
        using DescriptorSetTypes = BindingTypes;
        using DescriptorSetCounts = uint16_t[ DescriptorSetTypes::_MAX_COUNT ];

        struct SDescriptorSetLayoutDesc
        {
            struct Binding
            {
                uint32_t        idx = 0;
                BINDING_TYPE    type = BindingTypes::SAMPLED_TEXTURE;
                uint32_t        count = 1;
                PIPELINE_STAGES stages = PipelineStages::VERTEX;
            };

            using BindingArray = Utils::TCDynamicArray< Binding, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_BINDING_COUNT >;
            
            SDescriptorSetLayoutDesc() {}
            SDescriptorSetLayoutDesc(DEFAULT_CTOR_INIT)
            {
                vBindings.PushBack({});
            }
            
            BindingArray    vBindings;
        };

        struct SGraphicsContextCallbacks
        {
            std::function<void(CGraphicsContext*)> RenderFrame;
        };

        struct SGraphicsContextDesc
        {
            SSwapChainDesc              SwapChainDesc;
            void*                       pPrivate = nullptr;
        };

        struct SDeviceContextDesc
        {
            const SAdapterInfo* pAdapterInfo = nullptr;
            const void*         pPrivate = nullptr;
            DescriptorSetCounts aMaxDescriptorSetCounts = { 0 };
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
            using AttachmentArray = Utils::TCDynamicArray< TextureViewHandle, 8 >;
            ExtentU16           Size;
            AttachmentArray     vAttachments;
            RenderPassHandle    hRenderPass;
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
                FILE_IO                     = VKE_BIT( 7 ),
                _MAX_COUNT                  = 7
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

        struct SCreateTextureDesc
        {
            SCreateResourceDesc Create;
            STextureDesc        Texture;
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

        struct SCreateTextureViewDesc
        {
            SCreateResourceDesc Create;
            STextureViewDesc    TextureView;
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
                TextureViewHandle               hTextureView = NULL_HANDLE;
                TEXTURE_LAYOUT                  beginLayout = TextureLayouts::UNDEFINED;
                TEXTURE_LAYOUT                  endLayout = TextureLayouts::UNDEFINED;
                RENDER_PASS_ATTACHMENT_USAGE    usage = RenderPassAttachmentUsages::UNDEFINED;
                SColor                          ClearColor = SColor::ONE;
                TEXTURE_FORMAT                  format = Formats::UNDEFINED;
                SAMPLE_COUNT                    sampleCount = SampleCounts::SAMPLE_1;

                VKE_RENDER_SYSTEM_DEBUG_NAME;
            };

            using SubpassDescArray = Utils::TCDynamicArray< SSubpassDesc, 8 >;
            using AttachmentDescArray = Utils::TCDynamicArray< SAttachmentDesc, 8 >;

            struct SRenderPassDesc() {}
            struct SRenderPassDesc( DEFAULT_CTOR_INIT ) :
                Size( 800, 600 )
            {
            }

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

        struct ShaderStates
        {
            enum STATE : uint8_t
            {
                UNKNOWN,
                HIGH_LEVEL_TEXT, // e.g. hlsl/glsl text format
                COMPILED_IR_TEXT, // e.g. spirv text format
                COMPILED_IR_BINARY, // e.g. spv/dxbc binary format
                _MAX_COUNT
            };
        };
        using SHADER_STATE = ShaderStates::STATE;

        struct SShaderData
        {
            uint32_t            codeSize;
            SHADER_TYPE         type;
            SHADER_STATE        state;
            const uint8_t*      pCode;
        };

        struct SShaderDesc
        {
            using IncludeString = Utils::TCString< char, Config::RenderSystem::Shader::MAX_INCLUDE_PATH_LENGTH >;
            using PreprocessorString = Utils::TCString< char, Config::RenderSystem::Shader::MAX_PREPROCESSOR_DIRECTIVE_LENGTH >;
            
            using IncStringArray = Utils::TCDynamicArray< IncludeString >;
            using PrepStringArray = Utils::TCDynamicArray< PreprocessorString >;
            
            SResourceDesc   Base;
            SHADER_TYPE     type;
            cstr_t          pEntryPoint = "main";
            IncStringArray  vIncludes;
            PrepStringArray vPreprocessor;
            SShaderData*    pData = nullptr; // optional parameter if an application wants to use its own binaries
            
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
                pData = Other.pData;
                return *this;
            }

            SShaderDesc& operator=(SShaderDesc&& Other)
            {
                Base = Other.Base;
                type = Other.type;
                pEntryPoint = Other.pEntryPoint;
                vIncludes = std::move(Other.vIncludes);
                vPreprocessor = std::move(Other.vPreprocessor);
                pData = std::move( Other.pData );
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
            uint32_t    maxPipelineLayoutCount = Config::RenderSystem::Pipeline::MAX_PIPELINE_LAYOUT_COUNT;
        };

        struct PrimitiveTopologies
        {
            enum TOPOLOGY
            {
                POINT_LIST,
                LINE_LIST,
                LINE_STRIP,
                TRIANGLE_LIST,
                TRIANGLE_STRIP,
                TRIANGLE_FAN,
                _MAX_COUNT
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
            struct SShaders
            {
                //ShaderHandle    aStages[ ShaderTypes::_MAX_COUNT ] = { };
                Utils::TCConstantArray< ShaderHandle, ShaderTypes::_MAX_COUNT > aStages;

                SShaders()
                {
                    aStages.Resize( ShaderTypes::_MAX_COUNT, NULL_HANDLE );
                }
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
                    SVertexAttribute() {}
                    SVertexAttribute(DEFAULT_CTOR_INIT) :
                        pName( "" ), format{ Formats::R32G32B32A32_SFLOAT }, binding{ 0 }, location{ 0 },
                        offset{ 0 }, stride{ 0 }, inputRate{ VertexInputRates::VERTEX }
                    {}

                    cstr_t              pName = "";
                    FORMAT              format = Formats::R32G32B32A32_SFLOAT;
                    uint16_t            binding = 0;
                    uint16_t            location = 0;
                    uint16_t            offset = 0;
                    uint16_t            stride = 0;
                    VERTEX_INPUT_RATE   inputRate = VertexInputRates::VERTEX;
                };
                using SVertexAttributeArray = Utils::TCDynamicArray< SVertexAttribute, Config::RenderSystem::Pipeline::MAX_VERTEX_ATTRIBUTE_COUNT >;

                SInputLayout() {}
                SInputLayout(DEFAULT_CTOR_INIT) :
                    topology{ PrimitiveTopologies::TRIANGLE_LIST }, enablePrimitiveRestart{ false }
                {
                    vVertexAttributes.PushBack( DEFAULT_CONSTRUCTOR_INIT );
                }

                SVertexAttributeArray   vVertexAttributes;
                PRIMITIVE_TOPOLOGY      topology = PrimitiveTopologies::TRIANGLE_LIST;
                bool                    enablePrimitiveRestart = false;
            };

            struct STesselation
            {
                bool enable = false;
            };

            SShaders                Shaders;
            SBlending               Blending;
            SRasterization          Rasterization;
            SViewport               Viewport;
            SMultisampling          Multisampling;
            SDepthStencil           DepthStencil;
            SInputLayout            InputLayout;
            STesselation            Tesselation;
            PipelineLayoutHandle    hLayout = NULL_HANDLE;
            RenderPassHandle        hRenderPass = NULL_HANDLE;
        };

        struct SPipelineCreateDesc
        {
            SCreateResourceDesc     Create;
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

        struct SShaderCreateDesc
        {
            SCreateResourceDesc Create;
            SShaderDesc         Shader;

            SShaderCreateDesc() {}
            SShaderCreateDesc(const SShaderCreateDesc& Other) :
                Create{ Other.Create }
                , Shader{ Other.Shader }
            {
            }

            SShaderCreateDesc(SShaderCreateDesc&& Other) = default;

            SShaderCreateDesc& operator=(const SShaderCreateDesc& Other)
            {
                Create = Other.Create;
                Shader = Other.Shader;
                return *this;
            }

            SShaderCreateDesc& operator=(SShaderCreateDesc&& Other) = default;
        };

        struct BufferTypes
        {
            enum TYPE
            {
                VERTEX,
                INDEX,
                _MAX_COUNT
            };
        };
        using BUFFER_TYPE = BufferTypes::TYPE;

        struct BufferUsages
        {
            enum BITS
            {
                TRANSFER_SRC            = VKE_BIT( 1 ),
                TRANSFER_DST            = VKE_BIT( 2 ),
                UNIFORM_TEXEL_BUFFER    = VKE_BIT( 3 ),
                STORAGE_TEXEL_BUFFER    = VKE_BIT( 4 ),
                UNIFORM_BUFFER          = VKE_BIT( 5 ),
                STORAGE_BUFFER          = VKE_BIT( 6 ),
                INDEX_BUFFER            = VKE_BIT( 7 ),
                VERTEX_BUFFER           = VKE_BIT( 8 ),
                INDIRECT_BUFFER         = VKE_BIT( 9 )
            };
        };
        using BufferUsageBits = BufferUsages::BITS;
        using BUFFER_USAGE = uint32_t;

        struct SBufferDesc
        {
            BUFFER_USAGE    usage;
            uint32_t        size;
        };

        struct SVertexBufferDesc
        {
            SBufferDesc     BaseDesc;
        };

        struct SIndexBufferDesc
        {
            SBufferDesc     BaseDesc;
            INDEX_TYPE      indexType;
        };

        struct SBufferViewDesc
        {
            BufferHandle    hBuffer;
            size_t          offset;
            FORMAT          format;
        };

        struct SCreateBufferDesc
        {
            SCreateResourceDesc Create;
            SBufferDesc         Buffer;
        };

        struct SSemaphoreDesc
        {

        };

        struct SFenceDesc
        {
            bool    isSignaled = true;
        };

        struct CommandBufferLevels
        {
            enum LEVEL
            {
                PRIMARY,
                SECONDARY,
                _MAX_COUNT
            };
        };
        using COMMAND_BUFFER_LEVEL = CommandBufferLevels::LEVEL;

        

        struct SPresentSurfaceDesc
        {
            handle_t        hProcess;
            handle_t        hWindow;
            uint32_t        queueFamilyIndex;
        };   

        struct SPresentSurfaceCaps
        {
            using Formats = Utils::TCDynamicArray < SPresentSurfaceFormat >;
            using Modes = Utils::TCDynamicArray< PRESENT_MODE, 8 >;

            Formats     vFormats;
            Modes       vModes;
            ExtentU32   MinSize;
            ExtentU32   MaxSize;
            ExtentU32   CurrentSize;
            uint32_t    minImageCount;
            uint32_t    maxImageCount;
            bool        canBeUsedAsRenderTarget;
        };

        struct SRenderPassInfo
        {
            using ClearValueArray = Utils::TCDynamicArray< SClearValue >;
            ClearValueArray     vClearValues;
            RenderPassHandle    hPass;
            FramebufferHandle   hFramebuffer;
            ExtentU16           Size;
        };

        struct SCommandBufferInfo
        {

        };

        struct PipelineTypes
        {
            enum TYPE
            {
                GRAPHICS,
                COMPUTE,
                _MAX_COUNT
            };
        };
        using PIPELINE_TYPE = PipelineTypes::TYPE;

        struct SPipelineLayoutDesc
        {
            static const auto MAX_COUNT = Config::RenderSystem::Pipeline::MAX_PIPELINE_LAYOUT_DESCRIPTOR_SET_COUNT;
            using DescSetLayoutArray = Utils::TCDynamicArray< DescriptorSetLayoutHandle, MAX_COUNT >;

            SPipelineLayoutDesc() {}
            SPipelineLayoutDesc( DescriptorSetLayoutHandle hLayout )
            {
                vDescriptorSetLayouts.PushBack( hLayout );
            }

            DescSetLayoutArray  vDescriptorSetLayouts;
        };

        struct SDDISwapChain
        {
            using ImageArray = Utils::TCDynamicArray< DDITexture, 3 >;
            using ImageViewArray = Utils::TCDynamicArray< DDITextureView, 3 >;

            ImageArray              vImages;
            ImageViewArray          vImageViews;
            DDIPresentSurface       hSurface = VK_NULL_HANDLE;
            DDISwapChain            hSwapChain = VK_NULL_HANDLE;
            ExtentU32               Size;
            SPresentSurfaceFormat   Format;
            PRESENT_MODE            mode;
        };

        struct SDDIGetBackBufferInfo
        {
            uint64_t        waitTimeout = UINT64_MAX;
            DDISemaphore    hAcquireSemaphore;
            DDIFence        hFence = DDI_NULL_HANDLE;
        };

        struct SDDILoadInfo
        {
            SAPIAppInfo     AppInfo;
        };

        using AdapterInfoArray = Utils::TCDynamicArray< RenderSystem::SAdapterInfo >;

        struct SSubmitInfo
        {
            DDISemaphore*       pDDISignalSemaphores = nullptr;
            DDISemaphore*       pDDIWaitSemaphores = nullptr;
            CommandBufferPtr*   pCommandBuffers = nullptr;
            DDIFence            hDDIFence = DDI_NULL_HANDLE;
            DDIQueue            hDDIQueue = DDI_NULL_HANDLE;
            uint8_t             signalSemaphoreCount = 0;
            uint8_t             waitSemaphoreCount = 0;
            uint8_t             commandBufferCount = 0;
        };

        struct SPresentInfo
        {
            using UintArray = Utils::TCDynamicArray< uint32_t, 8 >;
            using SemaphoreArray = Utils::TCDynamicArray< DDISemaphore, 8 >;
            using SwapChainArray = Utils::TCDynamicArray< DDISwapChain, 8 >;
            SwapChainArray      vSwapchains;
            SemaphoreArray      vWaitSemaphores;
            UintArray           vImageIndices;
            DDIQueue            hQueue = DDI_NULL_HANDLE;
        };

        struct SCommandBufferPoolDesc
        {
            uint32_t    queueFamilyIndex;
        };

        struct SMemoryAllocateData
        {
            DDIMemory   hMemory = DDI_NULL_HANDLE;
            uint32_t    alignment = 0;
            uint32_t    size = 0;
        };

        struct SBindMemoryInfo
        {
            DDITexture    hImage = DDI_NULL_HANDLE;
            DDIBuffer   hBuffer = DDI_NULL_HANDLE;
            DDIMemory   hMemory = DDI_NULL_HANDLE;
            uint32_t    offset = 0;
        };

        struct SBindPipelineInfo
        {
            CCommandBuffer*     pCmdBuffer;
            PipelinePtr         pPipeline;
        };

        struct SBindRenderPassInfo
        {
            CCommandBuffer*     pCmdBuffer;
            SRenderPassInfo*    pRenderPassInfo;
        };

        struct SBindVertexBufferInfo
        {
            CCommandBuffer*     pCmdBuffer;
            CVertexBuffer*      pBuffer;
            size_t              offset;
        };

        struct SBindIndexBufferInfo
        {
            CCommandBuffer*     pCmdBuffer;
            CIndexBuffer*       pBuffer;
            size_t              offset;
        };

        struct SBindDescriptorSetsInfo
        {
            CCommandBuffer*     pCmdBuffer;
            DDIDescriptorSet*   aDDISetHandles;
            uint32_t*           aDynamicOffsets = nullptr;
            CPipelineLayout*    pPipelineLayout;
            uint16_t            firstSet;
            uint16_t            setCount;
            uint16_t            dynamicOffsetCount = 0;
            PIPELINE_TYPE       type;
        };

        struct SDDISwapChainDesc
        {
            ExtentU32           Size = { 800, 600 };
            uint32_t            queueFamilyIndex = 0;
            COLOR_SPACE         colorSpace = ColorSpaces::SRGB;
            TEXTURE_FORMAT      format = Formats::R8G8B8A8_UNORM;
            PRESENT_MODE        mode = PresentModes::FIFO;
            uint16_t            elementCount = Constants::OPTIMAL;
        };

        struct SAllocateCommandBufferInfo
        {
            DDICommandBufferPool    hDDIPool;
            uint32_t                count;
            COMMAND_BUFFER_LEVEL    level;
        };

        struct SFreeCommandBufferInfo
        {
            DDICommandBufferPool    hDDIPool;
            DDICommandBuffer*       pDDICommandBuffers;
            uint32_t                count;
        };

        struct SDescriptorPoolDesc
        {
            struct SSize
            {
                DESCRIPTOR_SET_TYPE type;
                uint32_t            count;
            };
            using SizeVec = Utils::TCDynamicArray< SSize >;

            uint32_t    maxSetCount;
            SizeVec     vPoolSizes;
        };

        struct QueueTypes
        {
            enum TYPE : uint8_t
            {
                GRAPHICS = VKE_BIT( 0 ),
                COMPUTE = VKE_BIT( 1 ),
                TRANSFER = VKE_BIT( 2 ),
                SPARSE = VKE_BIT( 3 ),
                PRESENT = VKE_BIT( 4 ),
                ALL = GRAPHICS | COMPUTE | TRANSFER | SPARSE | PRESENT,
                _MAX_COUNT = 6
            };
        };
        using QUEUE_TYPE = uint8_t;
        using QueueTypeBits = QueueTypes;

#define VKE_ADD_DDI_OBJECT(_type) \
        protected: _type  m_hDDIObject = DDI_NULL_HANDLE; \
        public: vke_force_inline const _type& GetDDIObject() const { return m_hDDIObject; }

    } // RenderSystem

    using SRenderSystemDesc = RenderSystem::SRenderSystemDesc;

} // VKE
