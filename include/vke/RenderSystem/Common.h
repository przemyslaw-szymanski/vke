#pragma once

#include "Core/VKECommon.h"
#include "RenderSystem/VKEImageFormats.h"
// #include "Core/Platform/CWindow.h"
#include "Config.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Common.h"
#include "Core/Resources/CResource.h"
#include "Core/Utils/CLogger.h"
#include "Core/Utils/TCConstantArray.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/TCString.h"
#include "Core/VKEForwardDeclarations.h"
#include "RenderSystem/RHITypes.h"

// #ifdef OPTIONAL
#pragma push_macro( "OPTIONAL" )
#undef OPTIONAL
// #endif

#ifdef DOMAIN
#pragma push_macro( "DOMAIN" )
#undef DOMAIN
#endif

#ifndef VKE_RENDER_SYSTEM_DEBUG
#if VKE_DEBUG
#define VKE_RENDER_SYSTEM_DEBUG 1
#else
#define VKE_RENDER_SYSTEM_DEBUG 0
#endif // VKE_DEBUG
#endif // VKE_RENDER_SYSTEM_DEBUG

namespace VKE
{

#if VKE_RENDER_SYSTEM_DEBUG || VKE_DEBUG
#define VKE_RENDER_SYSTEM_DEBUG_CODE( _code ) _code
#define VKE_RENDER_SYSTEM_DEBUG_NAME                                                                                   \
    ResourceName _DbgName = "";                                                                                        \
    void         SetDebugName( cstr_t pName )                                                                          \
    {                                                                                                                  \
        _DbgName = pName;                                                                                              \
    }                                                                                                                  \
    template< typename... ArgsT >                                                                                      \
    void SetDebugName( cstr_t pFormat, ArgsT... args )                                                                 \
    {                                                                                                                  \
        _DbgName.Format( pFormat, std::forward< ArgsT >( args )... );                                                  \
    }                                                                                                                  \
    cstr_t GetDebugName() const                                                                                        \
    {                                                                                                                  \
        return _DbgName.GetData();                                                                                     \
    }                                                                                                                  \
    bool IsDebugNameEmpty() const                                                                                      \
    {                                                                                                                  \
        return _DbgName.IsEmpty();                                                                                     \
    }
#define VKE_RENDER_SYSTEM_DEBUG_INFO SDebugInfo* pDebugInfo = nullptr
#define VKE_RENDER_SYSTEM_BEGIN_DEBUG_INFO( _pCmdBuff, _obj ) ( _pCmdBuff )->BeginDebugInfo( ( _obj ).pDebugInfo )
#define VKE_RENDER_SYSTEM_END_DEBUG_INFO( _pCmdBuff ) ( _pCmdBuff )->EndDebugInfo()
#define VKE_RENDER_SYSTEM_SET_DEBUG_INFO( _obj, _text, _Color )                                                        \
    VKE::RenderSystem::SDebugInfo __dbgInfo;                                                                           \
    __dbgInfo.pText     = ( _text );                                                                                   \
    __dbgInfo.Color     = ( _Color );                                                                                  \
    ( _obj ).pDebugInfo = &__dbgInfo;
#else
#define VKE_RENDER_SYSTEM_DEBUG_CODE( _code )
#define VKE_RENDER_SYSTEM_DEBUG_NAME                                                                                   \
    template< typename... ArgsT >                                                                                      \
    void SetDebugName( ArgsT... )                                                                                      \
    {                                                                                                                  \
    }                                                                                                                  \
    cstr_t GetDebugName() const                                                                                        \
    {                                                                                                                  \
        return "";                                                                                                     \
    }                                                                                                                  \
    bool IsDebugNameEmpty() const                                                                                      \
    {                                                                                                                  \
        return true;                                                                                                   \
    }
#define VKE_RENDER_SYSTEM_DEBUG_INFO
#define VKE_RENDER_SYSTEM_BEGIN_DEBUG_INFO( _pCmdBuff, _obj )
#define VKE_RENDER_SYSTEM_END_DEBUG_INFO( _pCmdBuff )
#define VKE_RENDER_SYSTEM_SET_DEBUG_INFO( _obj, _text, _Color )
#endif // VKE_RENDER_SYSTEM_DEBUG

#define VKE_RENDER_SYSTEM_SET_DEBUG_NAME( _obj, _name ) VKE_DEBUG_CODE( ( _obj ).Name = _name )
#if VKE_RENDER_SYSTEM_DEBUG
#define VKE_RENDER_SYSTEM_GET_DEBUG_NAME( _obj ) ( _obj ).Name
// #   define VKE_RENDER_SYSTEM_SET_DEBUG_INFO(_obj, _color, _text ) do{ (_obj).pDebugInfo->Color = (_color);
// (_obj).pDebugInfo->pText = (_text); }while(0,0)
#else
#define VKE_RENDER_SYSTEM_GET_DEBUG_NAME( _obj ) ""
// #   define VKE_RENDER_SYSTEM_SET_DEBUG_INFO( _dbgInfo, _color, _text )
#endif // VKE_RENDER_SYSTEM_DEBUG

#if VKE_USE_GLSL_COMPILER
#define VKE_SHADER_COMPILER_STR( _str ) _str
#elif VKE_USE_DIRECTX_SHADER_COMPILER
#define VKE_SHADER_COMPILER_STR( _str ) L##_str
#endif

#if !defined( VKE_USE_HLSL_SYNTAX )
#define VKE_USE_HLSL_SYNTAX 0
#endif

#if VKE_USE_GLSL_COMPILER
    using ShaderCompilerStrType  = cstr_t;
    using ShaderCompilerCharType = char;
#elif VKE_USE_DIRECTX_SHADER_COMPILER
    using ShaderCompilerStrType  = cwstr_t;
    using ShaderCompilerCharType = wchar_t;
#endif

    using ShaderCompilerString = Utils::TCString< ShaderCompilerCharType >;

    class CRenderSystem;

    template< typename T >
    struct TSArray
    {
        uint32_t count = 0;
        const T* pData = nullptr;

        vke_force_inline const T& operator[]( uint32_t idx ) const
        {
            return pData[ idx ];
        }
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

        // using TextureHandle = _STagHandle< TextureTag >;
        // using TextureViewHandle = _STagHandle< TextureViewTag >;
        // using RenderTargetHandle = _STagHandle< RenderTargetTag >;
        // using RenderPassHandle = _STagHandle< RenderPassTag >;
        // using RenderingPipelineHandle = _STagHandle< RenderingPipelineTag >;
        // using SamplerHandle = _STagHandle< SamplerTag >;
        // using FramebufferHandle = _STagHandle< FramebufferTag >;
        // using ShaderHandle = _STagHandle< ShaderTag >;
        // using ShaderProgramHandle = _STagHandle< ShaderProgramTag >;

        class CContextBase;
        class CDeviceContext;

        using StringVec = Utils::TCDynamicArray< vke_string >;
        using CStrVec   = Utils::TCDynamicArray< cstr_t >;

        template< uint32_t DEFAULT_COUNT = 32 >
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

        struct FeatureLevels
        {
            enum LEVEL : uint8_t
            {
                /// <summary>
                /// Highest available
                /// </summary>
                LEVEL_DEFAULT,
                /// <summary>
                /// Basic functionality
                /// </summary>
                LEVEL_1_0,
                /// <summary>
                /// Adds:
                /// - Timeline semaphores
                /// </summary>
                LEVEL_1_1,
                /// <summary>
                /// Adds:
                /// - Bindless resource access
                /// </summary>
                LEVEL_1_2,
                /// <summary>
                /// Adds:
                /// - Raytracing
                /// - Mesh Shaders
                /// - VRS
                /// </summary>
                LEVEL_1_3,
                /// <summary>
                ///
                /// </summary>
                LEVEL_1_4,
                _MAX_COUNT,
                /// <summary>
                /// Highest available
                /// </summary>
                LEVEL_ULTIMATE = LEVEL_1_4,
            };
        };

        using FEATURE_LEVEL = FeatureLevels::LEVEL;

        static vke_force_inline uint16_t CalcFormatSize( const FORMAT& fmt )
        {
            // Size in bits
            static const uint16_t aSizes[] = {
                0,             // UNDEFINED,
                2 * 4,         // R4G4_UNORM_PACK8,
                3 * 4,         // R4G4B4A4_UNORM_PACK16,
                3 * 4,         // B4G4R4A4_UNORM_PACK16,
                5 + 6 + 5,     // R5G6B5_UNORM_PACK16,
                5 + 6 + 5,     // B5G6R5_UNORM_PACK16,
                5 + 5 + 5 + 1, // R5G5B5A1_UNORM_PACK16,
                5 + 5 + 5 + 1, // B5G5R5A1_UNORM_PACK16,
                1 + 5 + 5 + 5, // A1R5G5B5_UNORM_PACK16,
                8,             // R8_UNORM,
                8,             // R8_SNORM,
                8,             // R8_USCALED,
                8,             // R8_SSCALED,
                8,             // R8_UINT,
                8,             // R8_SINT,
                8,             // R8_SRGB,
                8 + 8,         // R8G8_UNORM,
                8 + 8,         // R8G8_SNORM,
                8 + 8,         // R8G8_USCALED,
                8 + 8,         // R8G8_SSCALED,
                8 + 8,         // R8G8_UINT,
                8 + 8,         // R8G8_SINT,
                8 + 8,         // R8G8_SRGB,
                3 * 8,         // R8G8B8_UNORM,
                3 * 8,         // R8G8B8_SNORM,
                3 * 8,         // R8G8B8_USCALED,
                3 * 8,         // R8G8B8_SSCALED,
                3 * 8,         // R8G8B8_UINT,
                3 * 8,         // R8G8B8_SINT,
                3 * 8,         // R8G8B8_SRGB,
                3 * 8,         // B8G8R8_UNORM,
                3 * 8,         // B8G8R8_SNORM,
                3 * 8,         // B8G8R8_USCALED,
                3 * 8,         // B8G8R8_SSCALED,
                3 * 8,         // B8G8R8_UINT,
                3 * 8,         // B8G8R8_SINT,
                3 * 8,         // B8G8R8_SRGB,
                4 * 8,         // R8G8B8A8_UNORM,
                4 * 8,         // R8G8B8A8_SNORM,
                4 * 8,         // R8G8B8A8_USCALED,
                4 * 8,         // R8G8B8A8_SSCALED,
                4 * 8,         // R8G8B8A8_UINT,
                4 * 8,         // R8G8B8A8_SINT,
                4 * 8,         // R8G8B8A8_SRGB,
                4 * 8,         // B8G8R8A8_UNORM,
                4 * 8,         // B8G8R8A8_SNORM,
                4 * 8,         // B8G8R8A8_USCALED,
                4 * 8,         // B8G8R8A8_SSCALED,
                4 * 8,         // B8G8R8A8_UINT,
                4 * 8,         // B8G8R8A8_SINT,
                4 * 8,         // B8G8R8A8_SRGB,
                4 * 8,         // A8B8G8R8_UNORM_PACK32,
                4 * 8,         // A8B8G8R8_SNORM_PACK32,
                4 * 8,         // A8B8G8R8_USCALED_PACK32,
                4 * 8,         // A8B8G8R8_SSCALED_PACK32,
                4 * 8,         // A8B8G8R8_UINT_PACK32,
                4 * 8,         // A8B8G8R8_SINT_PACK32,
                4 * 8,         // A8B8G8R8_SRGB_PACK32,
                2 + 3 * 10,    // A2R10G10B10_UNORM_PACK32,
                2 + 3 * 10,    // A2R10G10B10_SNORM_PACK32,
                2 + 3 * 10,    // A2R10G10B10_USCALED_PACK32,
                2 + 3 * 10,    // A2R10G10B10_SSCALED_PACK32,
                2 + 3 * 10,    // A2R10G10B10_UINT_PACK32,
                2 + 3 * 10,    // A2R10G10B10_SINT_PACK32,
                2 + 3 * 10,    // A2B10G10R10_UNORM_PACK32,
                2 + 3 * 10,    // A2B10G10R10_SNORM_PACK32,
                2 + 3 * 10,    // A2B10G10R10_USCALED_PACK32,
                2 + 3 * 10,    // A2B10G10R10_SSCALED_PACK32,
                2 + 3 * 10,    // A2B10G10R10_UINT_PACK32,
                2 + 3 * 10,    // A2B10G10R10_SINT_PACK32,
                16,            // R16_UNORM,
                16,            // R16_SNORM,
                16,            // R16_USCALED,
                16,            // R16_SSCALED,
                16,            // R16_UINT,
                16,            // R16_SINT,
                16,            // R16_SFLOAT,
                2 * 16,        // R16G16_UNORM,
                2 * 16,        // R16G16_SNORM,
                2 * 16,        // R16G16_USCALED,
                2 * 16,        // R16G16_SSCALED,
                2 * 16,        // R16G16_UINT,
                2 * 16,        // R16G16_SINT,
                2 * 16,        // R16G16_SFLOAT,
                3 * 16,        // R16G16B16_UNORM,
                3 * 16,        // R16G16B16_SNORM,
                3 * 16,        // R16G16B16_USCALED,
                3 * 16,        // R16G16B16_SSCALED,
                3 * 16,        // R16G16B16_UINT,
                3 * 16,        // R16G16B16_SINT,
                3 * 16,        // R16G16B16_SFLOAT,
                4 * 16,        // R16G16B16A16_UNORM,
                4 * 16,        // R16G16B16A16_SNORM,
                4 * 16,        // R16G16B16A16_USCALED,
                4 * 16,        // R16G16B16A16_SSCALED,
                4 * 16,        // R16G16B16A16_UINT,
                4 * 16,        // R16G16B16A16_SINT,
                4 * 16,        // R16G16B16A16_SFLOAT,
                32,            // R32_UINT,
                32,            // R32_SINT,
                32,            // R32_SFLOAT,
                2 * 32,        // R32G32_UINT,
                2 * 32,        // R32G32_SINT,
                2 * 32,        // R32G32_SFLOAT,
                3 * 32,        // R32G32B32_UINT,
                3 * 32,        // R32G32B32_SINT,
                3 * 32,        // R32G32B32_SFLOAT,
                4 * 32,        // R32G32B32A32_UINT,
                4 * 32,        // R32G32B32A32_SINT,
                4 * 32,        // R32G32B32A32_SFLOAT,
                64,            // R64_UINT,
                64,            // R64_SINT,
                64,            // R64_SFLOAT,
                2 * 64,        // R64G64_UINT,
                2 * 64,        // R64G64_SINT,
                2 * 64,        // R64G64_SFLOAT,
                3 * 64,        // R64G64B64_UINT,
                3 * 64,        // R64G64B64_SINT,
                3 * 64,        // R64G64B64_SFLOAT,
                4 * 64,        // R64G64B64A64_UINT,
                4 * 64,        // R64G64B64A64_SINT,
                4 * 64,        // R64G64B64A64_SFLOAT,
                10 + 2 * 11,   // B10G11R11_UFLOAT_PACK32,
                5 + 3 * 9,     // E5B9G9R9_UFLOAT_PACK32,
                1 * 16,        // D16_UNORM,
                32,            // X8_D24_UNORM_PACK32,
                32,            // D32_SFLOAT,
                8,             // S8_UINT,
                16,            // D16_UNORM_S8_UINT,
                32,            // D24_UNORM_S8_UINT,
                32,            // D32_SFLOAT_S8_UINT,
                64,            // BC1_RGB_UNORM_BLOCK,
                64,            // BC1_RGB_SRGB_BLOCK,
                64,            // BC1_RGBA_UNORM_BLOCK,
                64,            // BC1_RGBA_SRGB_BLOCK,
                128,           // BC2_UNORM_BLOCK,
                128,           // BC2_SRGB_BLOCK,
                128,           // BC3_UNORM_BLOCK,
                128,           // BC3_SRGB_BLOCK,
                64,            // BC4_UNORM_BLOCK,
                64,            // BC4_SNORM_BLOCK,
                128,           // BC5_UNORM_BLOCK,
                128,           // BC5_SNORM_BLOCK,
                128,           // BC6H_UFLOAT_BLOCK,
                128,           // BC6H_SFLOAT_BLOCK,
                128,           // BC7_UNORM_BLOCK,
                128,           // BC7_SRGB_BLOCK,
                128,           // ETC2_R8G8B8_UNORM_BLOCK,
                128,           // ETC2_R8G8B8_SRGB_BLOCK,
                64,            // ETC2_R8G8B8A1_UNORM_BLOCK,
                64,            // ETC2_R8G8B8A1_SRGB_BLOCK,
                128,           // ETC2_R8G8B8A8_UNORM_BLOCK,
                128,           // ETC2_R8G8B8A8_SRGB_BLOCK,
                128,           // EAC_R11_UNORM_BLOCK,
                128,           // EAC_R11_SNORM_BLOCK,
                128,           // EAC_R11G11_UNORM_BLOCK,
                128,           // EAC_R11G11_SNORM_BLOCK,
                128,           // ASTC_4x4_UNORM_BLOCK,
                128,           // ASTC_4x4_SRGB_BLOCK,
                128,           // ASTC_5x4_UNORM_BLOCK,
                128,           // ASTC_5x4_SRGB_BLOCK,
                128,           // ASTC_5x5_UNORM_BLOCK,
                128,           // ASTC_5x5_SRGB_BLOCK,
                128,           // ASTC_6x5_UNORM_BLOCK,
                128,           // ASTC_6x5_SRGB_BLOCK,
                128,           // ASTC_6x6_UNORM_BLOCK,
                128,           // ASTC_6x6_SRGB_BLOCK,
                128,           // ASTC_8x5_UNORM_BLOCK,
                128,           // ASTC_8x5_SRGB_BLOCK,
                128,           // ASTC_8x6_UNORM_BLOCK,
                128,           // ASTC_8x6_SRGB_BLOCK,
                128,           // ASTC_8x8_UNORM_BLOCK,
                128,           // ASTC_8x8_SRGB_BLOCK,
                128,           // ASTC_10x5_UNORM_BLOCK,
                128,           // ASTC_10x5_SRGB_BLOCK,
                128,           // ASTC_10x6_UNORM_BLOCK,
                128,           // ASTC_10x6_SRGB_BLOCK,
                128,           // ASTC_10x8_UNORM_BLOCK,
                128,           // ASTC_10x8_SRGB_BLOCK,
                128,           // ASTC_10x10_UNORM_BLOCK,
                128,           // ASTC_10x10_SRGB_BLOCK,
                128,           // ASTC_12x10_UNORM_BLOCK,
                128,           // ASTC_12x10_SRGB_BLOCK,
                128,           // ASTC_12x12_UNORM_BLOCK,
                128,           // ASTC_12x12_SRGB_BLOCK,

            };
            return aSizes[ fmt ] / 8; // convert to bytes
        }

        struct ApiObjectTypes
        {
            enum TYPE
            {
                UNKNOWN               = 0,
                INSTANCE              = 1,
                ADAPTER               = 2,
                DEVICE                = 3,
                CONTEXT               = 4,
                GPU_FENCE             = 5,
                COMMAND_BUFFER        = 6,
                CPU_FENCE             = 7,
                DEVICE_MEMORY         = 8,
                BUFFER                = 9,
                TEXTURE               = 10,
                EVENT                 = 11,
                QUERY_POOL            = 12,
                BUFFER_VIEW           = 13,
                TEXTURE_VIEW          = 14,
                SHADER                = 15,
                PIPELINE_CACHE        = 16,
                PIPELINE_LAYOUT       = 17,
                RENDER_PASS           = 18,
                PIPELINE              = 19,
                DESCRIPTOR_SET_LAYOUT = 20,
                SAMPLER               = 21,
                DESCRIPTOR_POOL       = 22,
                DESCRIPTOR_SET        = 23,
                FRAMEBUFFER           = 24,
                COMMAND_POOL          = 25,
                _MAX_COUNT
            };
        };

        using API_OBJECT_TYPE = ApiObjectTypes::TYPE;

        struct SAPIAppInfo
        {
            uint32_t applicationVersion;
            uint32_t engineVersion;
            cstr_t   pEngineName;
            cstr_t   pApplicationName;
        };

        using TextureSizeType = uint16_t;
        using TextureSize     = TSExtent< TextureSizeType >;
        using BufferSizeType  = uint32_t;

        struct VKE_API SColor
        {
            union
            {
                struct
                {
                    float r, g, b, a;
                };

                float    floats[ 4 ];
                uint32_t uints[ 4 ];
                int32_t  ints[ 4 ];
                uint8_t  bytes[ 4 ];
            };

            SColor()
            {
            }

            SColor( const SColor& Other ) : r( Other.r ), g( Other.g ), b( Other.b ), a( Other.a )
            {
            }

            explicit SColor( uint32_t v ) :
                r( static_cast< float >( v ) ), g( static_cast< float >( v ) ), b( static_cast< float >( v ) ),
                a( static_cast< float >( v ) )
            {
            }

            SColor( float red, float green, float blue, float alpha ) : r( red ), g( green ), b( blue ), a( alpha )
            {
            }

            explicit SColor( float v ) : r( v ), g( v ), b( v ), a( v )
            {
            }

            void operator=( float v )
            {
                r = g = b = a = v;
            }

            void CopyToNative( void* pNativeArray ) const;

            static const SColor ZERO;
            static const SColor ONE;
            static const SColor RED;
            static const SColor GREEN;
            static const SColor BLUE;
            static const SColor ALPHA;
        };

        struct DebugColors
        {
            static const SColor UPLOAD;
            static const SColor DOWNLOAD;
            static const SColor DRAW;
            static const SColor DISPATCH;
            static const SColor DISPATCH_RAYS;
            static const SColor DISPATCH_MESH;
            static const SColor COPY;
            static const SColor BARRIER;
            static const SColor BEGIN_RENDER_PASS;
            static const SColor END_RENDER_PASS;
        };

        struct VKE_API SDepthStencilValue
        {
            SDepthStencilValue()
            {
            }

            SDepthStencilValue( float d, uint32_t s ) : depth{ d }, stencil{ s }
            {
            }

            float    depth;
            uint32_t stencil;
        };

        struct VKE_API SClearValue
        {
            union
            {
                SColor             Color;
                SDepthStencilValue DepthStencil;
            };

            SClearValue()
            {
            }

            SClearValue( const SClearValue& V ) : Color( V.Color )
            {
            }

            SClearValue( const SColor& C ) : Color{ C }
            {
            }

            SClearValue( const SDepthStencilValue& DS ) : DepthStencil{ DS }
            {
            }

            SClearValue( float r, float g, float b, float a ) : Color( r, g, b, a )
            {
            }

            SClearValue( float d, uint32_t s ) : DepthStencil( d, s )
            {
            }
        };

        struct SViewportDesc
        {
            ExtentF32 Position;
            ExtentF32 Size;
            ExtentF32 MinMaxDepth = { 0.0f, 1.0f };

            uint32_t CalcHash() const
            {
                Utils::SHash Hash;
                Hash.Combine( Position.x, Position.y, Size.width, Size.height, MinMaxDepth.min, MinMaxDepth.max );
                return static_cast< uint32_t >( Hash.value );
            }
        };

        struct SScissorDesc
        {
            ExtentI32 Position;
            ExtentU32 Size;

            uint32_t CalcHash() const
            {
                Utils::SHash Hash;
                Hash.Combine( Position.x, Position.y, Size.width, Size.height );
                return static_cast< uint32_t >( Hash.value );
            }
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

        struct SDebugInfo
        {
            cstr_t pText;
            SColor Color;
        };

        enum RES_ID_TYPE
        {
            RES_ID_NAME,
            RES_ID_HANDLE,
            RES_ID_POINTER,
            RES_ID_INDEX
        };

        template< class NameT, class HandleT, class PtrT >
        struct SResourceID
        {
            union
            {
                NameT    name;
                HandleT  handle;
                PtrT     ptr;
                uint32_t index;
            };

            RES_ID_TYPE type;

            SResourceID()
            {
            }

            SResourceID( const NameT& n, RES_ID_TYPE t ) : name{ n }, type{ t }
            {
            }

            SResourceID( const HandleT& h, RES_ID_TYPE t ) : handle{ h }, type{ t }
            {
            }

            SResourceID( const PtrT& p, RES_ID_TYPE t ) : ptr{ p }, type{ t }
            {
            }

            SResourceID( const uint32_t& i, RES_ID_TYPE t ) : index{ i }, type{ t }
            {
            }
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
                READ_ONLY_BUFFER,
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
            static const uint32_t RENDER     = VKE_BIT( 0 );
            static const uint32_t COMPUTE    = VKE_BIT( 1 );
            static const uint32_t TRANSFER   = VKE_BIT( 2 );
            static const uint32_t _MAX_COUNT = 3;
            static const uint32_t GENERAL    = RENDER | COMPUTE | TRANSFER;
        };

        using GRAPHICS_QUEUE_TYPE = uint32_t;

        struct SAdapterLimits
        {
        };

        struct SAdapterInfo
        {
            char           name[ Constants::MAX_NAME_LENGTH ];
            SAdapterLimits limits;
            uint32_t       apiVersion;
            uint32_t       driverVersion;
            uint32_t       deviceID;
            uint32_t       vendorID;
            ADAPTER_TYPE   type;
            handle_t       hDDIAdapter;
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
            WindowPtr pWindow = WindowPtr();
            // CGraphicsContext*   pCtx = nullptr;
            void*          pPrivate         = nullptr;
            uint32_t       queueFamilyIndex = 0;
            TextureSize    Size             = { 800, 600 };
            COLOR_SPACE    colorSpace       = ColorSpaces::SRGB;
            TEXTURE_FORMAT format           = Formats::R8G8B8A8_UNORM;
            uint16_t       backBufferCount  = Constants::OPTIMAL;
            bool           enableVSync      = true;
        };

        struct ShaderTypes
        {
            enum TYPE
            {
                VERTEX,
                HULL,
                DOMAIN,
                GEOMETRY,
                PIXEL,
                COMPUTE,
                TASK,
                MESH,
                RAYGEN,
                ANY_HIT,
                CLOSEST_HIT,
                MISS,
                CALLABLE,
                INTERSECTION,
                _MAX_COUNT,
                UNKNOWN = _MAX_COUNT,
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
                PROFILE_6_5,
                PROFILE_6_6,
                PROFILE_6_7,
                _MAX_COUNT,
                UNKNOWN = _MAX_COUNT,
                DEFAULT = PROFILE_6_0
            };
        };

        using SHADER_PROFILE = ShaderProfiles::PROFILE;

        struct PipelineStages
        {
            enum TYPE : uint16_t
            {
                VERTEX          = VKE_BIT( 1 ),
                TS_HULL         = VKE_BIT( 2 ),
                TS_DOMAIN       = VKE_BIT( 3 ),
                GEOMETRY        = VKE_BIT( 4 ),
                PIXEL           = VKE_BIT( 5 ),
                MS_TASK         = VKE_BIT( 6 ),
                MS_MESH         = VKE_BIT( 7 ),
                RT_RAYGEN       = VKE_BIT( 8 ),
                RT_ANY_HIT      = VKE_BIT( 9 ),
                RT_CLOSEST_HIT  = VKE_BIT( 10 ),
                RT_MISS_HIT     = VKE_BIT( 11 ),
                RT_CALLABLE     = VKE_BIT( 12 ),
                RT_INTERSECTION = VKE_BIT( 13 ),
                COMPUTE         = VKE_BIT( 14 ),
                _MAX_COUNT      = 14,
                UNKNOWN         = 0x0,
                MESH            = MS_TASK | MS_MESH,
                RAYTRACING      = RT_RAYGEN | RT_ANY_HIT | RT_CLOSEST_HIT | RT_MISS_HIT | RT_INTERSECTION | RT_CALLABLE,
                ALL             = VERTEX | TS_HULL | TS_DOMAIN | GEOMETRY | PIXEL | MESH | RAYTRACING | COMPUTE
            };
        };

        using PIPELINE_STAGES = uint16_t;

        struct BindingTypes
        {
            enum TYPE : uint8_t
            {
                SAMPLER, // only sampler
                TEXTURE, // only texture without sampler
                STORAGE_TEXTURE,
                READ_ONLY_TEXEL_BUFFER,
                READ_WRITE_TEXEL_BUFFER,
                CONSTANT_BUFFER,
                BUFFER,
                DYNAMIC_CONSTANT_BUFFER,
                DYNAMIC_BUFFER,
                RENDER_TARGET,
                DEPTH_STENCIL,
                _MAX_COUNT,
                UNKNOWN = _MAX_COUNT
            };
        };

        using BINDING_TYPE        = BindingTypes::TYPE;
        using DESCRIPTOR_SET_TYPE = BINDING_TYPE;
        using DescriptorSetTypes  = BindingTypes;
        using DescriptorSetCounts = uint16_t[ DescriptorSetTypes::_MAX_COUNT ];

        struct DescriptorPoolTypes
        {
            enum TYPE
            {
                TEXTURE_BUFFER_CBUFFER,
                SAMPLER,
                COLOR_RENDER_TARGET,
                DEPTH_STENCIL,
                _MAX_COUNT
            };
        };

        using DESCRIPTOR_POOL_TYPE = DescriptorPoolTypes::TYPE;

        vke_force_inline DESCRIPTOR_POOL_TYPE BindingTypeToPoolType( DESCRIPTOR_SET_TYPE bindingType )
        {
            static const DESCRIPTOR_POOL_TYPE ascValues[ BindingTypes::_MAX_COUNT ] = {
                DescriptorPoolTypes::SAMPLER,                // SAMPLER
                DescriptorPoolTypes::TEXTURE_BUFFER_CBUFFER, // TEXTURE
                DescriptorPoolTypes::TEXTURE_BUFFER_CBUFFER, // STORAGE_TEXTURE
                DescriptorPoolTypes::TEXTURE_BUFFER_CBUFFER, // READ_ONLY_TEXEL_BUFFER
                DescriptorPoolTypes::TEXTURE_BUFFER_CBUFFER, // READ_WRITE_TEXEL_BUFFER
                DescriptorPoolTypes::TEXTURE_BUFFER_CBUFFER, // CONSTANT_BUFFER
                DescriptorPoolTypes::TEXTURE_BUFFER_CBUFFER, // BUFFER
                DescriptorPoolTypes::TEXTURE_BUFFER_CBUFFER, // DYNAMIC_CONSTANT_BUFFER
                DescriptorPoolTypes::TEXTURE_BUFFER_CBUFFER, // DYNAMIC_BUFFER
                // RENDER_TARGET
                // DEPTH_STENCIL
            };
            return ascValues[ bindingType ];
        }

        struct SResourceBinding
        {
            SResourceBinding()
            {
            }

            SResourceBinding( uint8_t idx ) : index{ idx }, set{ 0 }, stages{ PipelineStages::ALL }, count{ 1 }
            {
            }

            SResourceBinding( uint8_t idx, PIPELINE_STAGES s ) : index{ idx }, set{ 0 }, stages{ s }, count{ 1 }
            {
            }

            SResourceBinding( uint8_t idx, PIPELINE_STAGES s, uint16_t c ) :
                index{ idx }, set{ 0 }, stages{ s }, count{ c }
            {
            }

            uint8_t         index;
            uint8_t         set;
            PIPELINE_STAGES stages;
            uint16_t        count;
            uint16_t        space;
        };

        struct STextureBinding : SResourceBinding
        {
            TextureViewHandle hTextureView;
        };

        struct SSamplerBinding : SResourceBinding
        {
            SamplerHandle hSampler;
        };

        struct SSamplerTextureBinding : SResourceBinding
        {
            SamplerHandle     hSampler;
            TextureViewHandle hTextureView;
        };

        struct SDescriptorSetLayoutDesc
        {
            struct SBinding
            {
                uint8_t         idx;
                BINDING_TYPE    type;
                uint16_t        count;
                PIPELINE_STAGES stages;
                uint16_t        space;
            };

            using BindingArray =
                Utils::TCDynamicArray< SBinding, Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_BINDING_COUNT >;

            SDescriptorSetLayoutDesc()
            {
            }

            SDescriptorSetLayoutDesc( DEFAULT_CTOR_INIT )
            {
                vBindings.PushBack( {} );
            }

            BindingArray vBindings;
            VKE_RENDER_SYSTEM_DEBUG_NAME;

            bool IsValid() const
            {
                if( vBindings.IsEmpty() == false )
                {
                    auto currType = BindingTypeToPoolType( vBindings[ 0 ].type );
                    for( uint32_t i = 1; i < vBindings.GetCount(); ++i )
                    {
                        const auto newType = BindingTypeToPoolType( vBindings[ i ].type );
                        if( currType != newType )
                        {
                            VKE_LOG_ERRF( "It is not possible to mix different DescriptorPool types: {} and {}",
                                          (uint32_t)currType,
                                          (uint32_t)newType );
                            return false;
                        }
                    }
                }
                // It is ok to have no bindings
                return true;
            }
        };

        struct SBufferRegion
        {
            SBufferRegion()
            {
            }

            SBufferRegion( size_t elemSize, uint32_t elemCount ) :
                elementSize( (uint32_t)elemSize ), elementCount{ elemCount }
            {
            }

            explicit SBufferRegion( size_t elemSize ) : SBufferRegion( elemSize, 1 )
            {
            }

            uint32_t elementSize;
            uint32_t elementCount = 1;
            uint32_t offset       = 0;
        };

        struct SUpdateBindingsHelper
        {
            template< class HandleType >
            struct TSBinding
            {
                const HandleType* ahHandles;
                uint16_t          count;
                BINDING_TYPE      type;
                uint8_t           binding;
            };

            struct SBufferBinding : TSBinding< BufferHandle >
            {
                uint32_t offset;
                uint32_t elementSize;
                uint32_t elementCount;
            };

            struct SSamplerAndTextureBinding
            {
                const SamplerHandle*     ahSamplers;
                const TextureHandle*     ahTextures;
                const TextureViewHandle* ahTexViews;
                uint16_t                 count;
                BINDING_TYPE             type;
                uint8_t                  binding;
            };

            void AddBinding( uint8_t binding, const RenderTargetHandle* ahHandles, const uint16_t count )
            {
                TSBinding< RenderTargetHandle > Binding;
                Binding.ahHandles = ahHandles;
                Binding.count     = count;
                Binding.binding   = binding;
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

            void AddBinding( uint8_t binding, const TextureViewHandle* ahHandles, const uint16_t count )
            {
                TSBinding< TextureViewHandle > Binding;
                Binding.ahHandles = ahHandles;
                Binding.count     = count;
                Binding.binding   = binding;
                vTexViews.PushBack( Binding );
            }

            void AddBinding( uint8_t binding, const SamplerHandle* ahHandles, const uint16_t count )
            {
                TSBinding< SamplerHandle > Binding;
                Binding.ahHandles = ahHandles;
                Binding.count     = count;
                Binding.binding   = binding;
                vSamplers.PushBack( Binding );
            }

            void AddBinding( uint8_t binding, const SBufferRegion& BufferRegion, const BufferHandle& hBuffer,
                             BINDING_TYPE type )
            {
                AddBinding(
                    binding, BufferRegion.offset, BufferRegion.elementSize, BufferRegion.elementCount, hBuffer, type );
            }

            void AddBinding( uint8_t binding, const uint32_t offset, const uint32_t elementSize, uint32_t elementCount,
                             const BufferHandle& hBuffer, BINDING_TYPE type )
            {
                SBufferBinding Binding;
                Binding.ahHandles    = &hBuffer;
                Binding.count        = 1;
                Binding.binding      = binding;
                Binding.offset       = offset;
                Binding.elementSize  = elementSize;
                Binding.elementCount = elementCount;
                Binding.type         = type;
                vBuffers.PushBack( Binding );
            }

            void AddBinding( uint8_t binding, const uint32_t offset, BufferPtr pBuffer );
            void AddBinding( uint8_t binding, const uint32_t offset, const uint32_t range, BufferPtr pBuffer );

            void Reset()
            {
                vRTs.Clear();
                vTexs.Clear();
                vTexViews.Clear();
                vSamplers.Clear();
                vBuffers.Clear();
                vSamplerAndTextures.Clear();
            }

            template< class HandleType >
            using BindingArray           = Utils::TCDynamicArray< TSBinding< HandleType >, 16 >;
            using RtArray                = BindingArray< RenderTargetHandle >;
            using TexArray               = BindingArray< TextureHandle >;
            using TexViewArray           = BindingArray< TextureViewHandle >;
            using SamplerArray           = BindingArray< SamplerHandle >;
            using BufferArray            = Utils::TCDynamicArray< SBufferBinding, 8 >;
            using SamplerAndTextureArray = Utils::TCDynamicArray< SSamplerAndTextureBinding, 16 >;

            RtArray                vRTs;
            TexArray               vTexs;
            TexViewArray           vTexViews;
            SamplerArray           vSamplers;
            BufferArray            vBuffers;
            SamplerAndTextureArray vSamplerAndTextures;
        };

        struct SCopyDescriptorSetInfo
        {
            DescriptorSetHandle hSrc;
            DescriptorSetHandle hDst;
        };

        struct SGraphicsContextCallbacks
        {
            std::function< void( CGraphicsContext* ) > RenderFrame;
        };

        struct SCommandBufferPoolDesc
        {
            CContextBase* pContext           = nullptr;
            uint32_t      commandBufferCount = Config::RenderSystem::CommandBuffer::DEFAULT_COUNT_IN_POOL;
            uint8_t       threadIndex        = UNDEFINED_U8;
        };

        struct SRenderSystemMemoryInfo
        {
            SRenderSystemMemoryInfo()
            {
                memset( aResourceTypes, UNDEFINED, sizeof( aResourceTypes ) );
            }

            uint16_t aResourceTypes[ ResourceTypes::_MAX_COUNT ];
        };

        struct QueueTypes
        {
            enum TYPE : uint8_t
            {
                GENERAL,
                COMPUTE,
                TRANSFER,
                SPARSE,
                PRESENT,
                _MAX_COUNT,
                UNKNOWN = _MAX_COUNT
            };
        };

        using QUEUE_TYPE    = QueueTypes::TYPE;
        using QueueTypeBits = QueueTypes;
        using CONTEXT_TYPE  = QUEUE_TYPE;
        using ContextTypes  = QueueTypes;

        struct FrameGraphFlagBits
        {
            enum FLAGS
            {
                NONE                = 0x0,
                BASIC_MULTITHREADED = VKE_BIT( 0 ),
                _MAX_COUNT
            };
        };

        using FRAME_GRAPH_FLAGS = FrameGraphFlagBits::FLAGS;

        struct SFrameGraphDesc
        {
            ResourceName      Name                                   = "DefaultMT";
            TextureSize       Size                                   = { 0, 0 };
            CDeviceContext*   pDevice                                = nullptr;
            CContextBase*     apContexts[ ContextTypes::_MAX_COUNT ] = { nullptr };
            FRAME_GRAPH_FLAGS flags                                  = FrameGraphFlagBits::BASIC_MULTITHREADED;
        };

        struct SRenderSystemDesc
        {
            SRenderSystemMemoryInfo MemoryProperties;
            TSArray< SWindowDesc >  Windows;
            SFrameGraphDesc         FrameGraph;
            bool                    debugMode = VKE_RENDER_SYSTEM_DEBUG;

            SRenderSystemDesc()
            {
            }
        };

        struct SRendersystemLimits
        {
        };

        struct SRenderQueueDesc
        {
            RENDER_QUEUE_USAGE usage    = RenderQueueUsages::DYNAMIC;
            uint16_t           priority = 0;
        };

        struct SFramebufferDesc
        {
            using AttachmentArray = Utils::TCDynamicArray< RHI::TextureView, 8 >;
            TextureSize     Size;
            AttachmentArray vDDIAttachments;
            RHI::RenderPass hRenderPass;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
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
            enum BITS : uint8_t
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

        /// <summary>
        /// Note. Every change in this struct requires changes in Map::ImageLayout, Map::ImageUsage
        /// CTexture::ConvertStateToSrcMemoryAccess, CTexture::ConvertStateToDstMemoryAccess
        /// </summary>
        struct TextureStates
        {
            enum STATE : uint8_t
            {
                UNDEFINED,
                GENERAL,
                COLOR_RENDER_TARGET,
                DEPTH_RENDER_TARGET,
                STENCIL_RENDER_TARGET,
                DEPTH_STENCIL_RENDER_TARGET,
                DEPTH_BUFFER,
                STENCIL_BUFFER,
                DEPTH_STENCIL_BUFFER,
                SHADER_READ,
                TRANSFER_SRC,
                TRANSFER_DST,
                PRESENT,
                COLOR_RENDER_TARGET_READ,
                DEPTH_RENDER_TARGET_READ,
                STENCIL_RENDER_TARGET_READ,
                DEPTH_STENCIL_RENDER_TARGET_READ,
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

        struct TextureFilters
        {
            enum FILTER : uint8_t
            {
                NEAREST,
                LINEAR,
                CUBIC,
                _MAX_COUNT
            };
        };

        using TEXTURE_FILTER = TextureFilters::FILTER;

        struct MemoryUsages
        {
            enum BITS : uint8_t
            {
                UNDEFINED            = 0x0,
                DEDICATED_ALLOCATION = VKE_BIT( 0 ),
                CPU_ACCESS           = VKE_BIT( 1 ),
                CPU_NO_FLUSH         = VKE_BIT( 2 ),
                CPU_CACHED           = VKE_BIT( 3 ),
                GPU_ACCESS           = VKE_BIT( 4 ),
                BUFFER               = VKE_BIT( 5 ),
                TEXTURE              = VKE_BIT( 6 ),
                DYNAMIC              = CPU_ACCESS | GPU_ACCESS,
                UPLOAD               = DYNAMIC | CPU_NO_FLUSH,
                STATIC               = GPU_ACCESS,
                DEFAULT              = STATIC,
                STAGING              = CPU_ACCESS | CPU_CACHED,
                STATIC_BUFFER        = STATIC | BUFFER,
                STATIC_TEXTURE       = STATIC | TEXTURE,
                DEFAULT_BUFFER       = DEFAULT | BUFFER,
                DEFAULT_TEXTURE      = DEFAULT | TEXTURE,
                STAGING_BUFFER       = STAGING | BUFFER,
                UPLOAD_BUFFER        = UPLOAD | BUFFER,
                CPU_COHERENT         = CPU_NO_FLUSH,
                GPU_READ             = GPU_ACCESS,
                GPU_WRITE            = GPU_ACCESS,
                CPU_READ             = CPU_ACCESS,
                CPU_WRITE            = CPU_ACCESS,
                CPU_WRITE_GPU_READ   = UPLOAD
            };
        };

        using MEMORY_USAGE = uint8_t;

        struct STextureSubresourceRange
        {
            uint16_t       beginMipmapLevel = 0;
            uint16_t       mipmapLevelCount = 1;
            uint8_t        beginArrayLayer  = 0;
            uint8_t        layerCount       = 1;
            TEXTURE_ASPECT aspect           = TextureAspects::UNKNOWN;
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
            ADDRESS_MODE U = AddressModes::CLAMP_TO_EDGE;
            ADDRESS_MODE V = AddressModes::CLAMP_TO_EDGE;
            ADDRESS_MODE W = AddressModes::CLAMP_TO_EDGE;
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
            SAMPLER_FILTER min = SamplerFilters::NEAREST;
            SAMPLER_FILTER mag = SamplerFilters::NEAREST;
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
            SAddressMode     AddressMode;
            BORDER_COLOR     borderColor = BorderColors::INT_OPAQUE_BLACK;
            COMPARE_FUNCTION compareFunc = CompareFunctions::NEVER;
            SSamplerFilters  Filter;
            MIPMAP_MODE      mipmapMode              = MipmapModes::NEAREST;
            ExtentF32        LOD                     = { 0.0f, 1.0f };
            float            maxAnisotropy           = 1.0f;
            float            mipLODBias              = 0.0f;
            bool             enableCompare           = false;
            bool             enableAnisotropy        = false;
            bool             unnormalizedCoordinates = false;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct STextureFormatProperties
        {
            ExtentU32 MaxSize;
            uint32_t  maxDepth;
            uint32_t  maxMipLevelCount;
            uint32_t  maxArrayLayerCount;
            // VkSampleCountFlags sampleCounts;
            uint32_t maxResourceSize;
        };

        struct SCreateTextureFlags
        {
            enum FLAGS
            {
                NONE                        = 0x0,
                DEDICATED_MEMORY_ALLOCATION = VKE_BIT( 0 ),
            };
        };

        using CREATE_TEXTURE_FLAGS = uint32_t;

        struct STextureDesc
        {
            TextureSize          Size;
            TEXTURE_FORMAT       format            = Formats::R8G8B8A8_UNORM;
            TEXTURE_USAGE        usage             = TextureUsages::SAMPLED;
            TEXTURE_TYPE         type              = TextureTypes::TEXTURE_2D;
            SAMPLE_COUNT         multisampling     = SampleCounts::SAMPLE_1;
            uint16_t             mipmapCount       = 1;
            MEMORY_USAGE         memoryUsage       = MemoryUsages::DEFAULT;
            uint16_t             arrayElementCount = 1;         // number of textures in array
            uint16_t             sliceCount        = 1;         // number of slices in 3d
            RHI::Texture         hNative           = RHI::Null; // create from native
            RHI::TextureView     hNativeView       = RHI::Null; // create from native
            CREATE_TEXTURE_FLAGS flags             = SCreateTextureFlags::NONE;
            // TODO(Any): clearValue is passed to CreateResource for optimized clear paths. This is currently unhandled
            // in Vk.
            SClearValue  optimizedClearValue = { 0.0, 0.0, 0.0, 0.0 };
            ResourceName Name;
            VKE_RENDER_SYSTEM_DEBUG_NAME;

            /*STextureDesc()
            {
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
                Name = Other.Name;
                VKE_RENDER_SYSTEM_SET_DEBUG_NAME( *this, VKE_RENDER_SYSTEM_GET_DEBUG_NAME( Other) );
                return *this;
            }*/
        };

        struct SCreateTextureDesc
        {
            Core::SCreateResourceInfo Create;
            STextureDesc              Texture;
            Core::ImageHandle         hImage = INVALID_HANDLE;
        };

        struct STextureViewDesc
        {
            TextureHandle            hTexture = INVALID_HANDLE;
            TEXTURE_VIEW_TYPE        type     = TextureViewTypes::VIEW_2D;
            TEXTURE_FORMAT           format   = Formats::R8G8B8A8_UNORM;
            STextureSubresourceRange SubresourceRange;
            RHI::TextureView         hNative = RHI::Null;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct SCreateTextureViewDesc
        {
            Core::SCreateResourceInfo Create;
            STextureViewDesc          TextureView;
        };

        struct SAttachmentDesc
        {
            SAMPLE_COUNT   multisampling = SampleCounts::SAMPLE_1;
            TEXTURE_FORMAT format;
        };

        struct RenderTargetRenderPassOperations
        {
            enum USAGE : uint8_t
            {
                UNDEFINED,
                COLOR,             // load = dont't care, store = don't care
                COLOR_CLEAR,       // load = clear, store = dont't care
                COLOR_STORE,       // load = don't care, store = store
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
                    COLOR,             // load = dont't care, store = don't care
                    COLOR_CLEAR,       // load = clear, store = dont't care
                    COLOR_STORE,       // load = don't care, store = store
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

        using RENDER_TARGET_WRITE_USAGE    = RenderTargetRenderPassOperations::Write::USAGE;
        using RENDER_TARGET_READ_USAGE     = RenderTargetRenderPassOperations::Read::USAGE;
        using RENDER_TARGET_RENDER_PASS_OP = RenderTargetRenderPassOperations::USAGE;

        struct SBaseRenderTargetDesc
        {
            ExtentU16     Size;
            FORMAT        format;
            MEMORY_USAGE  memoryUsage;
            TEXTURE_USAGE usage;
            TEXTURE_TYPE  type;
            SAMPLE_COUNT  multisampling = SampleCounts::SAMPLE_1;
            uint16_t      mipmapCount   = 1;
            ResourceName  Name;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct SRenderTargetDesc : SBaseRenderTargetDesc
        {
            TEXTURE_STATE                beginState      = TextureStates::UNDEFINED;
            TEXTURE_STATE                endState        = TextureStates::UNDEFINED;
            RENDER_TARGET_RENDER_PASS_OP renderPassUsage = RenderTargetRenderPassOperations::UNDEFINED;
            SClearValue                  ClearValue      = { { 0, 0, 0, 1 } };
            TextureHandle                hTexture        = INVALID_HANDLE;
        };

        struct RenderTargetIndices
        {
            enum INDEX
            {
                COLOR0,
                COLOR1,
                COLOR2,
                COLOR3,
                COLOR4,
                COLOR5,
                COLOR6,
                COLOR7,
                COLOR8,
                DEPTH0,
                STENCIL0,
                _MAX_COUNT,
                DIFFUSE  = COLOR0,
                NORMAL   = COLOR1,
                SPECULAR = COLOR2
            };
        };

        using RENDER_TARGET_INDEX = RenderTargetIndices::INDEX;

        struct VKE_API SRenderPassDesc
        {
            struct VKE_API SSubpassDesc
            {
                struct VKE_API SRenderTargetDesc
                {
                    TextureViewHandle hTextureView = INVALID_HANDLE;
                    TEXTURE_STATE     state        = TextureStates::UNDEFINED;
                    VKE_RENDER_SYSTEM_DEBUG_NAME;
                };

                using AttachmentDescArray = Utils::TCDynamicArray< SRenderTargetDesc, 8 >;
                AttachmentDescArray vRenderTargets;
                AttachmentDescArray vTextures;
                SRenderTargetDesc   DepthBuffer;
                VKE_RENDER_SYSTEM_DEBUG_NAME;
            };

            struct VKE_API SRenderTargetDesc
            {
                TextureViewHandle            hTextureView = INVALID_HANDLE;
                RHI::TextureView             hRHIView     = RHI::Null;
                TEXTURE_STATE                beginState   = TextureStates::UNDEFINED;
                TEXTURE_STATE                endState     = TextureStates::UNDEFINED;
                RENDER_TARGET_RENDER_PASS_OP usage        = RenderTargetRenderPassOperations::UNDEFINED;
                SClearValue                  ClearValue   = { { 0, 0, 0, 1 } };
                TEXTURE_FORMAT               format       = Formats::UNDEFINED;
                SAMPLE_COUNT                 sampleCount  = SampleCounts::SAMPLE_1;

                VKE_RENDER_SYSTEM_DEBUG_NAME;
            };

            using SubpassDescArray      = Utils::TCDynamicArray< SSubpassDesc, 8 >;
            using AttachmentDescArray   = Utils::TCDynamicArray< SRenderTargetDesc, 8 >;
            using RenderTargetDescArray = Utils::TCDynamicArray< RenderSystem::SRenderTargetDesc, 8 >;

            struct SRenderPassDesc()
            {
            }

            struct SRenderPassDesc( DEFAULT_CTOR_INIT ) : Size( 800, 600 )

            {
            }

            AttachmentDescArray   vRenderTargets;
            RenderTargetDescArray vRenderTargetDescs;
            SubpassDescArray      vSubpasses;
            ExtentI32             PositionOffset = { 0, 0 };
            TextureSize           Size;
            ResourceName          Name;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        using SRenderPassAttachmentDesc = SRenderPassDesc::SRenderTargetDesc;
        using SSubpassAttachmentDesc    = SRenderPassDesc::SSubpassDesc::SRenderTargetDesc;

        using RenderTargetID = SResourceID< cstr_t, RenderTargetHandle, void* >;
        using TextureID      = SResourceID< cstr_t, TextureHandle, void* >;
        using TextureViewID  = SResourceID< cstr_t, TextureViewHandle, void* >;
        using RenderPassID   = SResourceID< cstr_t, RenderPassHandle, void* >;

        struct SSetRenderTargetInfo
        {
            RenderTargetID               RenderTarget;
            SClearValue                  ClearColor;
            TEXTURE_STATE                state;
            RENDER_TARGET_RENDER_PASS_OP renderPassOp = RenderTargetRenderPassOperations::UNDEFINED;

            SSetRenderTargetInfo()
            {
            }

            explicit SSetRenderTargetInfo( RenderTargetHandle hRT ) :
                RenderTarget{ hRT, RES_ID_HANDLE }, ClearColor{ 0, 0, 0, 1 },
                state{ TextureStates::COLOR_RENDER_TARGET },
                renderPassOp{ RenderTargetRenderPassOperations::COLOR_CLEAR_STORE }
            {
            }

            explicit SSetRenderTargetInfo( RenderTargetHandle hRT, const SClearValue& Clear ) :
                RenderTarget{ hRT, RES_ID_HANDLE }, ClearColor{ Clear }, state{ TextureStates::COLOR_RENDER_TARGET },
                renderPassOp{ RenderTargetRenderPassOperations::COLOR_CLEAR_STORE }
            {
            }

            explicit SSetRenderTargetInfo( RenderTargetHandle hRT, const SClearValue& Clear, TEXTURE_STATE state ) :
                RenderTarget{ hRT, RES_ID_HANDLE }, ClearColor{ Clear }, state{ state },
                renderPassOp{ RenderTargetRenderPassOperations::COLOR_CLEAR_STORE }
            {
            }
        };

        struct SRenderTargetInfo
        {
            RHI::TextureView             hDDIView = RHI::Null;
            FORMAT                       format   = Formats::UNDEFINED;
            SClearValue                  ClearColor;
            TEXTURE_STATE                state;
            RENDER_TARGET_RENDER_PASS_OP renderPassOp = RenderTargetRenderPassOperations::UNDEFINED;
        };

        struct VKE_API SSimpleRenderPassDesc
        {
            using RenderTargetArray = Utils::TCDynamicArray< SSetRenderTargetInfo, 8 >;
            RenderTargetArray vRenderTargets;
            Rect2DI32         RenderArea;
            ResourceName      Name;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct SRenderingPipelineDesc
        {
            struct SPassDesc
            {
                using Callback                = std::function< void( const SPassDesc& ) >;
                RenderPassHandle hPass        = INVALID_HANDLE;
                RenderPassHandle hWaitForPass = INVALID_HANDLE;
                Callback         OnRender     = {};
                bool             isAsync      = false;
                VKE_RENDER_SYSTEM_DEBUG_NAME;
            };

            using RenderPassArray = Utils::TCDynamicArray< SPassDesc >;

            RenderPassArray vRenderPassHandles;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct SBeginRenderPassInfo
        {
            RHI::RenderPass hDDIRenderPass;
            Rect2DI32       RenderArea;
        };

        using RenderTargetInfoArray = Utils::TCDynamicArray< SRenderTargetInfo, 8 >;

        struct SBeginRenderPassInfo2
        {
            RenderTargetInfoArray vColorRenderTargetInfos;
            SRenderTargetInfo     DepthRenderTargetInfo;
            SRenderTargetInfo     StencilRenderTargetInfo;
            Rect2DI32             RenderArea;
            uint32_t              renderTargetLayerCount = 1;
            uint32_t              renderTargetLayerIndex = 0;
            VKE_RENDER_SYSTEM_DEBUG_NAME;

            static vke_force_inline hash_t CalcHash( const SBeginRenderPassInfo2& Info )
            {
                Utils::SHash Hash;
                Hash.Combine( Info.DepthRenderTargetInfo.ClearColor.DepthStencil.depth,
                              Info.DepthRenderTargetInfo.ClearColor.DepthStencil.stencil,
                              Info.DepthRenderTargetInfo.hDDIView,
                              Info.DepthRenderTargetInfo.renderPassOp,
                              Info.DepthRenderTargetInfo.state,
                              Info.StencilRenderTargetInfo.ClearColor.DepthStencil.stencil,
                              Info.StencilRenderTargetInfo.hDDIView,
                              Info.StencilRenderTargetInfo.renderPassOp,
                              Info.StencilRenderTargetInfo.state,
                              Info.RenderArea.Position.x,
                              Info.RenderArea.Position.y,
                              Info.RenderArea.Size.width,
                              Info.RenderArea.Size.height,
                              Info.renderTargetLayerCount,
                              Info.renderTargetLayerIndex );

                for( uint32_t i = 0; i < Info.vColorRenderTargetInfos.GetCount(); ++i )
                {
                    const auto& RT = Info.vColorRenderTargetInfos[ i ];
                    Hash.Combine( RT.ClearColor.Color.r,
                                  RT.ClearColor.Color.g,
                                  RT.ClearColor.Color.g,
                                  RT.ClearColor.Color.a,
                                  RT.hDDIView,
                                  RT.renderPassOp,
                                  RT.state );
                }
                return Hash.value;
            }
        };

        struct SBindRenderPassInfo
        {
            RHI::CommandBuffer          hDDICommandBuffer;
            const SBeginRenderPassInfo* pBeginInfo;
        };

        namespace EventListeners
        {
            struct IGraphicsContext
            {
                virtual ~IGraphicsContext()
                {
                }

                virtual bool OnRenderFrame( CGraphicsContext* )
                {
                    return true;
                }

                virtual void OnAfterPresent( CGraphicsContext* )
                {
                }

                virtual void OnBeforePresent( CGraphicsContext* )
                {
                }

                virtual void OnBeforeExecute( CGraphicsContext* )
                {
                }

                virtual void OnAfterExecute( CGraphicsContext* )
                {
                }

                virtual bool AutoDestroy()
                {
                    return true;
                }
            };
        } // namespace EventListeners

        struct DeviceMemoryResizePolicies
        {
            enum POLICY
            {
                DEFAULT_SIZE,
                TWO_TIMES_DEFAULT_SIZE,
                FOUR_TIMES_DEFAULT_SIZE,
                TWO_TIMES_LAST_SIZE,
                FOUR_TIMES_LAST_SIZE,
                _MAX_COUNT
            };
        };

        using DEVICE_MEMORY_RESIZE_POLICY = DeviceMemoryResizePolicies::POLICY;

        struct SDeviceMemoryManagerDesc
        {
            SMemoryPoolManagerDesc      MemoryPoolDesc;
            uint32_t                    defaultPoolSize = VKE_MEGABYTES( 16 );
            DEVICE_MEMORY_RESIZE_POLICY resizePolicy    = DeviceMemoryResizePolicies::TWO_TIMES_LAST_SIZE;
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
                UNKNOWN                           = 0x0,
                INDIRECT_BUFFER_READ              = VKE_BIT( 1 ),
                INDEX_READ                        = VKE_BIT( 2 ),
                VERTEX_ATTRIBUTE_READ             = VKE_BIT( 3 ),
                VS_UNIFORM_READ                   = VKE_BIT( 4 ),
                PS_UNIFORM_READ                   = VKE_BIT( 5 ),
                GS_UNIFORM_READ                   = VKE_BIT( 6 ),
                TS_UNIFORM_READ                   = VKE_BIT( 7 ),
                CS_UNIFORM_READ                   = VKE_BIT( 8 ),
                MS_UNIFORM_READ                   = VKE_BIT( 9 ),
                RT_UNIFORM_READ                   = VKE_BIT( 10 ),
                INPUT_ATTACHMENT_READ             = VKE_BIT( 11 ),
                VS_SHADER_READ                    = VKE_BIT( 12 ),
                PS_SHADER_READ                    = VKE_BIT( 13 ),
                GS_SHADER_READ                    = VKE_BIT( 14 ),
                TS_SHADER_READ                    = VKE_BIT( 15 ),
                CS_SHADER_READ                    = VKE_BIT( 16 ),
                MS_SHADER_READ                    = VKE_BIT( 17 ),
                RS_SHADER_READ                    = VKE_BIT( 18 ),
                VS_SHADER_WRITE                   = VKE_BIT( 19 ),
                PS_SHADER_WRITE                   = VKE_BIT( 20 ),
                GS_SHADER_WRITE                   = VKE_BIT( 21 ),
                TS_SHADER_WRITE                   = VKE_BIT( 22 ),
                CS_SHADER_WRITE                   = VKE_BIT( 23 ),
                MS_SHADER_WRITE                   = VKE_BIT( 24 ),
                RS_SHADER_WRITE                   = VKE_BIT( 25 ),
                COLOR_RENDER_TARGET_READ          = VKE_BIT( 26 ),
                COLOR_RENDER_TARGET_WRITE         = VKE_BIT( 27 ),
                DEPTH_STENCIL_RENDER_TARGET_READ  = VKE_BIT( 28 ),
                DEPTH_STENCIL_RENDER_TARGET_WRITE = VKE_BIT( 29 ),
                DATA_TRANSFER_READ                = VKE_BIT( 30 ),
                DATA_TRANSFER_WRITE               = VKE_BIT( 31 ),
                CPU_MEMORY_READ                   = VKE_BIT( 32 ),
                CPU_MEMORY_WRITE                  = VKE_BIT( 33 ),
                GPU_MEMORY_READ                   = VKE_BIT( 34 ),
                GPU_MEMORY_WRITE                  = VKE_BIT( 35 ),
                _MAX_COUNT                        = 36,
                UNIFORM_READ                      = VS_UNIFORM_READ | PS_UNIFORM_READ,
                SHADER_READ                       = VS_SHADER_READ | PS_SHADER_READ,
                SHADER_WRITE                      = VS_SHADER_WRITE | PS_SHADER_WRITE
            };
        };

        using MEMORY_ACCESS_TYPE = uint64_t;

        using BufferStates = MemoryAccessTypes;
        using BUFFER_STATE = MEMORY_ACCESS_TYPE;

        struct SResourceManagerDesc
        {
            SDeviceMemoryManagerDesc DeviceMemoryDesc;
            uint32_t                 aMemorySizes[ MemoryAccessTypes::_MAX_COUNT ] = { 0 };
        };

        struct ShaderCompilationStages
        {
            enum STATE : uint8_t
            {
                UNKNOWN,
                HIGH_LEVEL_TEXT,    // e.g. hlsl/glsl text format
                COMPILED_IR_TEXT,   // e.g. spirv text format
                COMPILED_IR_BINARY, // e.g. spv/dxbc binary format
                _MAX_COUNT
            };
        };

        using SHADER_COMPILATION_STAGE = ShaderCompilationStages::STATE;

        struct SShaderData
        {
            uint32_t                 codeSize;
            SHADER_TYPE              type = ShaderTypes::_MAX_COUNT;
            SHADER_COMPILATION_STAGE stage;
            const uint8_t*           pCode;
        };

        struct SShaderDefine
        {
            // using String = Utils::TCString< char, Config::RenderSystem::Shader::MAX_ENTRY_POINT_NAME_LENGTH >;
            using String = ShaderCompilerString;
            String Name;
            String Value;
        };

        struct SShaderDesc
        {
            template< typename T >
            using NameString = Utils::TCString< T, Config::RenderSystem::Shader::MAX_ENTRY_POINT_NAME_LENGTH >;

            using NameWString = NameString< wchar_t >;
            using NameCString = NameString< char >;
            // using NameString = Utils::TCString< char, Config::RenderSystem::Shader::MAX_ENTRY_POINT_NAME_LENGTH >;
            using IncludeString = Utils::TCString< char, Config::RenderSystem::Shader::MAX_INCLUDE_PATH_LENGTH >;
            using PreprocessorString =
                Utils::TCString< char, Config::RenderSystem::Shader::MAX_PREPROCESSOR_DIRECTIVE_LENGTH >;

            using IncStringArray  = Utils::TCDynamicArray< IncludeString >;
            using PrepStringArray = Utils::TCDynamicArray< PreprocessorString >;

            using DefineArray = Utils::TCDynamicArray< SShaderDefine >;

            Core::SFileInfo FileInfo;
            SHADER_TYPE     type       = ShaderTypes::UNKNOWN;
            SHADER_PROFILE  profile    = ShaderProfiles::UNKNOWN;
            NameCString     EntryPoint = NameCString( "main" );
            NameCString     Name       = "Unknown";
            IncStringArray  vIncludes;
            PrepStringArray vPreprocessor;
            DefineArray     vDefines;
            SShaderData*    pData = nullptr; // optional parameter if an application wants to use its own binaries
            bool            useShaderCache = true;

            // SShaderDesc() = default;

            // SShaderDesc(const SShaderDesc& Other)
            //{
            //     this->operator=( Other );
            // }

            // SShaderDesc(SShaderDesc&& Other)
            //{
            //     this->operator=( std::move( Other ) );
            // }

            // SShaderDesc& operator=(const SShaderDesc& Other)
            //{
            //     FileInfo = Other.FileInfo;
            //     type = Other.type;
            //     //SetEntryPoint( Other.pEntryPoint );
            //     EntryPoint = Other.EntryPoint;
            //     vIncludes = Other.vIncludes;
            //     vPreprocessor = Other.vPreprocessor;
            //     pData = Other.pData;
            //     return *this;
            // }

            // SShaderDesc& operator=(SShaderDesc&& Other)
            //{
            //     FileInfo = Other.FileInfo;
            //     type = Other.type;
            //     //SetEntryPoint( Other.pEntryPoint );
            //     EntryPoint = Other.EntryPoint;
            //     vIncludes = std::move(Other.vIncludes);
            //     vPreprocessor = std::move(Other.vPreprocessor);
            //     pData = std::move( Other.pData );
            //     return *this;
            // }

            /* void vke_force_inline SetEntryPoint( cstr_t pName )
             {
                 if (std::is_same< cstr_t, cstr_t >::value)
                 {
                     VKE_ASSERT2(strlen( (const char*)pName) < sizeof(pEntryPoint), "");
                     vke_strcpy( (char*)pEntryPoint, sizeof(pEntryPoint), (cstr_t)pName);
                 }
                 else if (std::is_same< cwstr_t, ShaderCompilerStrType >::value)
                 {
                     VKE_ASSERT2(wcslen(pName) < sizeof(pEntryPoint), "");
                     vke_wstrcpy( (wchar_t*)pEntryPoint, sizeof(pEntryPoint), (cwstr_t)pName);
                 }
             }*/
        };

        struct SCompileShaderInfo
        {
            cstr_t             pBuffer       = nullptr;
            void*              pCompilerData = nullptr;
            const SShaderDesc* pDesc         = nullptr;
            uint32_t           bufferSize    = 0;
            uint8_t            tid           = 0;
        };

        struct SCompileShaderData
        {
#if VKE_USE_GLSL_COMPILER
            using BinaryElement = uint32_t;
#elif VKE_USE_DIRECTX_SHADER_COMPILER
            using BinaryElement = uint8_t;
#endif
            using ShaderBinaryData = vke_vector< BinaryElement >;
            ShaderBinaryData vShaderBinary;
            uint32_t         codeByteSize;
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
            STENCIL_FUNCTION failFunc      = StencilFunctions::KEEP;
            STENCIL_FUNCTION passFunc      = StencilFunctions::KEEP;
            STENCIL_FUNCTION depthFailFunc = StencilFunctions::KEEP;
            COMPARE_FUNCTION compareFunc   = CompareFunctions::ALWAYS;
            uint32_t         compareMask   = 255;
            uint32_t         writeMask     = 255;
            uint32_t         reference     = 0;
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
                RED   = 1,
                GREEN = 2,
                BLUE  = 4,
                ALPHA = 8,
                ALL   = ( RED | GREEN | BLUE | ALPHA )
            };
        };

        using ColorComponent = uint8_t;

        struct SBlendState
        {
            struct SBlendFactors
            {
                BLEND_FACTOR    src       = BlendFactors::ONE;
                BLEND_FACTOR    dst       = BlendFactors::ZERO;
                BLEND_OPERATION operation = BlendOperations::ADD;
            };

            SBlendFactors  Color;
            SBlendFactors  Alpha;
            bool           enable    = false;
            ColorComponent writeMask = ColorComponents::ALL;
        };

        struct SPipelineManagerDesc
        {
            uint32_t maxPipelineCount       = Config::RenderSystem::Pipeline::MAX_PIPELINE_COUNT;
            uint32_t maxPipelineLayoutCount = Config::RenderSystem::Pipeline::MAX_PIPELINE_LAYOUT_COUNT;
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
                LINE_LIST_WITH_ADJACENCY,
                LINE_STRIP_WITH_ADJACENCY,
                TRIANGLE_LIST_WITH_ADJACENCY,
                TRIANGLE_STRIP_WITH_ADJACENCY,
                PATCH_LIST,

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

        template< class T, uint32_t TASK_COUNT >
        struct TaskPoolHelper
        {
            using Pool = Utils::TSFreePool< T, uint32_t, TASK_COUNT >;

            static T* GetTask( Pool* pPool )
            {
                T*       pTask = nullptr;
                uint32_t idx;
                if( pPool->vFreeElements.PopBack( &idx ) )
                {
                    pTask = &pPool->vPool[ idx ];
                }
                else
                {
                    T Task;
                    idx   = pPool->vPool.PushBack( Task );
                    pTask = &pPool->vPool[ idx ];
                }
                return pTask;
            }
        };

        struct SPipelineLayoutDesc;

        struct SCreateShaderDesc
        {
            Core::SCreateResourceInfo Create;
            SShaderDesc               Shader;
        };

        struct TessellationDomainOrigins
        {
            enum ORIGIN : uint8_t
            {
                UPPER_LEFT,
                LOWER_LEFT,
                _MAX_COUNT
            };
        };

        using TESSELLATION_DOMAIN_ORIGIN = TessellationDomainOrigins::ORIGIN;

        struct VKE_API SCreateBindingDesc
        {
            friend class CContextBase;
            void AddBinding( const SDescriptorSetLayoutDesc::SBinding& Binding );
            void AddBinding( const SResourceBinding& Binding, const BufferPtr& pBuffer );
            void AddBinding( const STextureBinding& Binding );
            void AddBinding( const SSamplerBinding& Binding );
            void AddBinding( const SSamplerTextureBinding& Binding );
            void AddConstantBuffer( uint8_t index, PIPELINE_STAGES stages, uint16_t space = 0u );
            void AddBuffer( uint8_t index, PIPELINE_STAGES stages, const uint16_t& arrayElementCount,
                            uint16_t space = 0u );
            void AddDynamicConstantBuffer( uint8_t index, PIPELINE_STAGES stages, uint16_t space = 0u );
            void AddDynamicBuffer( uint8_t index, PIPELINE_STAGES stages, const uint16_t& arrayElementCount,
                                   uint16_t space = 0u );
            void AddTextures( uint8_t index, PIPELINE_STAGES stages, uint16_t count = 1u, uint16_t space = 0u );
            void AddSamplers( uint8_t index, PIPELINE_STAGES stages, uint16_t count = 1u, uint16_t space = 0u );

            void                     AddSamplerAndTexture( uint8_t index, PIPELINE_STAGES stages, uint16_t space = 0u );
            SDescriptorSetLayoutDesc LayoutDesc;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct SPipelineDesc
        {
            using FormatArray = Utils::TCDynamicArray< FORMAT, 8 >;

            struct SShaders
            {
                struct SShaderName
                {
                    ResourceName FileName;
                    ShortName    EntryPoint;
                };

                ~SShaders()
                {
                }

                ShaderPtr apShaders[ ShaderTypes::_MAX_COUNT ];
            };

            struct SBlending
            {
                using BlendStateArray =
                    Utils::TCDynamicArray< SBlendState, Config::RenderSystem::Pipeline::MAX_BLEND_STATE_COUNT >;
                BlendStateArray vBlendStates;
                LOGIC_OPERATION logicOperation = LogicOperations::NO_OPERATION;
                bool            enable         = false;
            };

            struct SViewport
            {
                using ViewportArray =
                    Utils::TCDynamicArray< SViewportDesc, Config::RenderSystem::Pipeline::MAX_VIEWPORT_COUNT >;
                using ScissorArray =
                    Utils::TCDynamicArray< SScissorDesc, Config::RenderSystem::Pipeline::MAX_SCISSOR_COUNT >;
                ViewportArray vViewports;
                ScissorArray  vScissors;
                bool          enable = true;
            };

            struct SRasterization
            {
                struct
                {
                    POLYGON_MODE mode      = PolygonModes::FILL;
                    CULL_MODE    cullMode  = CullModes::NONE;
                    FRONT_FACE   frontFace = FrontFaces::COUNTER_CLOCKWISE;
                } Polygon;

                struct
                {
                    float biasConstantFactor = 0.0f;
                    float biasClampFactor    = 0.0f;
                    float biasSlopeFactor    = 0.0f;
                    bool  enableClamp        = false;
                } Depth;
            };

            struct SMultisampling
            {
                SAMPLE_COUNT sampleCount = SampleCounts::SAMPLE_1;
                bool         enable      = false;
            };

            struct SDepthStencil
            {
                struct
                {
                    bool             enable      = true;
                    bool             test        = false;
                    bool             write       = false;
                    COMPARE_FUNCTION compareFunc = CompareFunctions::LESS_EQUAL;

                    struct
                    {
                        bool  enable = false;
                        float min    = 0.0f;
                        float max    = 0.0f;
                    } Bounds;
                } Depth;

                struct
                {
                    bool                  enable = false;
                    bool                  test   = false;
                    SStencilOperationDesc FrontFace;
                    SStencilOperationDesc BackFace;
                } Stencil;
            };

            struct SInputLayout
            {
                struct SVertexAttribute
                {
                    SVertexAttribute()
                    {
                    }

                    SVertexAttribute( DEFAULT_CTOR_INIT ) :
                        pName( "" ), format{ Formats::R32G32B32_SFLOAT }, vertexBufferBindingIndex{ 0 }, location{ 0 },
                        offset{ 0 }, stride{ 3 * 4 }, inputRate{ VertexInputRates::VERTEX }
                    {
                    }

                    SVertexAttribute( cstr_t name, const FORMAT& fmt, uint16_t bindingLocation ) :
                        pName{ name }, format{ fmt }, vertexBufferBindingIndex{ 0 }, location{ bindingLocation },
                        offset{ 0 }, stride{ CalcFormatSize( fmt ) }, inputRate{ VertexInputRates::VERTEX }
                    {
                    }

                    SVertexAttribute( cstr_t name, const FORMAT& fmt, const uint16_t& bufferBindingIdx,
                                      const uint16_t& bindingLocation, const uint16_t& off, const uint16_t& strd,
                                      const VERTEX_INPUT_RATE& rate ) :
                        pName{ name }, format{ fmt }, vertexBufferBindingIndex{ bufferBindingIdx },
                        location{ bindingLocation }, offset{ off }, stride{ strd }, inputRate{ rate }
                    {
                    }

                    cstr_t            pName;
                    FORMAT            format;
                    uint16_t          location;
                    uint16_t          vertexBufferBindingIndex;
                    uint16_t          offset = 0;
                    uint16_t          stride;
                    VERTEX_INPUT_RATE inputRate = VertexInputRates::VERTEX;
                };

                using SVertexAttributeArray =
                    Utils::TCDynamicArray< SVertexAttribute,
                                           Config::RenderSystem::Pipeline::MAX_VERTEX_ATTRIBUTE_COUNT >;

                SVertexAttributeArray vVertexAttributes;
                PRIMITIVE_TOPOLOGY    topology               = PrimitiveTopologies::TRIANGLE_LIST;
                bool                  enablePrimitiveRestart = false;
                bool                  enable                 = true;
            };

            struct STesselation
            {
                bool                       enable             = false;
                uint8_t                    patchControlPoints = 1;
                TESSELLATION_DOMAIN_ORIGIN domainOrigin       = TessellationDomainOrigins::UPPER_LEFT;
            };

            SPipelineDesc()
            {
            }

            ~SPipelineDesc()
            {
            }

            SPipelineDesc& operator=( const SPipelineDesc& ) = default;

            SShaders             Shaders;
            SBlending            Blending;
            SRasterization       Rasterization;
            SViewport            Viewport;
            SMultisampling       Multisampling;
            SDepthStencil        DepthStencil;
            SInputLayout         InputLayout;
            STesselation         Tesselation;
            FormatArray          vColorRenderTargetFormats;
            FORMAT               depthRenderTargetFormat   = Formats::UNDEFINED;
            FORMAT               stencilRenderTargetFormat = Formats::UNDEFINED;
            PipelineLayoutHandle hLayout                   = INVALID_HANDLE;
            RHI::PipelineLayout  hDDILayout                = RHI::Null;
            // RenderPassHandle          hRenderPass               = INVALID_HANDLE;
            RHI::RenderPass    hDDIRenderPass = RHI::Null;
            RHI::Pipeline      hDDIParent     = RHI::Null;
            PipelinePtr        pDefault;
            SCreateBindingDesc ResourceBindings;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct SPipelineCreateDesc
        {
            Core::SCreateResourceInfo Create;
            SPipelineDesc             Pipeline;
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
            enum BITS : uint32_t
            {
                UNDEFINED       = 0,
                TRANSFER_SRC    = VKE_BIT( 1 ),
                TRANSFER_DST    = VKE_BIT( 2 ),
                TEXEL_BUFFER    = VKE_BIT( 3 ),
                CONSTANT_BUFFER = VKE_BIT( 4 ),
                BUFFER          = VKE_BIT( 5 ),
                INDEX_BUFFER    = VKE_BIT( 6 ),
                VERTEX_BUFFER   = VKE_BIT( 7 ),
                INDIRECT_BUFFER = VKE_BIT( 8 ),
                UPLOAD          = TRANSFER_SRC
            };
        };

        using BUFFER_USAGE = BufferUsages::BITS;

        struct SBufferDesc
        {
            using BufferRegions = Utils::TCDynamicArray< SBufferRegion, 8 >;

            MEMORY_USAGE memoryUsage = MemoryUsages::DEFAULT;
            BUFFER_USAGE usage       = BufferUsages::UNDEFINED;
            INDEX_TYPE   indexType;
            /// <summary>
            /// if stagingBufferRegionCount > 0 then a separate staging buffer will be created
            /// with size = (SBufferDesc::size * stagingBufferRegionCount)
            /// </summary>
            uint32_t      stagingBufferRegionCount = 0;
            BufferRegions vRegions;
            SColor        optimizedClearValue = { 0.0, 0.0, 0.0, 0.0 };

            uint32_t CalcSize() const
            {
                uint32_t ret = 0;
                for( uint32_t i = 0; i < vRegions.GetCount(); ++i )
                {
                    ret += vRegions[ i ].elementCount * vRegions[ i ].elementSize;
                }
                VKE_ASSERT( ret > 0 );
                return ret;
            }

            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct VertexAttributeTypes
        {
            enum TYPE
            {
                UNDEFINED = Formats::UNDEFINED,
                FLOAT     = Formats::R32_SFLOAT,
                VECTOR2   = Formats::R32G32_SFLOAT,
                VECTOR3   = Formats::R32G32B32_SFLOAT,
                VECTOR4   = Formats::R32G32B32A32_SFLOAT,
                INT       = Formats::R32_SINT,
                UINT      = Formats::R32_UINT,
                POSITION3 = VECTOR3,
                POSITION4 = VECTOR4,
                POSITION  = POSITION3,
                TEXCOORD  = VECTOR2,
                NORMAL    = VECTOR3
            };
        };

        using VERTEX_ATTRIBUTE_TYPE = VertexAttributeTypes::TYPE;

        struct SVertexAttributeDesc
        {
            cstr_t                pName = "";
            VERTEX_ATTRIBUTE_TYPE type;
            uint8_t               vertexBufferBinding = 0;
        };

        struct SVertexInputLayoutDesc
        {
            using AttributeArray = Utils::TCDynamicArray< SVertexAttributeDesc, 8 >;
            AttributeArray     vAttributes;
            PRIMITIVE_TOPOLOGY topology               = PrimitiveTopologies::TRIANGLE_LIST;
            bool               enablePrimitiveRestart = false;
        };

        struct SVertexBufferDesc : SBufferDesc
        {
            SVertexInputLayoutDesc Layout;
        };

        struct SIndexBufferDesc
        {
            SBufferDesc BaseDesc;
            INDEX_TYPE  indexType;
        };

        struct SBufferViewDesc
        {
            BufferHandle hBuffer;
            size_t       offset;
            FORMAT       format;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        struct SCreateBufferDesc
        {
            Core::SCreateResourceInfo Create;
            SBufferDesc               Buffer;
        };

        struct SCreateVertexBufferDesc
        {
            Core::SCreateResourceInfo Create;
            SVertexBufferDesc         Buffer;
        };

        struct SSemaphoreDesc
        {
            VKE_RENDER_SYSTEM_DEBUG_NAME;
            RHI::FenceValue startValue = UNDEFINED_U64;
        };

        struct SFenceDesc
        {
            RHI::FenceValue startValue = UNDEFINED_U64;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
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
            handle_t hProcess;
            handle_t hWindow;
            uint32_t queueFamilyIndex;
        };

        struct SPresentSurfaceCaps
        {
            using Formats = Utils::TCDynamicArray< SPresentSurfaceFormat >;
            using Modes   = Utils::TCDynamicArray< PRESENT_MODE, 8 >;

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
            using DescSetLayoutArray    = Utils::TCDynamicArray< DescriptorSetLayoutHandle, MAX_COUNT >;
            using PushConstantArray     = Utils::TCDynamicArray< SPushConstantDesc, 4 >;

            SPipelineLayoutDesc()
            {
            }

            SPipelineLayoutDesc( DescriptorSetLayoutHandle hLayout )
            {
                vDescriptorSetLayouts.PushBack( hLayout );
            }

            VKE_RENDER_SYSTEM_DEBUG_NAME;
            DescSetLayoutArray vDescriptorSetLayouts;
            PushConstantArray  vPushConstants;
        };

        struct SDDISwapChain
        {
            using ImageArray       = Utils::TCDynamicArray< RHI::Texture, 3 >;
            using ImageViewArray   = Utils::TCDynamicArray< RHI::TextureView, 3 >;
            using FramebufferArray = Utils::TCDynamicArray< RHI::Framebuffer, 3 >;

            void*                 pInternalAllocator = nullptr;
            ImageArray            vImages;
            ImageViewArray        vImageViews;
            FramebufferArray      vFramebuffers;
            RHI::RenderPass       hDDIRenderPass = RHI::Null;
            RHI::PresentSurface   hSurface       = RHI::Null;
            RHI::SwapChain        hSwapChain     = RHI::Null;
            TextureSize           Size;
            SPresentSurfaceFormat Format;
            PRESENT_MODE          mode;
            SPresentSurfaceCaps   Caps;
        };

        struct SDDIGetBackBufferInfo
        {
            uint64_t        waitTimeout      = UINT64_MAX;
            RHI::FenceValue signalFenceValue = 0;
            RHI::Fence      hSignalFence     = RHI::Null;
            RHI::GPUFence   hSignalGPUFence  = RHI::Null;
            RHI::CPUFence   hSignalCPUFence  = RHI::Null;
            RHI::Queue      hQueue           = RHI::Null;
        };

        struct SDDILoadInfo
        {
            SAPIAppInfo AppInfo;
            bool        enableDebugMode = VKE_RENDER_SYSTEM_DEBUG;
        };

        struct SDriverInfo
        {
            FEATURE_LEVEL featureLevel;
        };

        using AdapterInfoArray = Utils::TCDynamicArray< RenderSystem::SAdapterInfo >;

        struct SSubmitInfo
        {
            const RHI::GPUFence*      pDDISignalSemaphores = nullptr;
            const RHI::GPUFence*      pDDIWaitSemaphores   = nullptr;
            RHI::Fence                hSignalFence         = RHI::Null;
            RHI::Fence                hWaitForFence        = RHI::Null;
            RHI::CommandBuffer const* pDDICommandBuffers   = nullptr;
            RHI::CPUFence             hDDIFence            = RHI::Null;
            RHI::Queue                hDDIQueue            = RHI::Null;
            RHI::FenceValue           signalFenceValue     = 0;
            RHI::FenceValue           waitForFenceValue    = 0;
            uint16_t                  signalSemaphoreCount = 0;
            uint16_t                  waitSemaphoreCount   = 0;
            uint16_t                  commandBufferCount   = 0;
        };

        struct SPresentInfo
        {
            RHI::SwapChain  hSwapChain        = RHI::Null;
            RHI::Fence      hSignalFence      = RHI::Null;
            RHI::FenceValue signalFenceValue  = 0;
            RHI::Fence      hWaitForFence     = RHI::Null;
            RHI::FenceValue waitForFenceValue = 0;
            uint32_t        presentImageIndex = UNDEFINED_U32;
        };

        struct SPresentData
        {
            using UintArray       = Utils::TCDynamicArray< uint32_t, 8 >;
            using FenceValueArray = Utils::TCDynamicArray< RHI::FenceValue, 8 >;
            using SemaphoreArray  = Utils::TCDynamicArray< RHI::GPUFence, 8 >;
            using FenceArray      = Utils::TCDynamicArray< RHI::Fence, 8 >;
            using SwapChainArray  = Utils::TCDynamicArray< RHI::SwapChain, 8 >;

            SwapChainArray vSwapchains;
            // SemaphoreArray   vWaitSemaphores;
            FenceArray      vWaitForFences;
            FenceValueArray vWaitForFenceValues;
            UintArray       vImageIndices;
            RHI::Queue      hQueue = RHI::Null;
        };

        struct MemoryHeapTypes
        {
            enum TYPE : uint8_t
            {
                CPU,
                GPU,
                UPLOAD,
                CPU_CACHED,
                CPU_COHERENT,
                OTHER,
                _MAX_COUNT
            };
        };

        using MEMORY_HEAP_TYPE = MemoryHeapTypes::TYPE;

        using STAGING_BUFFER_FLAGS = uint32_t;

        struct StagingBufferFlagBits
        {
            enum FLAGS : STAGING_BUFFER_FLAGS
            {
                OUT_OF_SPACE_DEFAULT        = 0,
                OUT_OF_SPACE_DO_NOTHING     = VKE_BIT( 1 ),
                OUT_OF_SPACE_ALLOCATE_NEW   = VKE_BIT( 2 ),
                OUT_OF_SPACE_FLUSH_AND_WAIT = VKE_BIT( 3 ),
                OUT_OF_SPACE_WAIT_FOR_FRAME = VKE_BIT( 4 ),
                _MAX_COUNT                  = 5
            };
        };

        using StagingBufferFlags = Utils::TCBitset< STAGING_BUFFER_FLAGS >;

        struct SUpdateMemoryInfo
        {
            union
            {
                RHI::Buffer  hBuffer = RHI::Null;
                RHI::Texture hTexture;
            };

            const void*        pData;
            uint32_t           dataSize;
            uint32_t           dstDataOffset = 0;
            handle_t           hMemory       = INVALID_HANDLE;
            StagingBufferFlags flags         = StagingBufferFlagBits::OUT_OF_SPACE_DEFAULT;
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
            uint32_t hUpdateInfo;
            uint32_t dstBufferOffset;
            uint32_t totalSize = 0; /// 0
            CBuffer* pDstBuffer;
            VKE_RENDER_SYSTEM_DEBUG_INFO;
        };

        struct SBindPipelineInfo
        {
            CCommandBuffer* pCmdBuffer;
            CPipeline*      pPipeline;
        };

        struct SBindVertexBufferInfo
        {
            RHI::CommandBuffer hDDICommandBuffer;
            RHI::Buffer        hDDIBuffer;
            RHI::Pipeline      hDDIPipeline;
            uint32_t           offset;
        };

        struct SBindIndexBufferInfo
        {
            CCommandBuffer* pCmdBuffer;
            CIndexBuffer*   pBuffer;
            size_t          offset;
        };

        struct SBindDDIDescriptorSetsInfo
        {
            RHI::CommandBuffer        hDDICommandBuffer;
            const RHI::DescriptorSet* aDDISetHandles;
            const uint32_t*           aDynamicOffsets = nullptr;
            RHI::PipelineLayout       hDDIPipelineLayout;
            uint16_t                  firstSet;
            uint16_t                  setCount;
            uint16_t                  dynamicOffsetCount = 0;
            PIPELINE_TYPE             pipelineType;
        };

        struct SDDISwapChainDesc
        {
            TextureSize    Size             = { 800, 600 };
            uint32_t       queueFamilyIndex = 0;
            COLOR_SPACE    colorSpace       = ColorSpaces::SRGB;
            TEXTURE_FORMAT format           = Formats::R8G8B8A8_UNORM;
            PRESENT_MODE   mode             = PresentModes::FIFO;
            uint16_t       elementCount     = Constants::OPTIMAL;
        };

        struct SAllocateCommandBufferInfo
        {
            RHI::CommandBufferPool hDDIPool;
            uint32_t               count;
            COMMAND_BUFFER_LEVEL   level;
        };

        struct SFreeCommandBufferInfo
        {
            RHI::CommandBufferPool hDDIPool;
            RHI::CommandBuffer*    pDDICommandBuffers;
            uint32_t               count;
        };

        struct SDescriptorPoolDesc
        {
            struct SSize
            {
                DESCRIPTOR_SET_TYPE type;
                uint32_t            count;
            };

            using SizeVec = Utils::TCDynamicArray< SSize >;

            uint32_t maxSetCount = Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT;
            SizeVec  vPoolSizes;
            VKE_RENDER_SYSTEM_DEBUG_NAME;

            bool IsValid() const
            {
                // DX12 spec distinguish these resource types
                Utils::TCBitset< uint8_t > TypeBits = (uint8_t)0;

                for( uint32_t i = 0; i < vPoolSizes.GetCount(); ++i )
                {
                    auto dspType = BindingTypeToPoolType( vPoolSizes[ i ].type );
                    TypeBits.SetBit( (uint8_t)dspType );
                }
                return TypeBits.CalcSetBitCount() == 1;
            }
        };

        struct ExecuteCommandBufferFlags
        {
            enum FLAGS
            {
                END                        = VKE_BIT( 0 ),
                EXECUTE                    = VKE_BIT( 1 ),
                WAIT                       = VKE_BIT( 2 ),
                DONT_SIGNAL_SEMAPHORE      = VKE_BIT( 3 ),
                DONT_WAIT_FOR_SEMAPHORE    = VKE_BIT( 4 ),
                DONT_PUSH_SIGNAL_SEMAPHORE = VKE_BIT( 5 ),
                SIGNAL_GPU_FENCE           = VKE_BIT( 6 ),
                WAIT_FOR_GPU_FENCE         = VKE_BIT( 7 ),
                _MAX_COUNT                 = 8
            };
        };

        using EXECUTE_COMMAND_BUFFER_FLAGS = uint32_t;

        struct STransferContextDesc
        {
            // SCommandBufferPoolDesc  CmdBufferPoolDesc;
            void* pPrivate = nullptr;
        };

        struct SGraphicsContextDesc
        {
            SSwapChainDesc SwapChainDesc;
            // SCommandBufferPoolDesc      CmdBufferPoolDesc;
            SDescriptorPoolDesc DescriptorPoolDesc;
            void*               pPrivate = nullptr;
        };

        struct SDrawIndirectParams
        {
            uint32_t vertexCount;
            uint32_t instanceCount;
            uint32_t startVertex;
            uint32_t startInstance;
        };

        struct SDrawIndexedIndirectParams
        {
            uint32_t indexCount;
            uint32_t instanceCount;
            uint32_t startIndex;
            uint32_t vertexOffset;
            uint32_t startInstance;
        };

        struct SDrawMeshIndirectParams
        {
            uint32_t taskCount;
            uint32_t startTask;
        };

        struct SDrawParams
        {
            SDrawParams()
            {
            }

            union
            {
                struct
                {
                    uint32_t vertexCount;
                    uint32_t instanceCount;
                    uint32_t startVertex;
                    uint32_t startInstance;
                } Draw;

                struct
                {
                    uint32_t indexCount;
                    uint32_t instanceCount;
                    uint32_t startIndex;
                    uint32_t vertexOffset;
                    uint32_t startInstance;
                } Indexed;

                struct
                {
                    RHI::Buffer hArgumentBuffer{};
                    uint32_t    offset;
                    uint32_t    drawCount;
                    uint32_t    stride;
                } Indirect;

                struct
                {
                    RHI::Buffer hArgumentBuffer{};
                    RHI::Buffer hCountBuffer{};
                    uint32_t    offset;
                    uint32_t    countOffset;
                    uint32_t    maxCount;
                    uint32_t    stride;
                } IndirectCount;

                struct
                {
                    RHI::Buffer hArgumentBuffer{};
                    uint32_t    offset;
                    uint32_t    count;
                    uint32_t    stride;
                } IndexedIndirect;

                struct
                {
                    uint32_t count;
                    uint32_t first;
                } Mesh;

                struct
                {
                    RHI::Buffer hArgumentBuffer{};
                    uint32_t    offset;
                    uint32_t    count;
                    uint32_t    stride;
                } MeshIndirect;

                struct
                {
                    RHI::Buffer hArgumentBuffer{};
                    RHI::Buffer hCountBuffer{};
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

        struct FeatureEnableModes
        {
            enum MODE : uint8_t
            {
                DISABLE,
                ENABLE,
                OPTIONAL,
                _MAX_COUNT,
                DISABLED = DISABLE,
                ENABLED  = ENABLE
            };
        };

        using FEATURE_ENABLE_MODE = FeatureEnableModes::MODE;

        struct SStagingBufferInfo
        {
            handle_t    hMemory;
            RHI::Buffer hDDIBuffer;
            uint32_t    sizeLeft;
            uint32_t    alignedSize;
            uint32_t    offset;
        };

        struct SDeviceContextMetrics
        {
            float minFps         = 0.0f;
            float maxFps         = 0.0f;
            float avgFps         = 0.0f;
            float currentFps     = 0.0f;
            float minFrameTimeMs = 0;
            float maxFrameTimeMs = 0;
            float avgFrameTimeMs = 0;
        };

        struct SRenderSystemMetrics
        {
        };

        struct SDeviceFeatures
        {
            using Option                  = FEATURE_ENABLE_MODE;
            Option dynamicRenderPass      = FeatureEnableModes::ENABLE;
            Option meshShaders            = FeatureEnableModes::DISABLE;
            Option raytracing             = FeatureEnableModes::DISABLE;
            Option raytracingMotionBlur   = FeatureEnableModes::DISABLE;
            Option bindlessResourceAccess = FeatureEnableModes::OPTIONAL;
        };

        struct SSettings
        {
            FEATURE_LEVEL   featureLevel = FeatureLevels::LEVEL_DEFAULT;
            SDeviceFeatures Features;
        };

        struct SCreateDeviceDesc
        {
            SSettings Settings;
        };

        struct SDeviceContextDesc
        {
            SCreateDeviceDesc   DeviceDesc;
            const SAdapterInfo* pAdapterInfo            = nullptr;
            const void*         pPrivate                = nullptr;
            DescriptorSetCounts aMaxDescriptorSetCounts = { 0 };
        };

        struct STextureFormatFeatures
        {
            uint32_t sampled : 1;
            uint32_t storage : 1;
            uint32_t storageAtomic : 1;
            uint32_t uniformTexelBuffer : 1;
            uint32_t storageTexelBuffer : 1;
            uint32_t vertexBuffer : 1;
            uint32_t colorRenderTarget : 1;
            uint32_t colorRenderTargetBlend : 1;
            uint32_t depthStencilRenderTarget : 1;
            uint32_t storageTexelBufferAtomic : 1;
            uint32_t blitSrc : 1;
            uint32_t blitDst : 1;
            uint32_t linearFilter : 1;
            uint32_t transferSrc : 1;
            uint32_t transferDst : 1;
        };

        struct CommandBufferStates
        {
            enum STATE : uint8_t
            {
                UNKNOWN,
                RESET,
                BEGIN,
                END,
                FLUSH,
                EXECUTED,
                _MAX_COUNT
            };
        };

        using COMMAND_BUFFER_STATE = CommandBufferStates::STATE;

        struct SOffset3D
        {
            union
            {
                struct
                {
                    int32_t x;
                    int32_t y;
                    int32_t z;
                };

                int32_t xyz[ 3 ];
            };
        };

        struct SBlitTextureRegion
        {
            STextureSubresourceRange SrcSubresource;
            STextureSubresourceRange DstSubresource;
            SOffset3D                srcOffsets[ 2 ];
            SOffset3D                dstOffsets[ 2 ];
        };

        struct SBlitTextureInfo
        {
            using RegionArray = Utils::TCDynamicArray< SBlitTextureRegion >;
            RHI::Texture   hAPISrcTexture;
            RHI::Texture   hAPIDstTexture;
            TEXTURE_STATE  srcTextureState;
            TEXTURE_STATE  dstTextureState;
            TEXTURE_FILTER filter = TextureFilters::LINEAR;
            RegionArray    vRegions;
        };

        struct SFrameGraphNodeWorkload
        {
            CommandBufferPtr pCommandBuffer;
        };
        class CFrameGraphNode;
        using FrameGraphWorkload = std::function< Result( CFrameGraphNode* const, uint8_t ) >;

        struct RenderPassSizes
        {
            enum SIZE : uint8_t
            {
                _1_1  = 1,
                _1_2  = 2,
                _1_4  = 4,
                _1_8  = 8,
                _1_16 = 16,
                _1_32 = 32,
                _MAX_COUNT,
                DEFAULT = _1_1,
                UNKNOWN = _1_1
            };
        };

        using RENDER_PASS_SIZE = RenderPassSizes::SIZE;

        struct FrameGraphPassOperations
        {
            enum OP
            {
                SHADER_READ,
                RENDER_PASS_WRITE,
                RENDER_PASS_OVERWRITE,
                SHADER_READ_WRITE,
                RENDER_PASS_READ,
                _MAX_COUNT,
                UNKNOWN = _MAX_COUNT
            };
        };

        using RENDER_PASS_OP = FrameGraphPassOperations::OP;

        struct SFrameGraphRenderTargetTextureDesc
        {
            cstr_t           pName          = nullptr;
            TEXTURE_FORMAT   format         = Formats::UNDEFINED;
            RENDER_PASS_OP   operation      = FrameGraphPassOperations::UNKNOWN;
            RENDER_PASS_SIZE size           = RenderPassSizes::DEFAULT;
            RHI::Texture     hNativeTexture = RHI::Null;
            cstr_t           pOldName       = nullptr;
            SClearValue      clearValue     = { 0.0, 0.0, 0.0, 0.0 };
        };

        struct FrameGraphNodeFlags
        {
            enum FLAG : uint64_t
            {
                NONE = 0,
                START_FRAME             = VKE_BIT( 0 ),
                END_FRAME               = VKE_BIT( 1 ),
                UPLOAD                  = VKE_BIT( 2 ),
                RENDER_OPAQUE           = VKE_BIT( 3 ),
                RENDER_TRANSPARENT      = VKE_BIT( 4 ),
                RENDER_DEPTH            = VKE_BIT( 5 ),
                RENDER_UPDATE           = VKE_BIT( 6 ),
                RESOURCE_UPDATE         = VKE_BIT( 7 ),
                COMPUTE_UPDATE          = VKE_BIT( 8 ),
                COPY_UPDATE             = VKE_BIT( 9 ),
                COMPUTE_POST_RENDER     = VKE_BIT( 10 ),
                COPY_POST_RENDER        = VKE_BIT( 11 ),
                COMPUTE_POSTPROCESS     = VKE_BIT( 12 ),
                RENDER_POSTPROCESS      = VKE_BIT( 13 ),
                COMPILE_DATA            = VKE_BIT( 14 ),
                GENERATE_DATA           = VKE_BIT( 15 ),
                EXECUTE_COMMAND_BUFFERS = VKE_BIT( 16 ),
                PROCESS_TEXTURES        = VKE_BIT( 17 ),
                PROCESS_BUFFERS         = VKE_BIT( 18 ),
                PROCESS_SCRIPTS         = VKE_BIT( 19 ),
                LOAD_FILES              = VKE_BIT( 20 ),
                SIGNAL_GPU_FENCE        = VKE_BIT( 21 ),
                SIGNAL_THREAD_FENCE     = VKE_BIT( 22 ),
                FRAME_UPDATE            = VKE_BIT( 23 ),
                _LAST_BIT               = 23
            };
        };

        using FrameGraphNodeFlagBits = Utils::TCBitset< FrameGraphNodeFlags::FLAG >;

        struct SFrameGraphNodeDesc
        {
            using TextureDescArray = Utils::TCDynamicArray< SFrameGraphRenderTargetTextureDesc, 8 >;

            cstr_t           pName;
            cstr_t           pExecute       = "Main";
            cstr_t           pThread        = "Main";
            cstr_t           pCommandBuffer = "Main";
            FrameGraphNodeFlagBits Flags          = FrameGraphNodeFlags::NONE;
            CONTEXT_TYPE     contextType    = ContextTypes::GENERAL;
            RENDER_PASS_SIZE size           = RenderPassSizes::_1_1;
            uint16_t         gpuFenceValue  = 0;
            TextureDescArray vRenderTargets;
        };

        using SFrameGraphPassDesc = SFrameGraphNodeDesc;

        struct FrameGraphNodeTypes
        {
            enum TYPE
            {
                UPLOAD,
                UPDATE,
                RENDER,
                _MAX_COUNT
            };
        };

        using FRAME_GRAPH_NODE_TYPE = FrameGraphNodeTypes::TYPE;

        struct SCreateCommandBufferInfo
        {
            uint16_t count;
            uint8_t  threadIndex;
        };

        struct SExecuteBatch
        {
            static const uint32_t DEFAULT_CMD_BUFFER_COUNT = 32;
            using ExecuteBatchArray                        = Utils::TCDynamicArray< SExecuteBatch*, 4 >;
            using SemaphoreArray                           = Utils::TCDynamicArray< RHI::GPUFence, 8 >;
            VKE_RENDER_SYSTEM_DEBUG_NAME;

            CContextBase*                                                      pContext        = nullptr;
            RHI::GPUFence                                                      hSignalGPUFence = RHI::Null;
            RHI::CPUFence                                                      hSignalCPUFence = RHI::Null;
            SemaphoreArray                                                     vDDIWaitGPUFences;
            Utils::TCDynamicArray< CCommandBuffer*, DEFAULT_CMD_BUFFER_COUNT > vpCommandBuffers;
            uint32_t                                                           swapchainElementIndex = INVALID_POSITION;
            VKE_DEBUG_CODE( uint32_t executionCount = 0; )
            VKE_DEBUG_CODE( uint32_t acquireCount = 0; )
            uint32_t                     refCount        = 0;
            Result                       executionResult = Results::NOT_READY;
            EXECUTE_COMMAND_BUFFER_FLAGS executeFlags    = 0;
            ExecuteBatchArray            vDependencies;

            void AddDependency( SExecuteBatch** ppBatch )
            {
                SExecuteBatch* pBatch = *ppBatch;
                vDDIWaitGPUFences.PushBackUnique( pBatch->hSignalGPUFence );
                vDependencies.PushBackUnique( pBatch );
                pBatch->executeFlags |= ExecuteCommandBufferFlags::SIGNAL_GPU_FENCE;
                VKE_ASSERT( pBatch->executionResult == Results::NOT_READY );
            }
        };

        struct SAllocateMemoryData
        {
            RHI::MemoryHeap  hDDIMemory = RHI::Null;
            uint32_t         sizeLeft;
            MEMORY_HEAP_TYPE heapType;
        };

        struct SBindMemoryInfo
        {
            RHI::MemoryHeap hDDIMemory = RHI::Null;
            handle_t        hMemory    = INVALID_HANDLE;
            handle_t        reserved   = INVALID_HANDLE;
            uint32_t        offset     = 0;
        };

        struct SMapMemoryInfo
        {
            union
            {
                RHI::Buffer  hBuffer = RHI::Null;
                RHI::Texture hTexture;
            };

            RHI::MemoryHeap hMemory;
            uint32_t        offset;
            uint32_t        size;
        };

        struct SUnmapMemoryInfo
        {
            union
            {
                RHI::Buffer  hBuffer = RHI::Null;
                RHI::Texture hTexture;
            };

            RHI::MemoryHeap hMemory;
        };

        struct SAllocateMemoryDesc
        {
            uint32_t     size;
            MEMORY_USAGE usage;
        };

        struct SAllocationMemoryRequirementInfo
        {
            uint32_t size;
            uint32_t alignment;
            handle_t reserved;
            /// <summary>
            /// If out of memory then new pool could be created, therefore new size is required.
            /// 0 value is default and memory manager will choose appropriate size.
            /// </summary>
            uint32_t poolSize = 0;
            /// <summary>
            /// Declares and initializes a variable to track memory usage.
            /// 0 is invalid value. At least texture or buffer usage must be specified.
            /// </summary>
            MEMORY_USAGE memoryUsages = 0;
        };

        struct SResourceBindingInfo
        {
            uint8_t  index;
            uint16_t count;
            uint32_t offset;
            uint32_t range;
        };

        struct SMemoryBarrierInfo
        {
            MEMORY_ACCESS_TYPE srcMemoryAccess;
            MEMORY_ACCESS_TYPE dstMemoryAccess;
        };

        struct STextureBarrierInfo : SMemoryBarrierInfo
        {
            RHI::Texture             hDDITexture;
            TEXTURE_STATE            currentState;
            TEXTURE_STATE            newState;
            STextureSubresourceRange SubresourceRange;
        };

        struct SBufferBarrierInfo : SMemoryBarrierInfo
        {
            RHI::Buffer hDDIBuffer;
            uint32_t    size;
            uint32_t    offset;
        };

        struct SBarrierInfo
        {
            static const uint16_t MAX_BARRIER_COUNT = 16;

            using MemoryBarrierArray  = Utils::TCDynamicArray< SMemoryBarrierInfo, MAX_BARRIER_COUNT >;
            using TextureBarrierArray = Utils::TCDynamicArray< STextureBarrierInfo, MAX_BARRIER_COUNT >;
            using BufferBarrierArray  = Utils::TCDynamicArray< SBufferBarrierInfo, MAX_BARRIER_COUNT >;

            MemoryBarrierArray  vMemoryBarriers;
            TextureBarrierArray vTextureBarriers;
            BufferBarrierArray  vBufferBarriers;
        };

        struct SCopyBufferInfo
        {
            struct SRegion
            {
                uint32_t srcBufferOffset = UINT32_MAX;
                uint32_t dstBufferOffset = UINT32_MAX;
                uint32_t size;
            };

            using RegionArray = Utils::TCDynamicArray< SRegion >;

            RHI::Buffer hDDISrcBuffer;
            // RHI::Buffer           hDDIDstBuffer;
            BufferPtr pDstBuffer;
            SRegion   Region;
        };

        struct SCopyTextureInfo
        {
            RHI::Texture hDDISrcTexture;
            RHI::Texture hDDIDstTexture;
            TextureSize  Size;
            uint16_t     depth;
            TextureSize  SrcOffset;
            TextureSize  DstOffset;
        };

        struct SCopyTextureInfoEx
        {
            SCopyTextureInfo*        pBaseInfo;
            TEXTURE_STATE            srcTextureState;
            TEXTURE_STATE            dstTextureState;
            STextureSubresourceRange SrcSubresource;
            STextureSubresourceRange DstSubresource;
        };

        struct SBufferTextureRegion
        {
            uint32_t bufferOffset;
            uint32_t bufferRowLength;
            uint32_t bufferTextureHeight;

            STextureSubresourceRange TextureSubresource;

            uint32_t textureOffsetX;
            uint32_t textureOffsetY;
            uint32_t textureOffsetZ;
            uint32_t textureWidth;
            uint32_t textureHeight;
            uint32_t textureDepth;
        };

        struct SCopyBufferToTextureInfo
        {
            using RegionArray = Utils::TCDynamicArray< SBufferTextureRegion >;

            RHI::Buffer   hDDISrcBuffer;
            RHI::Texture  hDDIDstTexture;
            TEXTURE_STATE textureState;
            RegionArray   vRegions;
        };

        struct SDeviceInfo
        {
            struct
            {
                struct
                {
                    uint32_t constantBufferOffset;
                    uint32_t texelBufferOffset;
                    uint32_t storageBufferOffset;
                    uint32_t bufferCopyOffset;
                    uint32_t bufferCopyRowPitch;
                    uint32_t memoryMap;
                } Alignment;

                struct
                {
                    uint32_t maxAllocationCount;
                    uint32_t minMapAlignment;
                    uint32_t minTexelBufferOffsetAlignment;
                    uint32_t minConstantBufferOffsetAlignment;
                    uint32_t minStorageBufferOffsetAlignment;
                } MemoryProperties;

                struct
                {
                    int32_t  minTexel;
                    uint32_t maxTexel;

                } Offset;

                struct
                {
                    uint32_t maxDrawIndirect;
                    uint32_t maxRenderTarget;
                } Count;

                struct
                {
                    uint32_t maxRenderTargetWidth;
                    uint32_t maxRenderTargetHeight;
                    uint32_t maxTexture1DDimmension;
                    uint32_t maxTexture2DDimmension;
                    uint32_t maxTexture3DDimmension;
                    uint32_t maxTextureCubeDimmension;
                } Texture;

                struct
                {

                } Buffer;

                struct
                {
                    struct
                    {
                        uint32_t maxResourceCount;
                        uint32_t maxStorageTextureCount;
                        uint32_t maxTextureCount;
                        uint32_t maxStorageBufferCount;
                        uint32_t maxConstantBufferCount;
                        uint32_t maxSamplerCount;
                    } Stage;

                    uint32_t maxConstantBufferRange;
                    uint32_t maxPushConstantsSize;
                } Binding;

                struct
                {
                    uint32_t maxColorRenderTargetCount;
                } RenderPass;

                struct
                {
                    float timestampPeriod;
                } Query;

                uint32_t maxDrawIndexedIndexValue;
                uint32_t maxClipDistance;

            } Limits;

            SDeviceFeatures Features;
        };

        using QueuePriorityArray       = Utils::TCDynamicArray< float, 16 >;
        using QueueFamilyPropertyArray = Utils::TCDynamicArray< RHI::QueueFamilyProperties, 16 >;
        using UintArray                = Utils::TCDynamicArray< uint32_t, 16 >;
        using QueueTypeArray           = UintArray[ QueueTypes::_MAX_COUNT ];
        using DDIQueueArray            = Utils::TCDynamicArray< RHI::Queue >;

        struct SUpdateBufferDescriptorSetInfo
        {
            struct SBufferInfo
            {
                RHI::Buffer     hDDIBuffer;
                RHI::DeviceSize offset;
                RHI::DeviceSize range;
            };

            using BufferInfoArray = Utils::TCDynamicArray< SBufferInfo, 4 >;

            uint32_t           binding;
            uint32_t           count;
            RHI::DescriptorSet hDDISet;
            BufferInfoArray    vBufferInfos;
        };

        struct SUpdateTextureDescriptorSetInfo
        {
            struct STextureInfo
            {
                RHI::Sampler     hDDISampler;
                RHI::TextureView hDDITextureView;
                TEXTURE_STATE    textureState;
            };

            using TextureInfoArray = Utils::TCDynamicArray< STextureInfo, 8 >;

            TextureInfoArray   vTextureInfos;
            RHI::DescriptorSet hDDISet;
            uint8_t            binding;
            uint16_t           count;
        };

        struct SUpdateBindingInfo
        {
            struct SSamplerTextureInfo
            {
                RHI::Sampler     hDDISampler     = RHI::Null;
                RHI::TextureView hDDITextureView = RHI::Null;
                TEXTURE_STATE    textureState;
            };

            struct SBufferInfo
            {
                RHI::Buffer     hDDIBuffer;
                RHI::DeviceSize offset;
                RHI::DeviceSize range;
            };

            using BufferInfoArray  = Utils::TCDynamicArray< SBufferInfo, 8 >;
            using TextureInfoArray = Utils::TCDynamicArray< SSamplerTextureInfo, 8 >;

            RHI::DescriptorSet  hDDISet;
            DESCRIPTOR_SET_TYPE type;
            uint8_t             binding;
            BufferInfoArray     vBuffers;
            TextureInfoArray    vTextures;
        };

        struct QueueCaps
        {
            enum CAPABILITY
            {
                GRAPHICS = VKE_BIT(0),
                COMPUTE = VKE_BIT(1),
                TRANSFER = VKE_BIT(2),
                SPARSE = VKE_BIT(3),
                PRESENT = VKE_BIT(4),
                VIDEO_ENCODE = VKE_BIT(5),
                VIDEO_DECODE = VKE_BIT(6),
                TIMESTMAP = VKE_BIT(7),
            };
        };
        using QueueCapsBits = Utils::TCBitset< QueueCaps::CAPABILITY >;

        struct SQueueFamilyInfo
        {
            DDIQueueArray      vQueues;
            QueuePriorityArray vPriorities;
            uint32_t           index;
            /// <summary>
            /// Main queue type
            /// </summary>
            QUEUE_TYPE         type;
            /// <summary>
            /// Indicates all capabilities of the family
            /// </summary>
            QueueCapsBits      Caps;
        };

        using QueueFamilyInfoArray = Utils::TCDynamicArray< SQueueFamilyInfo >;

        struct SAdapterProperties
        {
            QueueFamilyInfoArray vQueueInfos;
        };

        struct SDeviceProperties
        {
            /*struct SQueueFamilyProperties
            {
                uint32_t queueCount;
                uint32_t queueFlags;
            };
            using QueueFamilyPropertyArray = Utils::TCDynamicArray< SQueueFamilyProperties, 16 >;
            QueueFamilyPropertyArray vQueueFamilyProperties;*/
            QueueFamilyInfoArray vQueueFamilies;
        }; // struct SDeviceProperties

        struct VKE_API SDDIDrawInfo
        {
            RHI::CommandBuffer hCommandBuffer;

            uint32_t vertexCount;
            uint32_t instanceCount;
            uint32_t firstVertex;
            uint32_t firstInstance;
        };

        struct AllocateDescs
        {
            struct SDescSet
            {
                RHI::DescriptorPool       hPool;
                RHI::DescriptorSetLayout* phLayouts;
                uint32_t                  count;
                VKE_RENDER_SYSTEM_DEBUG_NAME;
            };

            struct SMemory
            {
                RHI::Texture hDDITexture = RHI::Null;
                RHI::Buffer  hDDIBuffer  = RHI::Null;
                uint32_t     size;
                MEMORY_USAGE memoryUsages;
            };
        };

        struct FreeDescs
        {
            struct SDescSet
            {
                RHI::DescriptorPool hPool;
                RHI::DescriptorSet* phSets;
                uint32_t            count;
            };

            struct SCommandBuffers
            {
                RHI::CommandBufferPool hPool;
                RHI::CommandBuffer*    pBuffers;
                uint32_t               count;
            };
        };

#define VKE_ADD_DDI_OBJECT( _type )                                                                                    \
protected:                                                                                                             \
    _type m_hDDIObject = RHI::Null;                                                                                    \
                                                                                                                       \
public:                                                                                                                \
    vke_force_inline const _type& GetDDIObject() const                                                                 \
    {                                                                                                                  \
        return m_hDDIObject;                                                                                           \
    }

    } // namespace RenderSystem

    using SRenderSystemDesc = RenderSystem::SRenderSystemDesc;

} // namespace VKE

#pragma pop_macro( "OPTIONAL" )

// #pragma pop_macro( "DOMAIN" )
