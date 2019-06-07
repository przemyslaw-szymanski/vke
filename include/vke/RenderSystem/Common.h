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

#define VKE_RENDER_SYSTEM_SET_DEBUG_NAME(_obj, _name) VKE_DEBUG_CODE(_obj.pDebugName = _name)

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

        using StringVec = Utils::TCDynamicArray< vke_string >;
        using CStrVec = Utils::TCDynamicArray< cstr_t >;

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
        VKE_DECLARE_HANDLE( VertexBuffer );
        VKE_DECLARE_HANDLE( IndexBuffer );
        VKE_DECLARE_HANDLE( Texture );
        VKE_DECLARE_HANDLE( TextureView );
        VKE_DECLARE_HANDLE( BufferView );
        VKE_DECLARE_HANDLE( RenderPass );
        VKE_DECLARE_HANDLE( Sampler );
        VKE_DECLARE_HANDLE( Framebuffer );
        VKE_DECLARE_HANDLE( Shader );
        VKE_DECLARE_HANDLE( RenderTarget );
        VKE_DECLARE_HANDLE( Event );

        struct SAPIAppInfo
        {
            uint32_t    applicationVersion;
            uint32_t    engineVersion;
            cstr_t      pEngineName;
            cstr_t      pApplicationName;
        };

        using TextureSizeType = uint16_t;
        using TextureSize = TSExtent< TextureSizeType >;
        using BufferSizeType = uint32_t;

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

            explicit SColor(uint32_t v) :
                r(v), g(v), b(v), a(v)
            {}
            SColor(float red, float green, float blue, float alpha) :
                r(red), g(green), b(blue), a(alpha) {}
            explicit SColor(float v) :
                r(v), g(v), b(v), a(v) {}

            void operator=( float v )
            {
                r = g = b = a = v;
            }

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
            SDepthStencilValue() {}
            SDepthStencilValue( float d, uint32_t s ) : depth{ d }, stencil{ s } {}
            float       depth;
            uint32_t    stencil;
        };

        struct VKE_API SClearValue
        {
            union
            {
                SColor              Color;
                SDepthStencilValue  DepthStencil;
            };

            SClearValue() {}
            SClearValue( const SColor& C ) : Color{ C } {}
            SClearValue( const SDepthStencilValue& DS ) : DepthStencil{ DS } {}
            SClearValue( float r, float g, float b, float a ) : Color( r, g, b, a ) {}
            SClearValue( float d, uint32_t s ) : DepthStencil( d, s ) {}
        };

        struct SViewportDesc
        {
            ExtentF32   Position;
            ExtentF32   Size;
            ExtentF32   MinMaxDepth;

            uint32_t CalcHash() const
            {
                SHash Hash;
                Hash.Combine( Position.x, Position.y, Size.width, Size.height, MinMaxDepth.min, MinMaxDepth.max );
                return static_cast<uint32_t>( Hash.value );
            }
        };

        struct SScissorDesc
        {
            ExtentI32   Position;
            ExtentU32   Size;

            uint32_t CalcHash() const
            {
                SHash Hash;
                Hash.Combine( Position.x, Position.y, Size.width, Size.height );
                return static_cast<uint32_t>(Hash.value);
            }
        };

        struct SPipelineInfo
        {
            uint32_t bEnableDepth : 1;
            uint32_t bEnableStencil : 1;
            uint32_t bEnableBlending : 1;
        };

        template<typename T>
        struct TSRect2D
        {
            TSExtent< T >   Size;
            TSExtent< T >   Offset;
        };

        using DrawRect = TSRect2D< TextureSizeType >;

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
                UNKNOWN,
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
            TextureSize         Size = { 800, 600 };
            COLOR_SPACE         colorSpace = ColorSpaces::SRGB;
            TEXTURE_FORMAT      format = Formats::UNDEFINED;
            uint16_t            elementCount = Constants::OPTIMAL;
            bool                enableVSync = true;
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
            enum TYPE : uint16_t
            {
                VERTEX = VKE_BIT(1),
                TS_HULL = VKE_BIT(2),
                TS_DOMAIN = VKE_BIT(3),
                GEOMETRY = VKE_BIT(4),
                PIXEL = VKE_BIT(5),
                MS_TASK = VKE_BIT(6),
                MS_MESH = VKE_BIT(7),
                RT_RAYGEN = VKE_BIT(8),
                RT_ANY_HIT = VKE_BIT(9),
                RT_CLOSEST_HIT = VKE_BIT(10),
                RT_MISS_HIT = VKE_BIT(11),
                RT_INTERSECTION = VKE_BIT(12),
                RT_CALLABLE = VKE_BIT(13),
                COMPUTE = VKE_BIT(14),
                _MAX_COUNT = 14,
                MESH = MS_TASK | MS_MESH,
                RAYTRACING = RT_RAYGEN | RT_ANY_HIT | RT_CLOSEST_HIT | RT_MISS_HIT | RT_INTERSECTION | RT_CALLABLE,
                ALL = VERTEX | TS_HULL | TS_DOMAIN | GEOMETRY | PIXEL | MESH | RAYTRACING | COMPUTE
            };
        };
        using PIPELINE_STAGES = uint16_t;

        struct BindingTypes
        {
            enum TYPE : uint8_t
            {
                SAMPLER,
                COMBINED_TEXTURE_SAMPLER,
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
        
        struct SResourceBinding
        {
            SResourceBinding() {}
            SResourceBinding( uint8_t idx ) :
                index{ idx }, set{ 0 }, stages{ PipelineStages::ALL }, count{ 1 } {}
            SResourceBinding( uint8_t idx, PIPELINE_STAGES s ) :
                index{ idx }, set{ 0 }, stages{ s }, count{ 1 } {}
            SResourceBinding( uint8_t idx, PIPELINE_STAGES s, uint16_t c ) :
                index{ idx }, set{ 0 }, stages{ s }, count{ c } {}

            uint8_t         index;
            uint8_t         set;
            PIPELINE_STAGES stages;
            uint16_t        count;
        };

        struct STextureBinding : SResourceBinding
        {
            TextureViewHandle   hTextureView;
        };

        struct SSamplerBinding : SResourceBinding
        {
            SamplerHandle   hSampler;
        };

        struct SSamplerTextureBinding : SResourceBinding
        {
            SamplerHandle       hSampler;
            TextureViewHandle   hTextureView;
        };

        struct SDescriptorSetLayoutDesc
        {
            struct SBinding
            {
                uint8_t         idx;
                BINDING_TYPE    type;
                uint16_t        count;
                PIPELINE_STAGES stages;
            };

            using BindingArray = Utils::TCDynamicArray< SBinding, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_BINDING_COUNT >;
            
            SDescriptorSetLayoutDesc() {}
            SDescriptorSetLayoutDesc(DEFAULT_CTOR_INIT)
            {
                vBindings.PushBack({});
            }
            
            BindingArray    vBindings;
        };

        struct SUpdateBindingsInfo
        {
            template<class HandleType>
            struct TSBinding
            {
                const HandleType*   ahHandles;
                uint16_t            count;
                uint8_t             binding;
            };

            struct SBufferBinding : TSBinding<BufferHandle>
            {
                uint32_t    offset;
                uint32_t    range;
            };

            void AddBinding( uint8_t binding, const RenderTargetHandle* ahHandles, const uint32_t count )
            {
                TSBinding<RenderTargetHandle> Binding;
                Binding.ahHandles = ahHandles;
                Binding.count = count;
                Binding.binding = binding;
                vRTs.PushBack( Binding );
            }

            void AddBinding( uint8_t binding, const TextureHandle* ahHandles, const uint32_t count )
            {
                TSBinding<TextureHandle> Binding;
                Binding.ahHandles = ahHandles;
                Binding.count = count;
                Binding.binding = binding;
                vTexs.PushBack( Binding );
            }

            void AddBinding( uint8_t binding, const SamplerHandle* ahHandles, const uint32_t count )
            {
                TSBinding<SamplerHandle> Binding;
                Binding.ahHandles = ahHandles;
                Binding.count = count;
                Binding.binding = binding;
                vSamplers.PushBack( Binding );
            }

            void AddBinding( uint8_t binding, const uint32_t offset, const uint32_t range, const BufferHandle* ahHandles, const uint32_t count )
            {
                SBufferBinding Binding;
                Binding.ahHandles = ahHandles;
                Binding.count = count;
                Binding.binding = binding;
                Binding.offset = offset;
                Binding.range = range;
                vBuffers.PushBack( Binding );
            }

            template<class HandleType>
            using BindingArray = Utils::TCDynamicArray< TSBinding<HandleType>, 16 >;
            using RtArray = BindingArray< RenderTargetHandle >;
            using TexArray = BindingArray< TextureHandle >;
            using SamplerArray = BindingArray< SamplerHandle >;
            using BufferArray = Utils::TCDynamicArray< SBufferBinding, 8 >;

            RtArray         vRTs;
            TexArray        vTexs;
            SamplerArray    vSamplers;
            BufferArray     vBuffers;
        };

        struct SGraphicsContextCallbacks
        {
            std::function<void(CGraphicsContext*)> RenderFrame;
        };

        struct SCommandBufferPoolDesc
        {
            uint32_t    queueFamilyIndex = 0;
            uint32_t    commandBufferCount = Config::RenderSystem::CommandBuffer::DEFAULT_COUNT_IN_POOL;
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
            using AttachmentArray = Utils::TCDynamicArray< DDITextureView, 8 >;
            TextureSize         Size;
            AttachmentArray     vDDIAttachments;
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
        using TEXTURE_USAGE = uint8_t;

        struct TextureStates
        {
            enum STATE : uint8_t
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
        using TEXTURE_STATE = TextureStates::STATE;

        struct TextureTransitions
        {
            enum TRANSITION : uint8_t
            {
                UNDEFINED_TO_GENERAL,
                UNDEFINED_TO_COLOR_RENDER_TARGET,
                UNDEFINED_TO_DEPTH_STENCIL_RENDER_TARGET,
                UNDEFINED_TO_DEPTH_BUFFER,
                UNDEFINED_TO_SHADER_READ,
                UNDEFINED_TO_TRANSFER_SRC,
                UNDEFINED_TO_TRANSFER_DST,
                UNDEFINED_TO_PRESENT,
                
                GENERAL_TO_UNDEFINED,
                GENERAL_TO_COLOR_RENDER_TARGET,
                GENERAL_TO_DEPTH_STENCIL_RENDER_TARGET,
                GENERAL_TO_DEPTH_BUFFER,
                GENERAL_TO_SHADER_READ,
                GENERAL_TO_TRANSFER_SRC,
                GENERAL_TO_TRANSFER_DST,
                GENERAL_TO_PRESENT,
                
                COLOR_RENDER_TARGET_TO_UNDEFINED,
                COLOR_RENDER_TARGET_TO_GENERAL,
                COLOR_RENDER_TARGET_TO_SHADER_READ,
                COLOR_RENDER_TARGET_TO_TRANSFER_SRC,
                COLOR_RENDER_TARGET_TO_TRANSFER_DST,
                
                DEPTH_STENCIL_RENDER_TARGET_TO_UNDEFINED,
                DEPTH_STENCIL_RENDER_TARGET_TO_GENERAL,
                DEPTH_STENCIL_RENDER_TARGET_TO_DEPTH_BUFFER,
                DEPTH_STENCIL_RENDER_TARGET_TO_SHADER_READ,
                DEPTH_STENCIL_RENDER_TARGET_TO_TRANSFER_SRC,
                DEPTH_STENCIL_RENDER_TARGET_TO_TRANSFER_DST,

                DEPTH_BUFFER_TO_UNDEFINED,
                DEPTH_BUFFER_TO_GENERAL,
                DEPTH_BUFFER_TO_DEPTH_STENCIL_RENDER_TARGET,
                DEPTH_BUFFER_TO_SHADER_READ,
                DEPTH_BUFFER_TO_TRANSFER_SRC,
                DEPTH_BUFFER_TO_TRANSFER_DST,

                SHADER_READ_TO_UNDEFINED,
                SHADER_READ_TO_GENERAL,
                SHADER_READ_TO_COLOR_RENDER_TARGET,
                SHADER_READ_TO_DEPTH_STENCIL_RENDER_TARGET,
                SHADER_READ_TO_DEPTH_BUFFER,
                SHADER_READ_TO_TRANSFER_SRC,
                SHADER_READ_TO_TRANSFER_DST,
                SHADER_READ_TO_PRESENT,

                TRANSFER_SRC_TO_UNDEFINED,
                TRANSFER_SRC_TO_GENERAL,
                TRANSFER_SRC_TO_COLOR_RENDER_TARGET,
                TRANSFER_SRC_TO_DEPTH_STENCIL_RENDER_TARGET,
                TRANSFER_SRC_TO_DEPTH_BUFFER,
                TRANSFER_SRC_TO_SHADER_READ,
                TRANSFER_SRC_TO_TRANSFER_DST,
                TRANSFER_SRC_TO_PRESENT,

                TRANSFER_DST_TO_UNDEFINED,
                TRANSFER_DST_TO_GENERAL,
                TRANSFER_DST_TO_COLOR_RENDER_TARGET,
                TRANSFER_DST_TO_DEPTH_STENCIL_RENDER_TARGET,
                TRANSFER_DST_TO_DEPTH_BUFFER,
                TRANSFER_DST_TO_SHADER_READ,
                TRANSFER_DST_TO_TRANSFER_SRC,
                TRANSFER_DST_TO_PRESENT,

                PRESENT_TO_UNDEFINED,
                PRESENT_TO_GENERAL,
                PRESENT_TO_SHADER_READ,
                PRESENT_TO_TRANSFER_SRC,
                PRESENT_TO_TRANSFER_DST,

                _MAX_COUNT
            };
        };

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
            enum BITS : uint8_t
            {
                DEDICATED_ALLOCATION    = VKE_BIT( 1 ),
                CPU_ACCESS              = VKE_BIT( 2 ),
                CPU_NO_FLUSH            = VKE_BIT( 3 ),
                CPU_CACHED              = VKE_BIT( 4 ),
                GPU_ACCESS              = VKE_BIT( 5 ),
                BUFFER                  = VKE_BIT( 6 ),
                TEXTURE                 = VKE_BIT( 7 ),
                DYNAMIC                 = CPU_ACCESS | GPU_ACCESS,
                STATIC                  = GPU_ACCESS,
                DEFAULT                 = STATIC,
                STAGING                 = CPU_ACCESS | CPU_CACHED
            };
        };
        using MEMORY_USAGE = uint8_t;

        struct STextureSubresourceRange
        {
            uint16_t        beginMipmapLevel    = 0;
            uint16_t        mipmapLevelCount    = 1;
            uint8_t         beginArrayLayer     = 0;
            uint8_t         layerCount          = 1;
            TEXTURE_ASPECT  aspect              = TextureAspects::UNKNOWN;
        };

        struct AddressModes
        {
            enum MODE : uint8_t
            {
                REPEAT,
                MIRRORED_REPEAT,
                CLAMP_TO_EDGE,
                CLAMP_TO_BORDER,
                MIRROR_CLAMP_TO_EDGE,
                _MAX_COUNT
            };
        };
        using ADDRESS_MODE = AddressModes::MODE;

        struct SAddressMode
        {
            ADDRESS_MODE    U = AddressModes::CLAMP_TO_EDGE;
            ADDRESS_MODE    V = AddressModes::CLAMP_TO_EDGE;
            ADDRESS_MODE    W = AddressModes::CLAMP_TO_EDGE;
        };

        struct BorderColors
        {
            enum COLOR : uint8_t
            {
                FLOAT_TRANSPARENT_BLACK,
                INT_TRANSPARENT_BLACK,
                FLOAT_OPAQUE_BLACK,
                INT_OPAQUE_BLACK,
                FLOAT_OPAQUE_WHITE,
                INT_OPAQUE_WHITE,
                _MAX_COUNT
            };
        };
        using BORDER_COLOR = BorderColors::COLOR;

        struct SamplerFilters
        {
            enum FILTER : uint8_t
            {
                NEAREST,
                LINEAR,
                CUBIC_IMG,
                _MAX_COUNT
            };
        };
        using SAMPLER_FILTER = SamplerFilters::FILTER;

        struct SSamplerFilters
        {
            SAMPLER_FILTER  min = SamplerFilters::LINEAR;
            SAMPLER_FILTER  mag = SamplerFilters::LINEAR;
        };

        struct MipmapModes
        {
            enum MODE : uint8_t
            {
                NEAREST,
                LINEAR,
                _MAX_COUNT
            };
        };
        using MIPMAP_MODE = MipmapModes::MODE;

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

        struct SSamplerDesc
        {
            SAddressMode        AddressMode;
            BORDER_COLOR        borderColor = BorderColors::INT_OPAQUE_BLACK;
            COMPARE_FUNCTION    compareFunc = CompareFunctions::ALWAYS;
            SSamplerFilters     Filter;
            MIPMAP_MODE         mipmapMode = MipmapModes::LINEAR;
            ExtentF32           LOD = { 0.0f, 1.0f };
            float               maxAnisotropy = 0.0f;
            float               mipLODBias = 0.0f;
            bool                enableCompare = false;
            bool                enableAnisotropy = false;
            bool                unnormalizedCoordinates = false;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct STextureDesc
        {
            TextureSize         Size;
            TEXTURE_FORMAT      format = Formats::R8G8B8A8_UNORM;
            TEXTURE_USAGE       usage = TextureUsages::SAMPLED;
            TEXTURE_TYPE        type = TextureTypes::TEXTURE_2D;
            SAMPLE_COUNT        multisampling = SampleCounts::SAMPLE_1;
            uint16_t            mipLevelCount = 0;
            MEMORY_USAGE        memoryUsage = MemoryUsages::DEFAULT;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct SCreateTextureDesc
        {
            SCreateResourceDesc Create;
            STextureDesc        Texture;
        };

        struct STextureViewDesc
        {
            TextureHandle               hTexture = NULL_HANDLE;
            TEXTURE_VIEW_TYPE           type = TextureViewTypes::VIEW_2D;
            TEXTURE_FORMAT              format = Formats::R8G8B8A8_UNORM;
            STextureSubresourceRange    SubresourceRange;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
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
                struct VKE_API SRenderTargetDesc
                {
                    TextureViewHandle hTextureView = NULL_HANDLE;
                    TEXTURE_STATE layout = TextureStates::UNDEFINED;
                    VKE_RENDER_SYSTEM_DEBUG_NAME;
                };

                using AttachmentDescArray = Utils::TCDynamicArray< SRenderTargetDesc, 8 >;
                AttachmentDescArray vRenderTargets;
                AttachmentDescArray vTextures;
                SRenderTargetDesc DepthBuffer;
                VKE_RENDER_SYSTEM_DEBUG_NAME;
            };

            struct VKE_API SRenderTargetDesc
            {
                TextureViewHandle               hTextureView = NULL_HANDLE;
                TEXTURE_STATE                   beginLayout = TextureStates::UNDEFINED;
                TEXTURE_STATE                   endLayout = TextureStates::UNDEFINED;
                RENDER_PASS_ATTACHMENT_USAGE    usage = RenderPassAttachmentUsages::UNDEFINED;
                SClearValue                     ClearValue = { { 0,0,0,1 } };
                TEXTURE_FORMAT                  format = Formats::UNDEFINED;
                SAMPLE_COUNT                    sampleCount = SampleCounts::SAMPLE_1;

                VKE_RENDER_SYSTEM_DEBUG_NAME;
            };

            using SubpassDescArray = Utils::TCDynamicArray< SSubpassDesc, 8 >;
            using AttachmentDescArray = Utils::TCDynamicArray< SRenderTargetDesc, 8 >;

            struct SRenderPassDesc() {}
            struct SRenderPassDesc( DEFAULT_CTOR_INIT ) :
                Size( 800, 600 )
            {
            }

            AttachmentDescArray vRenderTargets;
            SubpassDescArray    vSubpasses;
            TextureSize         Size;
        };
        using SRenderPassAttachmentDesc = SRenderPassDesc::SRenderTargetDesc;
        using SSubpassAttachmentDesc = SRenderPassDesc::SSubpassDesc::SRenderTargetDesc;

        struct SRenderTargetDesc
        {
            ExtentU16       Size;
            FORMAT          format;
            MEMORY_USAGE    memoryUsage;
            TEXTURE_USAGE   usage;
            TEXTURE_TYPE    type;
            TEXTURE_STATE   beginState = TextureStates::UNDEFINED;
            TEXTURE_STATE   endState = TextureStates::UNDEFINED;
            RENDER_PASS_ATTACHMENT_USAGE clearStoreUsage;
            SAMPLE_COUNT    multisampling = SampleCounts::SAMPLE_1;
            SClearValue     ClearValue = { { 0,0,0,1 } };
            uint16_t        mipLevelCount = 1;

            cstr_t          pDebugName = "RenderTarget";
        };

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
                virtual ~IGraphicsContext(){}
                virtual bool OnRenderFrame(CGraphicsContext*) { return true; }
                virtual void OnAfterPresent(CGraphicsContext*) {}
                virtual void OnBeforePresent(CGraphicsContext*) {}
                virtual void OnBeforeExecute(CGraphicsContext*) {}
                virtual void OnAfterExecute(CGraphicsContext*) {}

                virtual bool AutoDestroy() { return true; }
            };
        } // EventListeners

        struct SDeviceMemoryManagerDesc
        {
            SMemoryPoolManagerDesc  MemoryPoolDesc;
        };

        /*struct MemoryAccessTypes
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
        using MEMORY_ACCESS_TYPE = MemoryAccessTypes::TYPE;*/

        struct MemoryAccessTypes
        {
            enum TYPE : uint64_t
            {
                UNKNOWN = 0x0,
                INDIRECT_BUFFER_READ = VKE_BIT( 1 ),
                INDEX_READ = VKE_BIT( 2 ),
                VERTEX_ATTRIBUTE_READ = VKE_BIT( 3 ),
                VS_UNIFORM_READ = VKE_BIT( 4 ),
                PS_UNIFORM_READ = VKE_BIT( 5 ),
                GS_UNIFORM_READ = VKE_BIT( 6 ),
                TS_UNIFORM_READ = VKE_BIT( 7 ),
                CS_UNIFORM_READ = VKE_BIT( 8 ),
                MS_UNIFORM_READ = VKE_BIT( 9 ),
                RT_UNIFORM_READ = VKE_BIT( 10 ),
                INPUT_ATTACHMENT_READ = VKE_BIT( 11 ),
                VS_SHADER_READ = VKE_BIT( 12 ),
                PS_SHADER_READ = VKE_BIT( 13 ),
                GS_SHADER_READ = VKE_BIT( 14 ),
                TS_SHADER_READ = VKE_BIT( 15 ),
                CS_SHADER_READ = VKE_BIT( 16 ),
                MS_SHADER_READ = VKE_BIT( 17 ),
                RS_SHADER_READ = VKE_BIT( 18 ),
                VS_SHADER_WRITE = VKE_BIT( 19 ),
                PS_SHADER_WRITE = VKE_BIT( 20 ),
                GS_SHADER_WRITE = VKE_BIT( 21 ),
                TS_SHADER_WRITE = VKE_BIT( 22 ),
                CS_SHADER_WRITE = VKE_BIT( 23 ),
                MS_SHADER_WRITE = VKE_BIT( 24 ),
                RS_SHADER_WRITE = VKE_BIT( 25 ),
                COLOR_ATTACHMENT_READ = VKE_BIT( 26 ),
                COLOR_ATTACHMENT_WRITE = VKE_BIT( 27 ),
                DEPTH_STENCIL_ATTACHMENT_READ = VKE_BIT( 28 ),
                DEPTH_STENCIL_ATTACHMENT_WRITE = VKE_BIT( 29 ),
                DATA_TRANSFER_READ = VKE_BIT( 30 ),
                DATA_TRANSFER_WRITE = VKE_BIT( 31 ),
                CPU_MEMORY_READ = VKE_BIT( 32 ),
                CPU_MEMORY_WRITE = VKE_BIT( 33 ),
                GPU_MEMORY_READ = VKE_BIT( 34 ),
                GPU_MEMORY_WRITE = VKE_BIT( 35 ),
                _MAX_COUNT = 36,
                UNIFORM_READ = VS_UNIFORM_READ | PS_UNIFORM_READ,
                SHADER_READ = VS_SHADER_READ | PS_SHADER_READ,
                SHADER_WRITE = VS_SHADER_WRITE | PS_SHADER_WRITE
            };
        };
        using MEMORY_ACCESS_TYPE = uint64_t;

        using BufferStates = MemoryAccessTypes;
        using BUFFER_STATE = MEMORY_ACCESS_TYPE;

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
            SHADER_TYPE         type = ShaderTypes::_MAX_COUNT;
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
            SHADER_TYPE     type = ShaderTypes::_MAX_COUNT;
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
                NONE,
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
            STENCIL_OPERATION   failOp = StencilOperations::KEEP;
            STENCIL_OPERATION   passOp = StencilOperations::KEEP;
            STENCIL_OPERATION   depthFailOp = StencilOperations::KEEP;
            COMPARE_FUNCTION    compareOp = CompareFunctions::ALWAYS;
            uint32_t            compareMask = 255;
            uint32_t            writeMask = 255;
            uint32_t            reference = 0;
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
                BLEND_FACTOR    src = BlendFactors::ONE;
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
            using Pool = Utils::TSFreePool< T, uint32_t, TASK_COUNT >;

            static T* GetTask(Pool* pPool)
            {
                T* pTask = nullptr;
                uint32_t idx;
                if( pPool->vFreeElements.PopBack( &idx ) )
                {
                    pTask = &pPool->vPool[idx];
                }
                else
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
            SPipelineDesc() {}
            SPipelineDesc( DEFAULT_CTOR_INIT ) :
                Blending{DEFAULT_CONSTRUCTOR_INIT}
                , DepthStencil{ DEFAULT_CONSTRUCTOR_INIT }
                , InputLayout{ DEFAULT_CONSTRUCTOR_INIT }
                , Multisampling{ DEFAULT_CONSTRUCTOR_INIT }
                , Rasterization{ DEFAULT_CONSTRUCTOR_INIT }
                , Shaders{ DEFAULT_CONSTRUCTOR_INIT }
                , Viewport{ DEFAULT_CONSTRUCTOR_INIT }
            {
            }

            struct SShaders
            {   
                /*ShaderHandle    apShaders[ShaderTypes::_MAX_COUNT];

                SShaders( DEFAULT_CTOR_INIT ) : SShaders() {}
                SShaders() :
                    apShaders{ { NULL_HANDLE }, { NULL_HANDLE }, { NULL_HANDLE }, { NULL_HANDLE }, { NULL_HANDLE }, { NULL_HANDLE } }
                {
                }*/
                ShaderPtr   apShaders[ ShaderTypes::_MAX_COUNT ];
                SShaders( DEFAULT_CTOR_INIT ) : SShaders()
                {}
                SShaders()
                {}
            };

            struct SBlending
            {
                SBlending() {}
                SBlending( DEFAULT_CTOR_INIT )
                {
                    SBlendState State;
                    vBlendStates.PushBack( State );
                }

                using BlendStateArray = Utils::TCDynamicArray< SBlendState, Config::RenderSystem::Pipeline::MAX_BLEND_STATE_COUNT >;
                BlendStateArray vBlendStates;
                LOGIC_OPERATION logicOperation = LogicOperations::NO_OPERATION;
                bool            enable = false;
            };

            struct SViewport
            {
                SViewport() {}
                SViewport( DEFAULT_CTOR_INIT )
                {
                    SViewportDesc Desc;
                    Desc.MinMaxDepth = {0,1};
                    Desc.Position = { 0,0 };
                    Desc.Size = { 800, 600 };
                    vViewports.PushBack( Desc );
                    SScissorDesc Scissor;
                    Scissor.Position = { 0,0 };
                    Scissor.Size = { 800, 600 };
                    vScissors.PushBack( Scissor );
                }

                using ViewportArray = Utils::TCDynamicArray< SViewportDesc, Config::RenderSystem::Pipeline::MAX_VIEWPORT_COUNT >;
                using ScissorArray = Utils::TCDynamicArray< SScissorDesc, Config::RenderSystem::Pipeline::MAX_SCISSOR_COUNT >;
                ViewportArray   vViewports;
                ScissorArray    vScissors;
                bool            enable = true;
            };

            struct SRasterization
            {
                SRasterization() {}
                SRasterization( DEFAULT_CTOR_INIT ) {}

                struct
                {
                    POLYGON_MODE    mode = PolygonModes::FILL;
                    CULL_MODE       cullMode = CullModes::NONE;
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
                SMultisampling() {}
                SMultisampling( DEFAULT_CTOR_INIT ) {}

                SAMPLE_COUNT    sampleCount = SampleCounts::SAMPLE_1;
                bool            enable = false;
            };

            struct SDepthStencil
            {
                SDepthStencil() {}
                SDepthStencil( DEFAULT_CTOR_INIT ) {}

                bool                    enable = true;
                bool                    enableDepthTest = false;
                bool                    enableDepthWrite = false;
                bool                    enableStencilTest = false;
                bool                    enableStencilWrite = false;
                COMPARE_FUNCTION        depthFunction = CompareFunctions::GREATER_EQUAL;
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
                    //vVertexAttributes.PushBack( DEFAULT_CONSTRUCTOR_INIT );
                }

                SVertexAttributeArray   vVertexAttributes;
                PRIMITIVE_TOPOLOGY      topology = PrimitiveTopologies::TRIANGLE_LIST;
                bool                    enablePrimitiveRestart = false;
            };

            struct STesselation
            {
                STesselation() {}
                STesselation( DEFAULT_CTOR_INIT ) {}
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
            DDIPipelineLayout       hDDILayout = DDI_NULL_HANDLE;
            RenderPassHandle        hRenderPass = NULL_HANDLE;
            DDIRenderPass           hDDIRenderPass = DDI_NULL_HANDLE;
        };

        struct SPipelineCreateDesc
        {
            SCreateResourceDesc     Create;
            SPipelineDesc           Pipeline;
        };

        struct IndexTypes
        {
            enum TYPE : uint8_t
            {
                UINT16,
                UINT32,
                _MAX_COUNT
            };
        };
        using INDEX_TYPE = IndexTypes::TYPE;

        struct SCreateShaderDesc
        {
            SCreateResourceDesc Create;
            SShaderDesc         Shader;

            SCreateShaderDesc() {}
            SCreateShaderDesc(const SCreateShaderDesc& Other) :
                Create{ Other.Create }
                , Shader{ Other.Shader }
            {
            }

            SCreateShaderDesc(SCreateShaderDesc&& Other) = default;

            SCreateShaderDesc& operator=(const SCreateShaderDesc& Other)
            {
                Create = Other.Create;
                Shader = Other.Shader;
                return *this;
            }

            SCreateShaderDesc& operator=(SCreateShaderDesc&& Other) = default;
        };

        struct BufferTypes
        {
            enum TYPE
            {
                VERTEX,
                INDEX,
                UNIFORM,
                INDIRECT,
                TRANSFER,
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
            MEMORY_USAGE    memoryUsage;
            BUFFER_USAGE    usage;
            INDEX_TYPE      indexType;
            uint32_t        size;
            uint8_t         chunkCount = 1;
        };

        struct VertexAttributeTypes
        {
            enum TYPE
            {
                UNDEFINED = Formats::UNDEFINED,
                FLOAT = Formats::R32_SFLOAT,
                VECTOR2 = Formats::R32G32_SFLOAT,
                VECTOR3 = Formats::R32G32B32_SFLOAT,
                VECTOR4 = Formats::R32G32B32A32_SFLOAT,
                INT = Formats::R32_SINT,
                UINT = Formats::R32_UINT,
                POSITION3 = VECTOR3,
                POSITION4 = VECTOR4,
                TEXCOORD = VECTOR2,
                NORMAL = VECTOR3
            };
        };
        using VERTEX_ATTRIBUTE_TYPE = VertexAttributeTypes::TYPE;

        struct SVertexAttributeDesc
        {
            cstr_t                  pName = "";
            VERTEX_ATTRIBUTE_TYPE   type;
            uint8_t                 vertexBufferBinding = 0;
        };

        struct SVertexInputLayoutDesc
        {
            using AttributeArray = Utils::TCDynamicArray< SVertexAttributeDesc >;
            AttributeArray      vAttributes;
            PRIMITIVE_TOPOLOGY  topology = PrimitiveTopologies::TRIANGLE_LIST;
            bool                enablePrimitiveRestart = false;
        };

        struct SVertexBufferDesc : SBufferDesc
        {
            SVertexInputLayoutDesc   Layout;
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

        struct SCreateVertexBufferDesc
        {
            SCreateResourceDesc Create;
            SVertexBufferDesc   Buffer;
        };

        struct SSemaphoreDesc
        {

        };

        struct SFenceDesc
        {
            bool    isSignaled = false;
        };

        struct SEventDesc
        {

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
            TextureSize MinSize;
            TextureSize MaxSize;
            TextureSize CurrentSize;
            uint32_t    minImageCount;
            uint32_t    maxImageCount;
            bool        canBeUsedAsRenderTarget;
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
            using FramebufferArray = Utils::TCDynamicArray< DDIFramebuffer, 3 >;

            void*                   pInternalAllocator = nullptr;
            ImageArray              vImages;
            ImageViewArray          vImageViews;
            FramebufferArray        vFramebuffers;
            DDIRenderPass           hDDIRenderPass = DDI_NULL_HANDLE;
            DDIPresentSurface       hSurface = DDI_NULL_HANDLE;
            DDISwapChain            hSwapChain = DDI_NULL_HANDLE;
            TextureSize             Size;
            SPresentSurfaceFormat   Format;
            PRESENT_MODE            mode;
            SPresentSurfaceCaps     Caps;
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
            //CommandBufferPtr*   pCommandBuffers = nullptr;
            DDICommandBuffer*   pDDICommandBuffers = nullptr;
            DDIFence            hDDIFence = DDI_NULL_HANDLE;
            DDIQueue            hDDIQueue = DDI_NULL_HANDLE;
            uint8_t             signalSemaphoreCount = 0;
            uint8_t             waitSemaphoreCount = 0;
            uint8_t             commandBufferCount = 0;
        };

        struct SPresentInfo
        {
            uint32_t        imageIndex;
            //DDISwapChain    hDDISwapChain;
            CSwapChain*     pSwapChain;
            DDISemaphore    hDDIWaitSemaphore = DDI_NULL_HANDLE;
        };

        struct SPresentData
        {
            using UintArray = Utils::TCDynamicArray< uint32_t, 8 >;
            using SemaphoreArray = Utils::TCDynamicArray< DDISemaphore, 8 >;
            using SwapChainArray = Utils::TCDynamicArray< DDISwapChain, 8 >;
            SwapChainArray      vSwapchains;
            SemaphoreArray      vWaitSemaphores;
            UintArray           vImageIndices;
            DDIQueue            hQueue = DDI_NULL_HANDLE;
        };

        struct SAllocateMemoryData
        {
            DDIMemory   hDDIMemory;
            uint32_t    sizeLeft;
        };

        struct SBindMemoryInfo
        {
            DDITexture  hDDITexture = DDI_NULL_HANDLE;
            DDIBuffer   hDDIBuffer  = DDI_NULL_HANDLE;
            DDIMemory   hDDIMemory  = DDI_NULL_HANDLE;
            handle_t    hMemory     = NULL_HANDLE;
            uint32_t    offset      = 0;
        };

        struct SUpdateMemoryInfo
        {
            const void*     pData;
            uint32_t        dataSize;
            uint32_t        dstDataOffset;
        };

        struct SBindPipelineInfo
        {
            CCommandBuffer*     pCmdBuffer;
            CPipeline*          pPipeline;
        };

        struct SBeginRenderPassInfo
        {
            using ClearValueArray = Utils::TCDynamicArray< DDIClearValue, 8 >;

            ClearValueArray vDDIClearValues;
            DDIFramebuffer  hDDIFramebuffer;
            DDIRenderPass   hDDIRenderPass;
            DrawRect        RenderArea;
        };

        struct SBindRenderPassInfo
        {
            DDICommandBuffer            hDDICommandBuffer;
            const SBeginRenderPassInfo* pBeginInfo;
        };

        struct SBindVertexBufferInfo
        {
            CCommandBuffer*     pCmdBuffer;
            CVertexBuffer*      pBuffer;
            size_t              offset;
        };

        struct SBindVertexBufferInfo2
        {
            DDICommandBuffer    hDDICommandBuffer;
            DDIBuffer           hDDIBuffer;
            uint32_t            offset;
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
            TextureSize         Size = { 800, 600 };
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

            uint32_t    maxSetCount = Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT;
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

        struct SCompileShaderInfo
        {
            const char*             pName = "Unknown";
            const char*             pEntryPoint = "main";
            const char*             pBuffer = nullptr;
            //glslang::TShader*   pShader = nullptr;
            //glslang::TProgram*	pProgram = nullptr;
            void*                   pCompilerData = nullptr;
            uint32_t                bufferSize = 0;
            SHADER_TYPE             type;
            uint8_t                 tid = 0;
        };

        struct SCompileShaderData
        {
            using BinaryElement = uint32_t;
            using ShaderBinaryData = vke_vector < BinaryElement >;
            ShaderBinaryData    vShaderBinary;
            uint32_t            codeByteSize;
        };

        struct CommandBufferEndFlags
        {
            enum FLAGS
            {
                END                         = VKE_BIT( 0 ),
                EXECUTE                     = VKE_BIT( 1 ),
                WAIT                        = VKE_BIT( 2 ),
                DONT_SIGNAL_SEMAPHORE       = VKE_BIT( 3 ),
                DONT_WAIT_FOR_SEMAPHORE     = VKE_BIT( 4 ),
                PUSH_SIGNAL_SEMAPHORE       = VKE_BIT( 5 ),
                _MAX_COUNT                  = 6
            };
        };
        using COMMAND_BUFFER_END_FLAGS = uint32_t;
        
        struct STransferContextDesc
        {
            SCommandBufferPoolDesc  CmdBufferPoolDesc;
            void*                   pPrivate = nullptr;
        };

        struct SGraphicsContextDesc
        {
            SSwapChainDesc              SwapChainDesc;
            SCommandBufferPoolDesc      CmdBufferPoolDesc;
            SDescriptorPoolDesc         DescriptorPoolDesc;
            void*                       pPrivate = nullptr;
        };

        struct SDeviceContextDesc
        {
            const SAdapterInfo* pAdapterInfo = nullptr;
            const void*         pPrivate = nullptr;
            DescriptorSetCounts aMaxDescriptorSetCounts = { 0 };
        };

        struct SDrawParams
        {
            union
            {
                uint32_t    indexCount;
                uint32_t    vertexCount;
            };
            uint32_t    instanceCount;
            union
            {
                uint32_t    startIndex;
                uint32_t    startVertex;
            };
            uint32_t    vertexOffset;
            uint32_t    startInstance;
        };

#define VKE_ADD_DDI_OBJECT(_type) \
        protected: _type  m_hDDIObject = DDI_NULL_HANDLE; \
        public: vke_force_inline const _type& GetDDIObject() const { return m_hDDIObject; }

    } // RenderSystem

    using SRenderSystemDesc = RenderSystem::SRenderSystemDesc;

} // VKE
