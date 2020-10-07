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
#define VKE_RENDER_SYSTEM_DEBUG VKE_RENDERER_DEBUG

#if VKE_RENDERER_DEBUG || VKE_DEBUG
#   define VKE_RENDER_SYSTEM_DEBUG_CODE(_code) _code
#   define VKE_RENDER_SYSTEM_DEBUG_NAME cstr_t pDebugName = ""
#   define VKE_RENDER_SYSTEM_DEBUG_INFO SDebugInfo* pDebugInfo = nullptr
#   define VKE_RENDER_SYSTEM_BEGIN_DEBUG_INFO(_pCmdBuff, _obj) \
    ( _pCmdBuff )->BeginDebugInfo( ( _obj ).pDebugInfo )
#   define VKE_RENDER_SYSTEM_END_DEBUG_INFO(_pCmdBuff) (_pCmdBuff)->EndDebugInfo()
#   define VKE_RENDER_SYSTEM_SET_DEBUG_INFO(_obj, _text, _Color) \
    VKE::RenderSystem::SDebugInfo __dbgInfo; \
    __dbgInfo.pText = ( _text ); \
    __dbgInfo.Color = ( _Color ); \
    (_obj).pDebugInfo = &__dbgInfo;
#else
#   define VKE_RENDER_SYSTEM_DEBUG_CODE(_code)
#   define VKE_RENDER_SYSTEM_DEBUG_NAME
#   define VKE_RENDER_SYSTEM_DEBUG_INFO
#   define VKE_RENDER_SYSTEM_BEGIN_DEBUG_INFO(_pCmdBuff, _obj)
#   define VKE_RENDER_SYSTEM_END_DEBUG_INFO(_pCmdBuff)
#   define VKE_RENDER_SYSTEM_SET_DEBUG_INFO(_obj, _text, _Color)
#endif // VKE_RENDER_SYSTEM_DEBUG

#define VKE_RENDER_SYSTEM_SET_DEBUG_NAME(_obj, _name) VKE_DEBUG_CODE(_obj.pDebugName = _name)
#if VKE_RENDERER_DEBUG
#   define VKE_RENDER_SYSTEM_GET_DEBUG_NAME(_obj)   (_obj).pDebugName
//#   define VKE_RENDER_SYSTEM_SET_DEBUG_INFO(_obj, _color, _text ) do{ (_obj).pDebugInfo->Color = (_color); (_obj).pDebugInfo->pText = (_text); }while(0,0)
#else
#   define VKE_RENDER_SYSTEM_GET_DEBUG_NAME(_obj)   ""
//#   define VKE_RENDER_SYSTEM_SET_DEBUG_INFO( _dbgInfo, _color, _text )
#endif // VKE_RENDERER_DEBUG

#if VKE_USE_GLSL_COMPILER
#   define  VKE_SHADER_COMPILER_STR(_str) _str
#elif VKE_USE_DIRECTX_SHADER_COMPILER
#   define  VKE_SHADER_COMPILER_STR(_str) L##_str
#endif

#if !defined (VKE_USE_HLSL_SYNTAX)
#   define VKE_USE_HLSL_SYNTAX 0
#endif

#if VKE_USE_GLSL_COMPILER
    using ShaderCompilerStrType = cstr_t;
    using ShaderCompilerCharType = char;
#elif VKE_USE_DIRECTX_SHADER_COMPILER
    using ShaderCompilerStrType = cwstr_t;
    using ShaderCompilerCharType = wchar_t;
#endif

    using ShaderCompilerString = Utils::TCString< ShaderCompilerCharType >;

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

        VKE_DECLARE_HANDLE( Pipeline );
        VKE_DECLARE_HANDLE( PipelineLayout );
        VKE_DECLARE_HANDLE( DescriptorSet );
        VKE_DECLARE_HANDLE( DescriptorSetLayout );
        VKE_DECLARE_HANDLE2( Buffer, uint32_t );
        VKE_DECLARE_HANDLE2( VertexBuffer, uint32_t );
        VKE_DECLARE_HANDLE2( IndexBuffer, uint32_t );
        VKE_DECLARE_HANDLE( Texture );
        VKE_DECLARE_HANDLE( TextureView );
        VKE_DECLARE_HANDLE( BufferView );
        VKE_DECLARE_HANDLE( RenderPass );
        VKE_DECLARE_HANDLE( Sampler );
        VKE_DECLARE_HANDLE( Framebuffer );
        VKE_DECLARE_HANDLE( Shader );
        VKE_DECLARE_HANDLE( RenderTarget );
        VKE_DECLARE_HANDLE( Event );



        static vke_force_inline uint16_t CalcFormatSize( const FORMAT& fmt )
        {
            // Size in bits
            static const uint16_t aSizes[] =
            {
                0, //UNDEFINED,
                2 * 4, // R4G4_UNORM_PACK8,
                3 * 4, // R4G4B4A4_UNORM_PACK16,
                3 * 4, // B4G4R4A4_UNORM_PACK16,
                5 + 6 + 5, // R5G6B5_UNORM_PACK16,
                5 + 6 + 5, // B5G6R5_UNORM_PACK16,
                5 + 5 + 5 + 1, // R5G5B5A1_UNORM_PACK16,
                5 + 5 + 5 + 1, // B5G5R5A1_UNORM_PACK16,
                1 + 5 + 5 + 5, // A1R5G5B5_UNORM_PACK16,
                8, // R8_UNORM,
                8, // R8_SNORM,
                8, // R8_USCALED,
                8, // R8_SSCALED,
                8, // R8_UINT,
                8, // R8_SINT,
                8, // R8_SRGB,
                8 + 8, // R8G8_UNORM,
                8 + 8, // R8G8_SNORM,
                8 + 8, // R8G8_USCALED,
                8 + 8, // R8G8_SSCALED,
                8 + 8, // R8G8_UINT,
                8 + 8, // R8G8_SINT,
                8 + 8, // R8G8_SRGB,
                3 * 8, // R8G8B8_UNORM,
                3 * 8, // R8G8B8_SNORM,
                3 * 8, // R8G8B8_USCALED,
                3 * 8, // R8G8B8_SSCALED,
                3 * 8, // R8G8B8_UINT,
                3 * 8, // R8G8B8_SINT,
                3 * 8, // R8G8B8_SRGB,
                3 * 8, // B8G8R8_UNORM,
                3 * 8, // B8G8R8_SNORM,
                3 * 8, // B8G8R8_USCALED,
                3 * 8, // B8G8R8_SSCALED,
                3 * 8, // B8G8R8_UINT,
                3 * 8, // B8G8R8_SINT,
                3 * 8, // B8G8R8_SRGB,
                4 * 8, // R8G8B8A8_UNORM,
                4 * 8, // R8G8B8A8_SNORM,
                4 * 8, // R8G8B8A8_USCALED,
                4 * 8, // R8G8B8A8_SSCALED,
                4 * 8, // R8G8B8A8_UINT,
                4 * 8, // R8G8B8A8_SINT,
                4 * 8, // R8G8B8A8_SRGB,
                4 * 8, // B8G8R8A8_UNORM,
                4 * 8, // B8G8R8A8_SNORM,
                4 * 8, // B8G8R8A8_USCALED,
                4 * 8, // B8G8R8A8_SSCALED,
                4 * 8, // B8G8R8A8_UINT,
                4 * 8, // B8G8R8A8_SINT,
                4 * 8, // B8G8R8A8_SRGB,
                4 * 8, // A8B8G8R8_UNORM_PACK32,
                4 * 8, // A8B8G8R8_SNORM_PACK32,
                4 * 8, // A8B8G8R8_USCALED_PACK32,
                4 * 8, // A8B8G8R8_SSCALED_PACK32,
                4 * 8, // A8B8G8R8_UINT_PACK32,
                4 * 8, // A8B8G8R8_SINT_PACK32,
                4 * 8, // A8B8G8R8_SRGB_PACK32,
                2 + 3 * 10, // A2R10G10B10_UNORM_PACK32,
                2 + 3 * 10, // A2R10G10B10_SNORM_PACK32,
                2 + 3 * 10, // A2R10G10B10_USCALED_PACK32,
                2 + 3 * 10, // A2R10G10B10_SSCALED_PACK32,
                2 + 3 * 10, // A2R10G10B10_UINT_PACK32,
                2 + 3 * 10, // A2R10G10B10_SINT_PACK32,
                2 + 3 * 10, // A2B10G10R10_UNORM_PACK32,
                2 + 3 * 10, // A2B10G10R10_SNORM_PACK32,
                2 + 3 * 10, // A2B10G10R10_USCALED_PACK32,
                2 + 3 * 10, // A2B10G10R10_SSCALED_PACK32,
                2 + 3 * 10, // A2B10G10R10_UINT_PACK32,
                2 + 3 * 10, // A2B10G10R10_SINT_PACK32,
                16, // R16_UNORM,
                16, // R16_SNORM,
                16, // R16_USCALED,
                16, // R16_SSCALED,
                16, // R16_UINT,
                16, // R16_SINT,
                16, // R16_SFLOAT,
                2 * 16, // R16G16_UNORM,
                2 * 16, // R16G16_SNORM,
                2 * 16, // R16G16_USCALED,
                2 * 16, // R16G16_SSCALED,
                2 * 16, // R16G16_UINT,
                2 * 16, // R16G16_SINT,
                2 * 16, // R16G16_SFLOAT,
                3 * 16, // R16G16B16_UNORM,
                3 * 16, // R16G16B16_SNORM,
                3 * 16, // R16G16B16_USCALED,
                3 * 16, // R16G16B16_SSCALED,
                3 * 16, // R16G16B16_UINT,
                3 * 16, // R16G16B16_SINT,
                3 * 16, // R16G16B16_SFLOAT,
                4 * 16, // R16G16B16A16_UNORM,
                4 * 16, // R16G16B16A16_SNORM,
                4 * 16, // R16G16B16A16_USCALED,
                4 * 16, // R16G16B16A16_SSCALED,
                4 * 16, // R16G16B16A16_UINT,
                4 * 16, // R16G16B16A16_SINT,
                4 * 16, // R16G16B16A16_SFLOAT,
                32, // R32_UINT,
                32, // R32_SINT,
                32, // R32_SFLOAT,
                2 * 32, // R32G32_UINT,
                2 * 32, // R32G32_SINT,
                2 * 32, // R32G32_SFLOAT,
                3 * 32, // R32G32B32_UINT,
                3 * 32, // R32G32B32_SINT,
                3 * 32, // R32G32B32_SFLOAT,
                4 * 32, // R32G32B32A32_UINT,
                4 * 32, // R32G32B32A32_SINT,
                4 * 32, // R32G32B32A32_SFLOAT,
                64, // R64_UINT,
                64, // R64_SINT,
                64, // R64_SFLOAT,
                2 * 64, // R64G64_UINT,
                2 * 64, // R64G64_SINT,
                2 * 64, // R64G64_SFLOAT,
                3 * 64, // R64G64B64_UINT,
                3 * 64, // R64G64B64_SINT,
                3 * 64, // R64G64B64_SFLOAT,
                4 * 64, // R64G64B64A64_UINT,
                4 * 64, // R64G64B64A64_SINT,
                4 * 64, // R64G64B64A64_SFLOAT,
                10 + 2 * 11, // B10G11R11_UFLOAT_PACK32,
                5 + 3 * 9, // E5B9G9R9_UFLOAT_PACK32,
                1 * 16, // D16_UNORM,
                32, // X8_D24_UNORM_PACK32,
                32, // D32_SFLOAT,
                8, // S8_UINT,
                16, // D16_UNORM_S8_UINT,
                32, // D24_UNORM_S8_UINT,
                32, // D32_SFLOAT_S8_UINT,
                64, // BC1_RGB_UNORM_BLOCK,
                64, // BC1_RGB_SRGB_BLOCK,
                64, // BC1_RGBA_UNORM_BLOCK,
                64, // BC1_RGBA_SRGB_BLOCK,
                128, // BC2_UNORM_BLOCK,
                128, // BC2_SRGB_BLOCK,
                128, // BC3_UNORM_BLOCK,
                128, // BC3_SRGB_BLOCK,
                64, // BC4_UNORM_BLOCK,
                64, // BC4_SNORM_BLOCK,
                128, // BC5_UNORM_BLOCK,
                128, // BC5_SNORM_BLOCK,
                128, // BC6H_UFLOAT_BLOCK,
                128, // BC6H_SFLOAT_BLOCK,
                128, // BC7_UNORM_BLOCK,
                128, // BC7_SRGB_BLOCK,
                128, // ETC2_R8G8B8_UNORM_BLOCK,
                128, // ETC2_R8G8B8_SRGB_BLOCK,
                64, // ETC2_R8G8B8A1_UNORM_BLOCK,
                64, // ETC2_R8G8B8A1_SRGB_BLOCK,
                128, // ETC2_R8G8B8A8_UNORM_BLOCK,
                128, // ETC2_R8G8B8A8_SRGB_BLOCK,
                128, // EAC_R11_UNORM_BLOCK,
                128, // EAC_R11_SNORM_BLOCK,
                128, // EAC_R11G11_UNORM_BLOCK,
                128, // EAC_R11G11_SNORM_BLOCK,
                128, // ASTC_4x4_UNORM_BLOCK,
                128, // ASTC_4x4_SRGB_BLOCK,
                128, // ASTC_5x4_UNORM_BLOCK,
                128, // ASTC_5x4_SRGB_BLOCK,
                128, // ASTC_5x5_UNORM_BLOCK,
                128, // ASTC_5x5_SRGB_BLOCK,
                128, // ASTC_6x5_UNORM_BLOCK,
                128, // ASTC_6x5_SRGB_BLOCK,
                128, // ASTC_6x6_UNORM_BLOCK,
                128, // ASTC_6x6_SRGB_BLOCK,
                128, // ASTC_8x5_UNORM_BLOCK,
                128, // ASTC_8x5_SRGB_BLOCK,
                128, // ASTC_8x6_UNORM_BLOCK,
                128, // ASTC_8x6_SRGB_BLOCK,
                128, // ASTC_8x8_UNORM_BLOCK,
                128, // ASTC_8x8_SRGB_BLOCK,
                128, // ASTC_10x5_UNORM_BLOCK,
                128, // ASTC_10x5_SRGB_BLOCK,
                128, // ASTC_10x6_UNORM_BLOCK,
                128, // ASTC_10x6_SRGB_BLOCK,
                128, // ASTC_10x8_UNORM_BLOCK,
                128, // ASTC_10x8_SRGB_BLOCK,
                128, // ASTC_10x10_UNORM_BLOCK,
                128, // ASTC_10x10_SRGB_BLOCK,
                128, // ASTC_12x10_UNORM_BLOCK,
                128, // ASTC_12x10_SRGB_BLOCK,
                128, // ASTC_12x12_UNORM_BLOCK,
                128, // ASTC_12x12_SRGB_BLOCK,

            };
            return aSizes[fmt] / 8; // convert to bytes
        }

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
                r(static_cast<float>(v)), g( static_cast<float>( v )), b( static_cast<float>( v ) ), a( static_cast<float>( v ) )
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
                Utils::SHash Hash;
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
                Utils::SHash Hash;
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

        struct SDebugInfo
        {
            cstr_t  pText;
            SColor  Color;
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

        struct ShaderProfiles
        {
            enum PROFILE
            {
                PROFILE_6_0,
                PROFILE_6_1,
                PROFILE_6_2,
                PROFILE_6_3,
                PROFILE_6_4,
                _MAX_COUNT,
                DEFAULT = PROFILE_6_0
            };
        };
        using SHADER_PROFILE = ShaderProfiles::PROFILE;

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
                SAMPLER, // only sampler
                TEXTURE, // only texture without sampler
                SAMPLER_AND_TEXTURE, // texture with sampler
                STORAGE_TEXTURE,
                UNIFORM_TEXEL_BUFFER,
                STORAGE_TEXEL_BUFFER,
                CONSTANT_BUFFER,
                STORAGE_BUFFER,
                DYNAMIC_CONSTANT_BUFFER,
                DYNAMIC_STORAGE_BUFFER,
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

        struct SUpdateBindingsHelper
        {
            template<class HandleType>
            struct TSBinding
            {
                const HandleType*   ahHandles;
                uint16_t            count;
                BINDING_TYPE        type;
                uint8_t             binding;
            };

            struct SBufferBinding : TSBinding<BufferHandle>
            {
                uint32_t    offset;
                uint32_t    range;
            };

            struct SSamplerAndTextureBinding
            {
                const SamplerHandle*    ahSamplers;
                const TextureHandle*    ahTextures;
                const TextureViewHandle* ahTexViews;
                uint16_t                count;
                BINDING_TYPE            type;
                uint8_t                 binding;
            };

            void AddBinding( uint8_t binding, const RenderTargetHandle* ahHandles, const uint16_t count )
            {
                TSBinding<RenderTargetHandle> Binding;
                Binding.ahHandles = ahHandles;
                Binding.count = count;
                Binding.binding = binding;
                vRTs.PushBack( Binding );
            }

            /*void AddBinding( uint8_t binding, const TextureHandle* ahHandles, const uint16_t count )
            {
                TSBinding<TextureHandle> Binding;
                Binding.ahHandles = ahHandles;
                Binding.count = count;
                Binding.binding = binding;
                vTexs.PushBack( Binding );
            }*/

            void AddBinding(uint8_t binding, const TextureViewHandle* ahHandles, const uint16_t count)
            {
                TSBinding<TextureViewHandle> Binding;
                Binding.ahHandles = ahHandles;
                Binding.count = count;
                Binding.binding = binding;
                vTexViews.PushBack(Binding);
            }

            void AddBinding( uint8_t binding, const SamplerHandle* ahHandles, const uint16_t count )
            {
                TSBinding<SamplerHandle> Binding;
                Binding.ahHandles = ahHandles;
                Binding.count = count;
                Binding.binding = binding;
                vSamplers.PushBack( Binding );
            }

            void AddBinding( uint8_t binding, const SamplerHandle* ahSamplers, const TextureViewHandle* ahTexViews,
                const uint16_t count )
            {
                SSamplerAndTextureBinding Binding;
                Binding.ahSamplers = ahSamplers;
                Binding.ahTexViews = ahTexViews;
                Binding.count = count;
                Binding.binding = binding;
                Binding.type = BindingTypes::SAMPLER_AND_TEXTURE;
                vSamplerAndTextures.PushBack(Binding);
            }

            void AddBinding( uint8_t binding, const uint32_t& offset, const uint32_t& range,
                             const BufferHandle& hBuffer, BINDING_TYPE type = BindingTypes::DYNAMIC_CONSTANT_BUFFER )
            {
                SBufferBinding Binding;
                Binding.ahHandles = &hBuffer;
                Binding.count = 1;
                Binding.binding = binding;
                Binding.offset = offset;
                Binding.range = range;
                Binding.type = type;
                vBuffers.PushBack( Binding );
            }

            void Reset()
            {
                vRTs.Clear();
                vTexs.Clear();
                vTexViews.Clear();
                vSamplers.Clear();
                vBuffers.Clear();
                vSamplerAndTextures.Clear();
            }

            template<class HandleType>
            using BindingArray = Utils::TCDynamicArray< TSBinding<HandleType>, 16 >;
            using RtArray = BindingArray< RenderTargetHandle >;
            using TexArray = BindingArray< TextureHandle >;
            using TexViewArray = BindingArray< TextureViewHandle >;
            using SamplerArray = BindingArray< SamplerHandle >;
            using BufferArray = Utils::TCDynamicArray< SBufferBinding, 8 >;
            using SamplerAndTextureArray = Utils::TCDynamicArray< SSamplerAndTextureBinding, 16 >;

            RtArray         vRTs;
            TexArray        vTexs;
            TexViewArray    vTexViews;
            SamplerArray    vSamplers;
            BufferArray     vBuffers;
            SamplerAndTextureArray  vSamplerAndTextures;
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
                TEXTURE_CUBE,
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
                FILE_IO                     = VKE_BIT( 7 ), // Texture created from file
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
            SAMPLER_FILTER  min = SamplerFilters::NEAREST;
            SAMPLER_FILTER  mag = SamplerFilters::NEAREST;
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
            COMPARE_FUNCTION    compareFunc = CompareFunctions::NEVER;
            SSamplerFilters     Filter;
            MIPMAP_MODE         mipmapMode = MipmapModes::NEAREST;
            ExtentF32           LOD = { 0.0f, 0.0f };
            float               maxAnisotropy = 1.0f;
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
            uint16_t            mipmapCount = 1;
            MEMORY_USAGE        memoryUsage = MemoryUsages::DEFAULT;
            uint16_t            arrayElementCount = 1; // number of textures in array
            uint16_t            sliceCount = 1; // number of slices in 3d
            char                aName[Config::Resource::MAX_NAME_LENGTH];
            VKE_RENDER_SYSTEM_DEBUG_NAME;

            STextureDesc()
            {
                aName[0] = 0;
            }

            void SetName(cstr_t pName)
            {
                VKE_ASSERT(strlen(pName) < Config::Resource::MAX_NAME_LENGTH, "Texture name length is too long.");
#if VKE_RENDERER_DEBUG
                if( strlen( pName ) > Config::Resource::MAX_NAME_LENGTH )
                {
                    VKE_LOG_ERR( "Texture name: " << pName << " is too long. Max length is set to: " << Config::Resource::MAX_NAME_LENGTH );
                }
#endif
                vke_strcpy( aName, sizeof( aName ), pName );
            }

            STextureDesc& operator=(const STextureDesc& Other)
            {
                Size = Other.Size;
                format = Other.format;
                usage = Other.usage;
                type = Other.type;
                multisampling = Other.multisampling;
                mipmapCount = Other.mipmapCount;
                memoryUsage = Other.memoryUsage;
                SetName(Other.aName);
                return *this;
            }
        };

        struct SCreateTextureDesc
        {
            Core::SCreateResourceInfo   Create;
            STextureDesc                Texture;
        };

        struct STextureViewDesc
        {
            TextureHandle               hTexture = INVALID_HANDLE;
            TEXTURE_VIEW_TYPE           type = TextureViewTypes::VIEW_2D;
            TEXTURE_FORMAT              format = Formats::R8G8B8A8_UNORM;
            STextureSubresourceRange    SubresourceRange;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct SCreateTextureViewDesc
        {
            Core::SCreateResourceInfo   Create;
            STextureViewDesc            TextureView;
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
                    TextureViewHandle hTextureView = INVALID_HANDLE;
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
                TextureViewHandle               hTextureView = INVALID_HANDLE;
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
                RenderPassHandle    hPass = INVALID_HANDLE;
                RenderPassHandle    hWaitForPass = INVALID_HANDLE;
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
                COLOR_RENDER_TARGET_READ = VKE_BIT( 26 ),
                COLOR_RENDER_TARGET_WRITE = VKE_BIT( 27 ),
                DEPTH_STENCIL_RENDER_TARGET_READ = VKE_BIT( 28 ),
                DEPTH_STENCIL_RENDER_TARGET_WRITE = VKE_BIT( 29 ),
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

        struct ShaderCompilationStages
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
        using SHADER_COMPILATION_STAGE = ShaderCompilationStages::STATE;

        struct SShaderData
        {
            uint32_t                    codeSize;
            SHADER_TYPE                 type = ShaderTypes::_MAX_COUNT;
            SHADER_COMPILATION_STAGE    stage;
            const uint8_t*              pCode;
        };

        struct SShaderDefine
        {
            //using String = Utils::TCString< char, Config::RenderSystem::Shader::MAX_ENTRY_POINT_NAME_LENGTH >;
            using String = ShaderCompilerStrType;
            String  Name;
            String  Value;
        };

        struct SShaderDesc
        {
            template<typename T>
            using NameString = Utils::TCString< T, Config::RenderSystem::Shader::MAX_ENTRY_POINT_NAME_LENGTH >;

            using NameWString = NameString< wchar_t >;
            using NameCString = NameString< char >;
            //using NameString = Utils::TCString< char, Config::RenderSystem::Shader::MAX_ENTRY_POINT_NAME_LENGTH >;
            using IncludeString = Utils::TCString< char, Config::RenderSystem::Shader::MAX_INCLUDE_PATH_LENGTH >;
            using PreprocessorString = Utils::TCString< char, Config::RenderSystem::Shader::MAX_PREPROCESSOR_DIRECTIVE_LENGTH >;

            using IncStringArray = Utils::TCDynamicArray< IncludeString >;
            using PrepStringArray = Utils::TCDynamicArray< PreprocessorString >;

            using DefineArray = Utils::TCDynamicArray< SShaderDefine >;

            Core::SFileInfo         FileInfo;
            SHADER_TYPE             type = ShaderTypes::_MAX_COUNT;
            SHADER_PROFILE          profile = ShaderProfiles::DEFAULT;
            NameCString             EntryPoint = "main";
            NameCString             Name = "Unknown";
            IncStringArray          vIncludes;
            PrepStringArray         vPreprocessor;
            DefineArray             vDefines;
            SShaderData*            pData = nullptr; // optional parameter if an application wants to use its own binaries
            bool                    useShaderCache = true;

            //SShaderDesc() = default;

            //SShaderDesc(const SShaderDesc& Other)
            //{
            //    this->operator=( Other );
            //}

            //SShaderDesc(SShaderDesc&& Other)
            //{
            //    this->operator=( std::move( Other ) );
            //}

            //SShaderDesc& operator=(const SShaderDesc& Other)
            //{
            //    FileInfo = Other.FileInfo;
            //    type = Other.type;
            //    //SetEntryPoint( Other.pEntryPoint );
            //    EntryPoint = Other.EntryPoint;
            //    vIncludes = Other.vIncludes;
            //    vPreprocessor = Other.vPreprocessor;
            //    pData = Other.pData;
            //    return *this;
            //}

            //SShaderDesc& operator=(SShaderDesc&& Other)
            //{
            //    FileInfo = Other.FileInfo;
            //    type = Other.type;
            //    //SetEntryPoint( Other.pEntryPoint );
            //    EntryPoint = Other.EntryPoint;
            //    vIncludes = std::move(Other.vIncludes);
            //    vPreprocessor = std::move(Other.vPreprocessor);
            //    pData = std::move( Other.pData );
            //    return *this;
            //}

           /* void vke_force_inline SetEntryPoint( cstr_t pName )
            {
                if (std::is_same< cstr_t, cstr_t >::value)
                {
                    VKE_ASSERT(strlen( (const char*)pName) < sizeof(pEntryPoint), "");
                    vke_strcpy( (char*)pEntryPoint, sizeof(pEntryPoint), (cstr_t)pName);
                }
                else if (std::is_same< cwstr_t, ShaderCompilerStrType >::value)
                {
                    VKE_ASSERT(wcslen(pName) < sizeof(pEntryPoint), "");
                    vke_wstrcpy( (wchar_t*)pEntryPoint, sizeof(pEntryPoint), (cwstr_t)pName);
                }
            }*/
        };

        struct SCompileShaderInfo
        {
            cstr_t              pBuffer = nullptr;
            void*               pCompilerData = nullptr;
            const SShaderDesc*  pDesc = nullptr;
            uint32_t            bufferSize = 0;
            uint8_t             tid = 0;
        };

        struct SCompileShaderData
        {
#if VKE_USE_GLSL_COMPILER
            using BinaryElement = uint32_t;
#elif VKE_USE_DIRECTX_SHADER_COMPILER
            using BinaryElement = uint8_t;
#endif
            using ShaderBinaryData = vke_vector < BinaryElement >;
            ShaderBinaryData    vShaderBinary;
            uint32_t            codeByteSize;
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

        struct StencilFunctions
        {
            enum FUNCTION : uint8_t
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
        using STENCIL_FUNCTION = StencilFunctions::FUNCTION;



        struct SStencilOperationDesc
        {
            STENCIL_FUNCTION    failFunc = StencilFunctions::KEEP;
            STENCIL_FUNCTION    passFunc = StencilFunctions::KEEP;
            STENCIL_FUNCTION    depthFailFunc = StencilFunctions::KEEP;
            COMPARE_FUNCTION    compareFunc = CompareFunctions::ALWAYS;
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
                    idx = pPool->vPool.PushBack(Task);
                    pTask = &pPool->vPool[idx];
                }
                return pTask;
            }
        };

        struct SPipelineLayoutDesc;

        struct SCreateShaderDesc
        {
            Core::SCreateResourceInfo   Create;
            SShaderDesc                 Shader;
        };

        struct SPipelineDesc
        {
            struct SShaders
            {
                ~SShaders() {}
                ShaderPtr           apShaders[ ShaderTypes::_MAX_COUNT ];
            };

            struct SBlending
            {
                using BlendStateArray = Utils::TCDynamicArray< SBlendState, Config::RenderSystem::Pipeline::MAX_BLEND_STATE_COUNT >;
                BlendStateArray vBlendStates;
                LOGIC_OPERATION logicOperation = LogicOperations::NO_OPERATION;
                bool            enable = false;
            };

            struct SViewport
            {
                using ViewportArray = Utils::TCDynamicArray< SViewportDesc, Config::RenderSystem::Pipeline::MAX_VIEWPORT_COUNT >;
                using ScissorArray = Utils::TCDynamicArray< SScissorDesc, Config::RenderSystem::Pipeline::MAX_SCISSOR_COUNT >;
                ViewportArray   vViewports;
                ScissorArray    vScissors;
                bool            enable = true;
            };

            struct SRasterization
            {
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
                SAMPLE_COUNT    sampleCount = SampleCounts::SAMPLE_1;
                bool            enable = false;
            };

            struct SDepthStencil
            {
                struct
                {
                    bool                    enable = true;
                    bool                    enableTest = false;
                    bool                    enableWrite = false;
                    COMPARE_FUNCTION        compareFunc = CompareFunctions::GREATER_EQUAL;

                    struct
                    {
                        bool        enable = false;
                        float       min = 0.0f;
                        float       max = 0.0f;
                    } Bounds;
                } Depth;
                struct
                {
                    bool                    enable = false;
                    bool                    enableTest = false;
                    SStencilOperationDesc   FrontFace;
                    SStencilOperationDesc   BackFace;
                } Stencil;
            };

            struct SInputLayout
            {
                struct SVertexAttribute
                {
                    SVertexAttribute() {}
                    SVertexAttribute(DEFAULT_CTOR_INIT) :
                        pName( "" ), format{ Formats::R32G32B32_SFLOAT }, vertexBufferBindingIndex{ 0 }, location{ 0 },
                        offset{ 0 }, stride{ 3 * 4 }, inputRate{ VertexInputRates::VERTEX }
                    {}
                    SVertexAttribute( cstr_t name, const FORMAT& fmt, uint16_t bindingLocation ) :
                        pName{ name }, format{ fmt }, vertexBufferBindingIndex{ 0 },
                        location{ bindingLocation }, offset{ 0 }, stride{ CalcFormatSize( fmt ) }, inputRate{ VertexInputRates::VERTEX }
                    {}
                    SVertexAttribute(cstr_t name, const FORMAT& fmt, const uint16_t& bufferBindingIdx,
                        const uint16_t& bindingLocation, const uint16_t& off, const uint16_t& strd,
                        const VERTEX_INPUT_RATE& rate) :
                        pName{ name }, format{ fmt }, vertexBufferBindingIndex{ bufferBindingIdx },
                        location{ bindingLocation }, offset{ off }, stride{ strd }, inputRate{ rate }
                    {}

                    cstr_t              pName;
                    FORMAT              format;
                    uint16_t            location;
                    uint16_t            vertexBufferBindingIndex;
                    uint16_t            offset = 0;
                    uint16_t            stride;
                    VERTEX_INPUT_RATE   inputRate = VertexInputRates::VERTEX;
                };
                using SVertexAttributeArray = Utils::TCDynamicArray< SVertexAttribute, Config::RenderSystem::Pipeline::MAX_VERTEX_ATTRIBUTE_COUNT >;

                SVertexAttributeArray   vVertexAttributes;
                PRIMITIVE_TOPOLOGY      topology = PrimitiveTopologies::TRIANGLE_LIST;
                bool                    enablePrimitiveRestart = false;
            };

            struct STesselation
            {
                bool enable = false;
            };

            SPipelineDesc() {}
            ~SPipelineDesc() {}
            SPipelineDesc& operator=( const SPipelineDesc& ) = default;

            SShaders                    Shaders;
            SBlending                   Blending;
            SRasterization              Rasterization;
            SViewport                   Viewport;
            SMultisampling              Multisampling;
            SDepthStencil               DepthStencil;
            SInputLayout                InputLayout;
            STesselation                Tesselation;
            PipelineLayoutHandle        hLayout = INVALID_HANDLE;
            DDIPipelineLayout           hDDILayout = DDI_NULL_HANDLE;
            RenderPassHandle            hRenderPass = INVALID_HANDLE;
            DDIRenderPass               hDDIRenderPass = DDI_NULL_HANDLE;
            DDIPipeline                 hDDIParent = DDI_NULL_HANDLE;
            PipelinePtr                 pDefault;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct SPipelineCreateDesc
        {
            Core::SCreateResourceInfo     Create;
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
                CONSTANT_BUFFER         = VKE_BIT( 5 ),
                STORAGE_BUFFER          = VKE_BIT( 6 ),
                INDEX_BUFFER            = VKE_BIT( 7 ),
                VERTEX_BUFFER           = VKE_BIT( 8 ),
                INDIRECT_BUFFER         = VKE_BIT( 9 )
            };
        };
        using BUFFER_USAGE = uint32_t;

        struct SBufferRegion
        {
            SBufferRegion() = default;
            SBufferRegion(const uint32_t& elemCount, const uint32_t& elemSize) :
                elementCount{ elemCount }, elementSize{ elemSize } {}
            uint32_t    elementCount; // max number of elements
            uint32_t    elementSize; // size of one element
        };

        struct SBufferDesc
        {
            using BufferRegions = Utils::TCDynamicArray< SBufferRegion >;

            MEMORY_USAGE    memoryUsage;
            BUFFER_USAGE    usage;
            INDEX_TYPE      indexType;
            uint32_t        size; // if 0 size is  calculated based on vRegions
            const void*     pData = nullptr;
            uint32_t        dataSize = 0;
            BufferRegions   vRegions;
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
                POSITION = POSITION3,
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
            using AttributeArray = Utils::TCDynamicArray< SVertexAttributeDesc, 8 >;
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
            Core::SCreateResourceInfo Create;
            SBufferDesc         Buffer;
        };

        struct SCreateVertexBufferDesc
        {
            Core::SCreateResourceInfo Create;
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
            struct SPushConstantDesc
            {
                PIPELINE_STAGES stages;
                uint16_t        size;
                uint16_t        offset;
            };

            static const auto MAX_COUNT = Config::RenderSystem::Pipeline::MAX_PIPELINE_LAYOUT_DESCRIPTOR_SET_COUNT;
            using DescSetLayoutArray = Utils::TCDynamicArray< DescriptorSetLayoutHandle, MAX_COUNT >;
            using PushConstantArray = Utils::TCDynamicArray< SPushConstantDesc, 4 >;

            SPipelineLayoutDesc() {}
            SPipelineLayoutDesc( DescriptorSetLayoutHandle hLayout )
            {
                vDescriptorSetLayouts.PushBack( hLayout );
            }

            DescSetLayoutArray  vDescriptorSetLayouts;
            PushConstantArray   vPushConstants;
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
            handle_t    hMemory     = INVALID_HANDLE;
            uint32_t    offset      = 0;
        };

        struct SUpdateMemoryInfo
        {
            const void*     pData;
            uint32_t        dataSize;
            uint32_t        dstDataOffset = 0;
            VKE_RENDER_SYSTEM_DEBUG_INFO;
        };

        struct SUpdateStagingBufferInfo
        {
            uint32_t    hLockedStagingBuffer;
            uint32_t    stagingBufferOffset;
            const void* pSrcData;
            uint32_t    dataSize;
            uint32_t    dataAlignedSize;
        };

        struct SUnlockBufferInfo
        {
            uint32_t    hUpdateInfo;
            uint32_t    dstBufferOffset;
            CBuffer*    pDstBuffer;
            VKE_RENDER_SYSTEM_DEBUG_INFO;
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

        struct SBindDDIDescriptorSetsInfo
        {
            DDICommandBuffer        hDDICommandBuffer;
            const DDIDescriptorSet* aDDISetHandles;
            const uint32_t*         aDynamicOffsets = nullptr;
            DDIPipelineLayout       hDDIPipelineLayout;
            uint16_t                firstSet;
            uint16_t                setCount;
            uint16_t                dynamicOffsetCount = 0;
            PIPELINE_TYPE           pipelineType;
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

        struct ExecuteCommandBufferFlags
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
        using EXECUTE_COMMAND_BUFFER_FLAGS = uint32_t;

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

        struct SDrawIndirectParams
        {
            uint32_t    vertexCount;
            uint32_t    instanceCount;
            uint32_t    startVertex;
            uint32_t    startInstance;
        };

        struct SDrawIndexedIndirectParams
        {
            uint32_t    indexCount;
            uint32_t    instanceCount;
            uint32_t    startIndex;
            uint32_t    vertexOffset;
            uint32_t    startInstance;
        };

        struct SDrawMeshIndirectParams
        {
            uint32_t    taskCount;
            uint32_t    startTask;
        };

        struct SDrawParams
        {
            union
            {
                struct
                {
                    uint32_t    vertexCount;
                    uint32_t    instanceCount;
                    uint32_t    startVertex;
                    uint32_t    startInstance;
                } Draw;

                struct
                {
                    uint32_t    indexCount;
                    uint32_t    instanceCount;
                    uint32_t    startIndex;
                    uint32_t    vertexOffset;
                    uint32_t    startInstance;
                } Indexed;

                struct
                {
                    DDIBuffer   hArgumentBuffer;
                    uint32_t    offset;
                    uint32_t    drawCount;
                    uint32_t    stride;
                } Indirect;

                struct
                {
                    DDIBuffer   hArgumentBuffer;
                    DDIBuffer   hCountBuffer;
                    uint32_t    offset;
                    uint32_t    countOffset;
                    uint32_t    maxCount;
                    uint32_t    stride;
                } IndirectCount;

                struct
                {
                    DDIBuffer   hArgumentBuffer;
                    uint32_t    offset;
                    uint32_t    count;
                    uint32_t    stride;
                } IndexedIndirect;

                struct
                {
                    uint32_t    count;
                    uint32_t    first;
                } Mesh;

                struct
                {
                    DDIBuffer   hArgumentBuffer;
                    uint32_t    offset;
                    uint32_t    count;
                    uint32_t    stride;
                } MeshIndirect;

                struct
                {
                    DDIBuffer   hArgumentBuffer;
                    DDIBuffer   hCountBuffer;
                    uint32_t    offset;
                    uint32_t    countOffset;
                    uint32_t    maxCount;
                    uint32_t    stride;
                } MeshIndirectCount;
            };
        };

        struct DrawTypes
        {
            enum TYPE
            {
                DRAW,
                INDEXED,
                INDIRECT,
                INDEXED_DINRECT,
                INDIRECT_COUNT,
                INDEXED_INDIRECT_COUNT,
                MESH,
                MESH_INDIRECT,
                MESH_INDIRECT_COUNT,
                _MAX_COUNT
            };
        };
        using DRAW_TYPE = DrawTypes::TYPE;

        struct SStagingBufferInfo
        {
            handle_t    hMemory;
            DDIBuffer   hDDIBuffer;
            uint32_t    sizeLeft;
            uint32_t    alignedSize;
            uint32_t    offset;
        };

#define VKE_ADD_DDI_OBJECT(_type) \
        protected: _type  m_hDDIObject = DDI_NULL_HANDLE; \
        public: vke_force_inline const _type& GetDDIObject() const { return m_hDDIObject; }

    } // RenderSystem

    using SRenderSystemDesc = RenderSystem::SRenderSystemDesc;

} // VKE
