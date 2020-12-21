#include "Core/Managers/CImageManager.h"
#include "Core/Managers/CFileManager.h"
#include "Core/Resources/CImage.h"

#include "RenderSystem/CRenderSystem.h"

#if VKE_USE_DEVIL
#include "IL/il.h"
#include "IL/ilu.h"
#include "IL/ilut.h"
#endif

#if VKE_USE_DIRECTXTEX
#   include "ThirdParty/DirectXTex/DirectXTex/DirectXTex.h"
#endif

namespace VKE
{
    namespace Core
    {
        CImageManager::CImageManager()
        {

        }

        CImageManager::~CImageManager()
        {
            _Destroy();
        }

        Result CImageManager::_Create(const SImageManagerDesc& Desc)
        {
            Result ret = VKE_OK;
            VKE_ASSERT(Desc.pFileMgr, "");
            m_pFileMgr = Desc.pFileMgr;
            m_MemoryPool.Create( Desc.maxImageCount, sizeof(CImage), 1 );

#if VKE_USE_DEVIL
            ret = _InitDevIL();
#elif VKE_USE_DIRECTXTEX
            ret = _InitDirectXTex();
#endif
            return ret;
        }

        Result CImageManager::_InitDevIL()
        {
            Result ret = VKE_FAIL;
#if VKE_USE_DEVIL
            ilInit();
            iluInit();
            ret = VKE_OK;
#endif
            return ret;
        }

        Result CImageManager::_InitDirectXTex()
        {
            Result ret = VKE_FAIL;
#if VKE_USE_DIRECTXTEX
            ::HRESULT hr = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            if (hr != S_OK)
            {
                VKE_LOG_ERR("Unable to initialize COM library.");
                ret = VKE_FAIL;
            }
            else
            {
                ret = VKE_OK;
            }
#endif
            return ret;
        }

        void CImageManager::_Destroy()
        {
            for (uint32_t i = 0; i < m_Buffer.FreeResources.GetCount(); ++i)
            {
                //auto pImg = m_Buffer.FreeResources[i];
                //_DestroyImage( &pImg );
            }
            for( auto& Itr : m_Buffer.Resources.Container )
            {
                auto pImg = Itr.second;
                _DestroyImage( &pImg );
            }
            m_MemoryPool.Destroy();
        }

        void CImageManager::_DestroyImage(Core::CImage** ppImgInOut)
        {
            VKE_ASSERT( ppImgInOut != nullptr && *ppImgInOut != nullptr, "Invalid image" );
            CImage* pImg = *ppImgInOut;
            pImg->_Destroy();
            Memory::DestroyObject( &m_MemoryPool, &pImg );
            *ppImgInOut = nullptr;
        }

        void CImageManager::DestroyImage(ImageHandle* phImg)
        {
            CImage* pImg = GetImage(*phImg).Release();
            _FreeImage( pImg );
            *phImg = INVALID_HANDLE;
        }

        hash_t CalcImageHash(const SLoadFileInfo& Info)
        {
            hash_t ret = 0;
            const hash_t h1 = CResource::CalcHash( Info.FileInfo );

            ret = h1;
            return ret;
        }

        hash_t CalcImageHash(const SImageDesc& Desc)
        {
            Utils::SHash Hash;
            Hash.Combine(Desc.depth, Desc.format, Desc.Name.GetData(), Desc.Size.width, Desc.Size.height, Desc.type);
            return Hash.value;
        }

        Result CImageManager::_CreateImage(const hash_t& hash, CImage** ppOut)
        {
            Result ret = VKE_FAIL;
            CImage* pImage = nullptr;
            if(!m_Buffer.Find(hash, &pImage))
            {
                if (!m_Buffer.TryToReuse(0, &pImage))
                {
                    if (VKE_SUCCEEDED(Memory::CreateObject(&m_MemoryPool, &pImage, this)))
                    {
                        if (m_Buffer.Add(hash, pImage))
                        {
                            pImage->m_Handle.handle = hash;
                            ret = VKE_OK;
                        }
                        else
                        {
                            Memory::DestroyObject(&m_MemoryPool, &pImage);
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR("Unable to allocate memory for CImage object");
                        ret = VKE_ENOMEMORY;
                    }
                }
            }
            *ppOut = pImage;
            return ret;
        }

        ImageHandle CImageManager::Load(const SLoadFileInfo& Info)
        {
            // Check if such image is already loaded
            const hash_t hash = CalcImageHash( Info );
            CImage* pImage = nullptr;
            if( !m_Buffer.Find( hash, &pImage ) )
            {
                if( !m_Buffer.TryToReuse( INVALID_HANDLE, &pImage ) )
                {
                    if( VKE_FAILED( Memory::CreateObject( &m_MemoryPool, &pImage, this ) ) )
                    {
                        VKE_LOG_ERR("Unable to create memory for CImage object: " << Info.FileInfo.pFileName);
                    }
                }
                if( pImage != nullptr )
                {
                    if (!m_Buffer.Add(hash, pImage))
                    {
                        Memory::DestroyObject(&m_MemoryPool, &pImage);
                    }
                    {
                        FilePtr pFile = m_pFileMgr->LoadFile(Info);
                        if (pFile.IsValid())
                        {
                            if (VKE_SUCCEEDED(_CreateImage(pFile.Get(), &pImage)))
                            {
                                pImage->m_Handle.handle = hash;
                            }
                            else
                            {
                                pImage->Release();
                            }
                            pFile->Release();
                        }
                    }
                }
            }

            return pImage->GetHandle();
        }

        void CImageManager::_FreeImage(CImage* pImg)
        {
            pImg->_Destroy();
            m_Buffer.AddFree( pImg->GetHandle().handle, pImg );
        }

        vke_force_inline BITS_PER_PIXEL MapBitsPerPixel(uint32_t bpp)
        {
            BITS_PER_PIXEL ret = BitsPerPixels::UNKNOWN;
#if VKE_USE_DEVIL
            ret = (BitsPerPixels::BPP)bpp;
#endif
            return ret;
        }

        vke_force_inline uint32_t GetImageFormatChannelCount(const PIXEL_FORMAT& format)
        {
            static const uint32_t aChannels[] =
            {
                0, // unknown
                1, // red
                2, // rg
                3, // rgb
                4, // rgba
                1, // alpha
                3, // bgr
                4, // bgra
                1, // luminance
            };
            return aChannels[format];
        }

        struct SImageExtValue
        {
            cstr_t              pExt;
            IMAGE_FILE_FORMAT   format;
        };

        static const SImageExtValue g_aFileExtensions[ImageFileFormats::_MAX_COUNT] =
        {
            { "", ImageFileFormats::UNKNOWN },
            { "bmp", ImageFileFormats::BMP },
            { "dds", ImageFileFormats::DDS },
            { "gif", ImageFileFormats::GIF },
            { "hdr", ImageFileFormats::HDR },
            { "jpg", ImageFileFormats::JPG },
            { "jpeg", ImageFileFormats::JPG },
            { "png", ImageFileFormats::PNG },
            { "tif", ImageFileFormats::TIFF },
            { "tiff", ImageFileFormats::TIFF },
            { "tga", ImageFileFormats::TGA },
        };

        IMAGE_FILE_FORMAT CImageManager::_DetermineFileFormat(const CFile* pFile) const
        {
            IMAGE_FILE_FORMAT ret = ImageFileFormats::UNKNOWN;
            // Try use name extension
            cstr_t pExt = pFile->GetExtension();
            if( pExt != nullptr )
            {
                for (uint32_t i = 0; i < ImageFileFormats::_MAX_COUNT; ++i)
                {
                    if (strcmp(pExt, g_aFileExtensions[i].pExt) == 0)
                    {
                        ret = g_aFileExtensions[i].format;
                        break;
                    }
                }
            }
            else
            {
                // If file name ext doesn't exists, use file data
                /// TODO: determine file  format by its header
            }
            return ret;
        }

#if VKE_USE_DIRECTXTEX
        vke_force_inline DirectX::WICCodecs MapFileFormatToWICCodec(IMAGE_FILE_FORMAT fmt)
        {
            static const DirectX::WICCodecs aCodecs[] =
            {
                (DirectX::WICCodecs)0, // unknown
                DirectX::WIC_CODEC_BMP,
                (DirectX::WICCodecs)0, // dds
                DirectX::WIC_CODEC_GIF,
                (DirectX::WICCodecs)0, // hdr
                DirectX::WIC_CODEC_JPEG,
                DirectX::WIC_CODEC_PNG,
                DirectX::WIC_CODEC_TIFF,
                (DirectX::WICCodecs)0, // tga
            };
            return aCodecs[fmt];
        }

        vke_force_inline IMAGE_TYPE MapDXGIDimmensionToImageType(DirectX::TEX_DIMENSION dimm)
        {
            static const IMAGE_TYPE aTypes[] =
            {
                RenderSystem::TextureTypes::TEXTURE_1D, // 0 unused
                RenderSystem::TextureTypes::TEXTURE_1D, // 1 unused
                RenderSystem::TextureTypes::TEXTURE_1D, // 1d
                RenderSystem::TextureTypes::TEXTURE_2D, // 2d
                RenderSystem::TextureTypes::TEXTURE_3D // 3d
            };
            return aTypes[dimm];
        }
#endif
#if DXGI_FORMAT_DEFINED
        vke_force_inline RenderSystem::FORMAT MapDXGIFormatToRenderSystemFormat(DXGI_FORMAT fmt)
        {
            using namespace RenderSystem;
            static const RenderSystem::FORMAT aFormats[] =
            {
                Formats::UNDEFINED, // unknown
                Formats::R32G32B32A32_SFLOAT, // r32g32b32a32 typeless
                Formats::R32G32B32A32_SFLOAT, //
                Formats::R32G32B32A32_UINT, //
                Formats::R32G32B32A32_SINT, //
                Formats::R32G32B32_SFLOAT, // r32g32b32 typeless
                Formats::R32G32B32_SFLOAT, //
                Formats::R32G32B32_UINT, //
                Formats::R32G32B32_SINT, //
                Formats::R16G16B16A16_SFLOAT, // r16g16b16a16 typeless
                Formats::R16G16B16A16_SFLOAT, //
                Formats::R16G16B16A16_UNORM, //
                Formats::R16G16B16A16_UINT, //
                Formats::R16G16B16A16_SNORM, //
                Formats::R16G16B16A16_SINT, //
                Formats::R32G32_SFLOAT, // r32g32 typeless
                Formats::R32G32_SFLOAT, //
                Formats::R32G32_UINT, //
                Formats::R32G32_SINT, //
                Formats::UNDEFINED, // r32g8x24 typeless
                Formats::UNDEFINED, // d32 float s8x24 uint
                Formats::UNDEFINED, // r32 float x8x24 uint
                Formats::UNDEFINED, // x32 typeless g8x24 uint
                Formats::A2R10G10B10_UNORM_PACK32, // r10g10b10a2 typeless
                Formats::A2R10G10B10_UNORM_PACK32, //
                Formats::A2R10G10B10_UINT_PACK32, //
                Formats::B10G11R11_UFLOAT_PACK32, //
                Formats::UNDEFINED, // r8g8b8a8 typeless
                Formats::R8G8B8A8_UNORM, //
                Formats::R8G8B8A8_SRGB, //
                Formats::R8G8B8A8_UINT, //
                Formats::R8G8B8A8_SNORM, //
                Formats::R8G8B8A8_SINT, //
                Formats::UNDEFINED, // r16g16 typeless
                Formats::R16G16_SFLOAT, //
                Formats::R16G16_UNORM, //
                Formats::R16G16_UINT, //
                Formats::R16G16_SNORM, //
                Formats::R16G16_SINT, //
                Formats::UNDEFINED, // r32 typeless
                Formats::D32_SFLOAT, //
                Formats::R32_SFLOAT, //
                Formats::R32_UINT, //
                Formats::R32_SINT, //
                Formats::UNDEFINED, // r24g8 typeless
                Formats::D24_UNORM_S8_UINT, //
                Formats::X8_D24_UNORM_PACK32, //
                Formats::UNDEFINED, // x24 typeless g8 uint
                Formats::UNDEFINED, // r8g8 typeless
                Formats::R8G8_UNORM, //
                Formats::R8G8_UINT, //
                Formats::R8G8_SNORM, //
                Formats::R8G8_SINT, //
                Formats::UNDEFINED, // r16 typeless
                Formats::R16_SFLOAT, //
                Formats::D16_UNORM, //
                Formats::R16_UNORM, //
                Formats::R16_UINT, //
                Formats::R16_SNORM, //
                Formats::R16_SINT, //
                Formats::UNDEFINED, // r8 typeless
                Formats::R8_UNORM, //
                Formats::R8_UINT, //
                Formats::R8_SNORM, //
                Formats::R8_SINT, //
                Formats::UNDEFINED, // a8 unorm
                Formats::UNDEFINED, // r1 unorm
                Formats::UNDEFINED, // r9g9b9e5 sharedxp
                Formats::UNDEFINED, // r8g8 b8g8 unorm
                Formats::UNDEFINED, // g8r8 g8b8 unorm
                Formats::UNDEFINED, // bc1 typeless
                Formats::BC1_RGB_UNORM_BLOCK, //
                Formats::BC1_RGB_SRGB_BLOCK, //
                Formats::UNDEFINED, // bc2 typeless
                Formats::BC2_UNORM_BLOCK, //
                Formats::BC2_SRGB_BLOCK, //
                Formats::BC3_UNORM_BLOCK, // bc3 typeless
                Formats::BC3_UNORM_BLOCK, //
                Formats::BC3_SRGB_BLOCK, //
                Formats::BC4_UNORM_BLOCK, //
                Formats::BC4_UNORM_BLOCK, //
                Formats::BC4_SNORM_BLOCK, //
                Formats::BC5_UNORM_BLOCK, // typeless
                Formats::BC5_UNORM_BLOCK, //
                Formats::BC5_SNORM_BLOCK, //
                Formats::B5G6R5_UNORM_PACK16, //
                Formats::B5G5R5A1_UNORM_PACK16, //
                Formats::B8G8R8A8_UNORM, //
                Formats::B8G8R8A8_UNORM, // b8g8r8x8 unorm
                Formats::UNDEFINED, // r10g10b10 xr bias a2 unorm
                Formats::B8G8R8A8_UNORM, // b8g8r8a8 typeless
                Formats::B8G8R8A8_SRGB, //
                Formats::B8G8R8A8_UNORM, // b8g8r8x8 typeless
                Formats::B8G8R8A8_SRGB, // b8g8r8x8 srgb
                Formats::BC6H_SFLOAT_BLOCK, // bc6h typeless
                Formats::BC6H_UFLOAT_BLOCK, //
                Formats::BC6H_SFLOAT_BLOCK, //
                Formats::BC7_UNORM_BLOCK, // bc7 typeless
                Formats::BC7_UNORM_BLOCK, //
                Formats::BC7_SRGB_BLOCK, //
                Formats::UNDEFINED, // ayuv
                Formats::UNDEFINED, // y410
                Formats::UNDEFINED, // y416
                Formats::UNDEFINED, // nv12
                Formats::UNDEFINED, // p010
                Formats::UNDEFINED, // p016
                Formats::UNDEFINED, // 420 opaque
                Formats::UNDEFINED, // yuv2
                Formats::UNDEFINED, // y210
                Formats::UNDEFINED, // y216
                Formats::UNDEFINED, // nv11
                Formats::UNDEFINED, // ai44
                Formats::UNDEFINED, // ia44
                Formats::UNDEFINED, // p8
                Formats::UNDEFINED, // a8p8
                Formats::B4G4R4A4_UNORM_PACK16, //
                Formats::UNDEFINED, // p208
                Formats::UNDEFINED, // v208
                Formats::UNDEFINED // v408
            };
            return aFormats[fmt];
        }

        vke_force_inline DXGI_FORMAT MapPixelFormatToDXGIFormat(const PIXEL_FORMAT& fmt)
        {
            static const DXGI_FORMAT aFormats[] =
            {
                DXGI_FORMAT_UNKNOWN, // UNDEFINED,
                DXGI_FORMAT_UNKNOWN, // R4G4_UNORM_PACK8,
                DXGI_FORMAT_UNKNOWN, // R4G4B4A4_UNORM_PACK16,
                DXGI_FORMAT_B4G4R4A4_UNORM, // B4G4R4A4_UNORM_PACK16,
                DXGI_FORMAT_UNKNOWN, // R5G6B5_UNORM_PACK16,
                DXGI_FORMAT_B5G6R5_UNORM, // B5G6R5_UNORM_PACK16,
                DXGI_FORMAT_B5G5R5A1_UNORM, // R5G5B5A1_UNORM_PACK16,
                DXGI_FORMAT_UNKNOWN, // B5G5R5A1_UNORM_PACK16,
                DXGI_FORMAT_UNKNOWN, // A1R5G5B5_UNORM_PACK16,
                DXGI_FORMAT_R8_UNORM, // R8_UNORM,
                DXGI_FORMAT_R8_SNORM, // R8_SNORM,
                DXGI_FORMAT_UNKNOWN, // R8_USCALED,
                DXGI_FORMAT_UNKNOWN, // R8_SSCALED,
                DXGI_FORMAT_R8_UINT, // R8_UINT,
                DXGI_FORMAT_R8_SINT, // R8_SINT,
                DXGI_FORMAT_R8_TYPELESS, // R8_SRGB,
                DXGI_FORMAT_R8G8_UNORM, // R8G8_UNORM,
                DXGI_FORMAT_R8G8_SNORM, // R8G8_SNORM,
                DXGI_FORMAT_UNKNOWN, // R8G8_USCALED,
                DXGI_FORMAT_UNKNOWN, // R8G8_SSCALED,
                DXGI_FORMAT_R8G8_UINT, // R8G8_UINT,
                DXGI_FORMAT_R8G8_SINT, // R8G8_SINT,
                DXGI_FORMAT_R8G8_TYPELESS, // R8G8_SRGB,
                DXGI_FORMAT_UNKNOWN, // R8G8B8_UNORM,
                DXGI_FORMAT_UNKNOWN, // R8G8B8_SNORM,
                DXGI_FORMAT_UNKNOWN, // R8G8B8_USCALED,
                DXGI_FORMAT_UNKNOWN, // R8G8B8_SSCALED,
                DXGI_FORMAT_UNKNOWN, // R8G8B8_UINT,
                DXGI_FORMAT_UNKNOWN, // R8G8B8_SINT,
                DXGI_FORMAT_UNKNOWN, // R8G8B8_SRGB,
                DXGI_FORMAT_B8G8R8X8_UNORM, // B8G8R8_UNORM,
                DXGI_FORMAT_B8G8R8X8_UNORM_SRGB, // B8G8R8_SNORM,
                DXGI_FORMAT_UNKNOWN, // B8G8R8_USCALED,
                DXGI_FORMAT_UNKNOWN, // B8G8R8_SSCALED,
                DXGI_FORMAT_UNKNOWN, // B8G8R8_UINT,
                DXGI_FORMAT_UNKNOWN, // B8G8R8_SINT,
                DXGI_FORMAT_UNKNOWN, // B8G8R8_SRGB,
                DXGI_FORMAT_R8G8B8A8_UNORM, // R8G8B8A8_UNORM,
                DXGI_FORMAT_R8G8B8A8_SNORM, // R8G8B8A8_SNORM,
                DXGI_FORMAT_UNKNOWN, // R8G8B8A8_USCALED,
                DXGI_FORMAT_UNKNOWN, // R8G8B8A8_SSCALED,
                DXGI_FORMAT_R8G8B8A8_UINT, // R8G8B8A8_UINT,
                DXGI_FORMAT_R8G8B8A8_SINT, // R8G8B8A8_SINT,
                DXGI_FORMAT_R8G8B8A8_TYPELESS, // R8G8B8A8_SRGB,
                DXGI_FORMAT_B8G8R8A8_UNORM, // B8G8R8A8_UNORM,
                DXGI_FORMAT_UNKNOWN, // B8G8R8A8_SNORM,
                DXGI_FORMAT_UNKNOWN, // B8G8R8A8_USCALED,
                DXGI_FORMAT_UNKNOWN, // B8G8R8A8_SSCALED,
                DXGI_FORMAT_UNKNOWN, // B8G8R8A8_UINT,
                DXGI_FORMAT_UNKNOWN, // B8G8R8A8_SINT,
                DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, // B8G8R8A8_SRGB,
                DXGI_FORMAT_UNKNOWN, // A8B8G8R8_UNORM_PACK32,
                DXGI_FORMAT_UNKNOWN, // A8B8G8R8_SNORM_PACK32,
                DXGI_FORMAT_UNKNOWN, // A8B8G8R8_USCALED_PACK32,
                DXGI_FORMAT_UNKNOWN, // A8B8G8R8_SSCALED_PACK32,
                DXGI_FORMAT_UNKNOWN, // A8B8G8R8_UINT_PACK32,
                DXGI_FORMAT_UNKNOWN, // A8B8G8R8_SINT_PACK32,
                DXGI_FORMAT_UNKNOWN, // A8B8G8R8_SRGB_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2R10G10B10_UNORM_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2R10G10B10_SNORM_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2R10G10B10_USCALED_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2R10G10B10_SSCALED_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2R10G10B10_UINT_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2R10G10B10_SINT_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2B10G10R10_UNORM_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2B10G10R10_SNORM_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2B10G10R10_USCALED_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2B10G10R10_SSCALED_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2B10G10R10_UINT_PACK32,
                DXGI_FORMAT_UNKNOWN, // A2B10G10R10_SINT_PACK32,
                DXGI_FORMAT_R16_UNORM, // R16_UNORM,
                DXGI_FORMAT_R16_SNORM, // R16_SNORM,
                DXGI_FORMAT_UNKNOWN, // R16_USCALED,
                DXGI_FORMAT_UNKNOWN, // R16_SSCALED,
                DXGI_FORMAT_R16_UINT, // R16_UINT,
                DXGI_FORMAT_R16_SINT, // R16_SINT,
                DXGI_FORMAT_R16_FLOAT, // R16_SFLOAT,
                DXGI_FORMAT_R16G16_UNORM, // R16G16_UNORM,
                DXGI_FORMAT_R16G16_SNORM, // R16G16_SNORM,
                DXGI_FORMAT_UNKNOWN, // R16G16_USCALED,
                DXGI_FORMAT_UNKNOWN, // R16G16_SSCALED,
                DXGI_FORMAT_R16G16_UINT, // R16G16_UINT,
                DXGI_FORMAT_R16G16_SINT, // R16G16_SINT,
                DXGI_FORMAT_R16G16_FLOAT, // R16G16_SFLOAT,
                DXGI_FORMAT_UNKNOWN, // R16G16B16_UNORM,
                DXGI_FORMAT_UNKNOWN, // R16G16B16_SNORM,
                DXGI_FORMAT_UNKNOWN, // R16G16B16_USCALED,
                DXGI_FORMAT_UNKNOWN, // R16G16B16_SSCALED,
                DXGI_FORMAT_UNKNOWN, // R16G16B16_UINT,
                DXGI_FORMAT_UNKNOWN, // R16G16B16_SINT,
                DXGI_FORMAT_UNKNOWN, // R16G16B16_SFLOAT,
                DXGI_FORMAT_R16G16B16A16_UNORM, // R16G16B16A16_UNORM,
                DXGI_FORMAT_R16G16B16A16_SNORM, // R16G16B16A16_SNORM,
                DXGI_FORMAT_UNKNOWN, // R16G16B16A16_USCALED,
                DXGI_FORMAT_UNKNOWN, // R16G16B16A16_SSCALED,
                DXGI_FORMAT_R16G16B16A16_UINT, // R16G16B16A16_UINT,
                DXGI_FORMAT_R16G16B16A16_SINT, // R16G16B16A16_SINT,
                DXGI_FORMAT_R16G16B16A16_FLOAT, // R16G16B16A16_SFLOAT,
                DXGI_FORMAT_R32_UINT, // R32_UINT,
                DXGI_FORMAT_R32_SINT, // R32_SINT,
                DXGI_FORMAT_R32_FLOAT, // R32_SFLOAT,
                DXGI_FORMAT_R32G32_UINT, // R32G32_UINT,
                DXGI_FORMAT_R32G32_SINT, // R32G32_SINT,
                DXGI_FORMAT_R32G32_FLOAT, // R32G32_SFLOAT,
                DXGI_FORMAT_R32G32B32_UINT, // R32G32B32_UINT,
                DXGI_FORMAT_R32G32B32_SINT, // R32G32B32_SINT,
                DXGI_FORMAT_R32G32B32_FLOAT, // R32G32B32_SFLOAT,
                DXGI_FORMAT_R32G32B32A32_UINT, // R32G32B32A32_UINT,
                DXGI_FORMAT_R32G32B32A32_SINT, // R32G32B32A32_SINT,
                DXGI_FORMAT_R32G32B32A32_FLOAT, // R32G32B32A32_SFLOAT,
                DXGI_FORMAT_UNKNOWN, // R64_UINT,
                DXGI_FORMAT_UNKNOWN, // R64_SINT,
                DXGI_FORMAT_UNKNOWN, // R64_SFLOAT,
                DXGI_FORMAT_UNKNOWN, // R64G64_UINT,
                DXGI_FORMAT_UNKNOWN, // R64G64_SINT,
                DXGI_FORMAT_UNKNOWN, // R64G64_SFLOAT,
                DXGI_FORMAT_UNKNOWN, // R64G64B64_UINT,
                DXGI_FORMAT_UNKNOWN, // R64G64B64_SINT,
                DXGI_FORMAT_UNKNOWN, // R64G64B64_SFLOAT,
                DXGI_FORMAT_UNKNOWN, // R64G64B64A64_UINT,
                DXGI_FORMAT_UNKNOWN, // R64G64B64A64_SINT,
                DXGI_FORMAT_UNKNOWN, // R64G64B64A64_SFLOAT,
                DXGI_FORMAT_UNKNOWN, // B10G11R11_UFLOAT_PACK32,
                DXGI_FORMAT_R9G9B9E5_SHAREDEXP, // E5B9G9R9_UFLOAT_PACK32,
                DXGI_FORMAT_D16_UNORM, // D16_UNORM,
                DXGI_FORMAT_R24_UNORM_X8_TYPELESS, // X8_D24_UNORM_PACK32,
                DXGI_FORMAT_D32_FLOAT, // D32_SFLOAT,
                DXGI_FORMAT_D32_FLOAT_S8X24_UINT, // S8_UINT,
                DXGI_FORMAT_UNKNOWN, // D16_UNORM_S8_UINT,
                DXGI_FORMAT_D24_UNORM_S8_UINT, // D24_UNORM_S8_UINT,
                DXGI_FORMAT_D32_FLOAT_S8X24_UINT, // D32_SFLOAT_S8_UINT,
                DXGI_FORMAT_BC1_UNORM, // BC1_RGB_UNORM_BLOCK,
                DXGI_FORMAT_BC1_UNORM_SRGB, // BC1_RGB_SRGB_BLOCK,
                DXGI_FORMAT_BC1_UNORM, // BC1_RGBA_UNORM_BLOCK,
                DXGI_FORMAT_BC1_UNORM_SRGB, // BC1_RGBA_SRGB_BLOCK,
                DXGI_FORMAT_BC2_UNORM, // BC2_UNORM_BLOCK,
                DXGI_FORMAT_BC2_UNORM_SRGB, // BC2_SRGB_BLOCK,
                DXGI_FORMAT_BC3_UNORM, // BC3_UNORM_BLOCK,
                DXGI_FORMAT_BC2_UNORM_SRGB, // BC3_SRGB_BLOCK,
                DXGI_FORMAT_BC4_UNORM, // BC4_UNORM_BLOCK,
                DXGI_FORMAT_BC4_SNORM, // BC4_SNORM_BLOCK,
                DXGI_FORMAT_BC5_UNORM, // BC5_UNORM_BLOCK,
                DXGI_FORMAT_BC5_SNORM, // BC5_SNORM_BLOCK,
                DXGI_FORMAT_BC6H_UF16, // BC6H_UFLOAT_BLOCK,
                DXGI_FORMAT_BC6H_SF16, // BC6H_SFLOAT_BLOCK,
                DXGI_FORMAT_BC7_UNORM, // BC7_UNORM_BLOCK,
                DXGI_FORMAT_BC7_UNORM_SRGB, // BC7_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ETC2_R8G8B8_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ETC2_R8G8B8_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ETC2_R8G8B8A1_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ETC2_R8G8B8A1_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ETC2_R8G8B8A8_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ETC2_R8G8B8A8_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // EAC_R11_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // EAC_R11_SNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // EAC_R11G11_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // EAC_R11G11_SNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_4x4_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_4x4_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_5x4_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_5x4_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_5x5_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_5x5_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_6x5_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_6x5_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_6x6_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_6x6_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_8x5_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_8x5_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_8x6_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_8x6_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_8x8_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_8x8_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_10x5_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_10x5_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_10x6_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_10x6_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_10x8_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_10x8_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_10x10_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_10x10_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_12x10_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_12x10_SRGB_BLOCK,
                DXGI_FORMAT_UNKNOWN, // ASTC_12x12_UNORM_BLOCK,
                DXGI_FORMAT_UNKNOWN // ASTC_12x12_SRGB_BLOCK,
            };
            return aFormats[fmt];
        }

        vke_force_inline RenderSystem::TEXTURE_TYPE MapTexDimmensionToTextureType(const DirectX::TEX_DIMENSION& dimm)
        {
            static const RenderSystem::TEXTURE_TYPE aTypes[] =
            {
                RenderSystem::TextureTypes::TEXTURE_1D, //
                RenderSystem::TextureTypes::TEXTURE_1D, //
                RenderSystem::TextureTypes::TEXTURE_1D, // TEX_DIMENSION_TEXTURE1D = 2,
                RenderSystem::TextureTypes::TEXTURE_2D, // TEX_DIMENSION_TEXTURE2D = 3,
                RenderSystem::TextureTypes::TEXTURE_3D, // TEX_DIMENSION_TEXTURE3D = 4,
            };
            return aTypes[dimm];
        }

#endif

        Result CImageManager::_CreateDirectXTexImage(const CFile* pFile, CImage** ppInOut)
        {
            Result ret = VKE_FAIL;
#if VKE_USE_DIRECTXTEX
            CImage* pImg = *ppInOut;
            const void* pData = pFile->GetData();
            const auto dataSize = pFile->GetDataSize();
            IMAGE_FILE_FORMAT fileFormat = _DetermineFileFormat(pFile);
            cstr_t pFileName = pFile->GetDesc().pFileName;
            DirectX::TexMetadata Metadata;
            auto& Image = pImg->m_DXImage;
            SImageDesc Desc;
            if( fileFormat == ImageFileFormats::DDS )
            {
                static const DirectX::DDS_FLAGS ddsFlags = DirectX::DDS_FLAGS_ALLOW_LARGE_FILES |
                    DirectX::DDS_FLAGS_LEGACY_DWORD;

                ::HRESULT hr = DirectX::LoadFromDDSMemory(pData, dataSize, ddsFlags, &Metadata, Image);
                if( SUCCEEDED( hr ) )
                {
                    if( DirectX::IsTypeless( Metadata.format ) )
                    {
                        Metadata.format = DirectX::MakeTypelessUNORM( Metadata.format );
                        if (!DirectX::IsTypeless(Metadata.format))
                        {
                            Image.OverrideFormat(Metadata.format);
                        }
                        else
                        {
                            VKE_LOG_ERR("Failed to load DDS image: " << pFileName << " due to typeless format.");
                            goto ERR;
                        }
                    }

                    ret = VKE_OK;
                }
                else
                {
                    VKE_LOG_ERR( "Failed to load DDS image: " << pFileName << " error: " << hr );
                    goto ERR;
                }
            }
            else if (fileFormat == ImageFileFormats::TGA)
            {
                ::HRESULT hr = DirectX::LoadFromTGAMemory(pData, dataSize, &Metadata, Image);
                if( SUCCEEDED( hr ) )
                {
                    ret = VKE_OK;
                }
            }
            else if (fileFormat == ImageFileFormats::HDR)
            {
                ::HRESULT hr = DirectX::LoadFromHDRMemory(pData, dataSize, &Metadata, Image);
                if (SUCCEEDED(hr))
                {
                    ret = VKE_OK;
                }
            }
            else // Copied from Texconv.cpp
            {
                // WIC shares the same filter values for mode and dither
                static_assert(static_cast<int>(DirectX::WIC_FLAGS_DITHER) == static_cast<int>(DirectX::TEX_FILTER_DITHER), "WIC_FLAGS_* & TEX_FILTER_* should match");
                static_assert(static_cast<int>(DirectX::WIC_FLAGS_DITHER_DIFFUSION) == static_cast<int>(DirectX::TEX_FILTER_DITHER_DIFFUSION), "WIC_FLAGS_* & TEX_FILTER_* should match");
                static_assert(static_cast<int>(DirectX::WIC_FLAGS_FILTER_POINT) == static_cast<int>(DirectX::TEX_FILTER_POINT), "WIC_FLAGS_* & TEX_FILTER_* should match");
                static_assert(static_cast<int>(DirectX::WIC_FLAGS_FILTER_LINEAR) == static_cast<int>(DirectX::TEX_FILTER_LINEAR), "WIC_FLAGS_* & TEX_FILTER_* should match");
                static_assert(static_cast<int>(DirectX::WIC_FLAGS_FILTER_CUBIC) == static_cast<int>(DirectX::TEX_FILTER_CUBIC), "WIC_FLAGS_* & TEX_FILTER_* should match");
                static_assert(static_cast<int>(DirectX::WIC_FLAGS_FILTER_FANT) == static_cast<int>(DirectX::TEX_FILTER_FANT), "WIC_FLAGS_* & TEX_FILTER_* should match");

                const DirectX::TEX_FILTER_FLAGS texFilter = DirectX::TEX_FILTER_DEFAULT;

                DirectX::WIC_FLAGS wicFlags = DirectX::WIC_FLAGS_NONE | texFilter;

                //hr = LoadFromWICFile(pConv->szSrc, wicFlags, &Metadata, Image);
                ::HRESULT hr = LoadFromWICMemory(pData, dataSize, wicFlags, &Metadata, Image);
                if( SUCCEEDED( hr ) )
                {
                    ret = VKE_OK;
                }
                else
                {
                    VKE_LOG_ERR( "Failed to load WIC image: " << pFileName << " error: " << hr );
                    goto ERR;
                }
            }
            if (VKE_SUCCEEDED(ret))
            {
                Desc.Size.width = (image_dimm_t)Metadata.width;
                Desc.Size.height = (image_dimm_t)Metadata.height;
                Desc.depth = (image_dimm_t)Metadata.depth;
                Desc.type = MapDXGIDimmensionToImageType(Metadata.dimension);
                Desc.format = MapDXGIFormatToRenderSystemFormat(Metadata.format);
                pImg->_Init( Desc );
            }
#endif
            return ret;
#if VKE_USE_DIRECTXTEX
            ERR:
                ret = VKE_FAIL;
                return ret;
#endif
        }

        Result CImageManager::_CreateImage(CFile* pFile, CImage** ppInOut)
        {
            Result ret = VKE_FAIL;
            CImage* pImg = *ppInOut;
            SImageDesc Desc;
#if VKE_USE_DEVIL
            auto idx = ilGenImage();
            ilBindImage( idx );
            void* pData = (void*)pFile->GetData();
            uint32_t size = pFile->GetDataSize();
            if( ilLoadL( IL_TYPE_UNKNOWN, pData, size ) )
            {
                pImg->m_hNative = idx;
                auto bpp = ilGetInteger(IL_IMAGE_BITS_PER_PIXEL);
                auto format = ilGetInteger(IL_IMAGE_FORMAT);
                auto type = ilGetInteger(IL_IMAGE_TYPE);
                Desc.bitsPerPixel = MapBitsPerPixel( bpp );
                Desc.Size.width = (image_dimm_t)ilGetInteger(IL_IMAGE_WIDTH);
                Desc.Size.height = (image_dimm_t)ilGetInteger(IL_IMAGE_HEIGHT);
                Desc.format = MapImageFormat( format );
                Desc.type = MapImageDataType( type );

                if (pImg->m_hNative != 0)
                {
                    pImg->_Init(Desc);
                    ret = VKE_OK;
                }
            }
            else
            {
                ilDeleteImage(idx);
                VKE_LOG_ERR( "Unable to create DevIL image for image: " << pFile->GetDesc().pFileName );
            }
#elif VKE_USE_DIRECTXTEX
            ret = _CreateDirectXTexImage( pFile, &pImg );
#endif
            if (VKE_SUCCEEDED(ret))
            {
                pImg->m_Desc.Name = pFile->GetDesc().pFileName;
            }
            return ret;
        }

        ImageRefPtr CImageManager::GetImage(const ImageHandle& hImg)
        {
            CImage* pImg = nullptr;
            m_Buffer.Find( hImg.handle, &pImg );
            return ImageRefPtr( pImg );
        }

        ImageHandle CImageManager::Copy(const SCreateCopyImageInfo& Info)
        {
            ImageHandle hRet = INVALID_HANDLE;
            ImagePtr pImg = GetImage(Info.hSrcImage);
            if( pImg.IsValid() )
            {

            }
            return hRet;
        }

        Result CImageManager::Slice(const SSliceImageInfo& Info, ImageHandle* pOut)
        {
            Result ret = VKE_OK;
            ImagePtr pSrcImg = GetImage(Info.hSrcImage);
            const hash_t& srcHash = Info.hSrcImage.handle;

            for (uint32_t i = 0; i < Info.vRegions.GetCount(); ++i)
            {
                const SImageRegion& Region = Info.vRegions[i];
                Utils::SHash Hash;
                Hash.Combine(srcHash, Region.Size.width, Region.Size.height, Region.Offset.x, Region.Offset.y, i);
                CImage* pDstImg;
                Result res = _CreateImage(Hash.value, &pDstImg);
                if (res == VKE_OK)
                {
#if VKE_USE_DIRECTXTEX
                    const auto& SrcMetadata = pSrcImg->m_DXImage.GetMetadata();

                    DirectX::CP_FLAGS flags = DirectX::CP_FLAGS_NONE;
                    //DXGI_FORMAT format = MapPixelFormatToDXGIFormat( Desc.format );
                    DXGI_FORMAT format = SrcMetadata.format;
                    const auto mipCount = SrcMetadata.mipLevels;
                    const auto arraySize = SrcMetadata.arraySize;
                    DirectX::ScratchImage& DstImg = pDstImg->m_DXImage;
                    ::HRESULT hr = DstImg.Initialize2D(format, Region.Size.width, Region.Size.height, arraySize,
                        mipCount, flags);
                    if (hr != S_OK)
                    {
                        ret = VKE_FAIL;
                        VKE_LOG_ERR("Failed to DirectX::ScratchImage::Initialize2D: " << hr);
                        break;
                    }

                    DirectX::Rect SrcRect(Region.Offset.x, Region.Offset.y, Region.Size.x, Region.Size.y);

                    const auto& SrcImage = *pSrcImg->m_DXImage.GetImage(0, 0, 0);
                    const auto& DstImage = *pDstImg->m_DXImage.GetImage(0, 0, 0);

                    hr = DirectX::CopyRectangle(SrcImage, SrcRect, DstImage, DirectX::TEX_FILTER_DEFAULT, 0, 0);
                    if (hr != S_OK)
                    {
                        ret = VKE_FAIL;
                        VKE_LOG_ERR("Failed to DirectX::CopyRectangle: " << hr);
                        break;
                    }

                    const auto& DstMetadata = DstImg.GetMetadata();
                    auto& DstDesc = pDstImg->m_Desc;
                    DstDesc.Size.width = (image_dimm_t)DstMetadata.width;
                    DstDesc.Size.height = (image_dimm_t)DstMetadata.height;
                    DstDesc.depth = (uint16_t)DstMetadata.depth;
                    DstDesc.format = MapDXGIFormatToRenderSystemFormat(DstMetadata.format);
                    DstDesc.type = MapTexDimmensionToTextureType(DstMetadata.dimension);
                    DstDesc.Name.Format("%s_%d", pSrcImg->GetDesc().Name.GetData(), i);
                    pOut[i] = pDstImg->GetHandle();
#else
#error "not implemented!"
#endif
                }
                else if (res == VKE_FAIL)
                {
                    VKE_LOG_ERR("Image with hash: " << Hash.value << " already exits. Cannot create new image with the same hash.");
                    ret = res;
                    break;
                }
                if (res == VKE_ENOMEMORY)
                {
                    VKE_LOG_ERR("Unable to allocate memory for image slice.");
                    ret = res;
                    break;
                }
            }
            return ret;
        }

        ImageHandle CImageManager::CreateNormalMap(const ImageHandle& hSrcImg)
        {
            ImageHandle hRet = INVALID_HANDLE;
#if VKE_USE_DIRECTXTEX
            CImage* pImg = GetImage(hSrcImg).Get();
            const auto& Meta = pImg->m_DXImage.GetMetadata();

            SImageDesc ImgDesc = pImg->GetDesc();
            ImgDesc.format = RenderSystem::Formats::R8G8B8A8_UNORM;
            ImgDesc.Name += "_normal";
            hash_t dstHash = CalcImageHash(ImgDesc);
            CImage* pDstImg;
            Result res = _CreateImage(dstHash, &pDstImg);
            if (VKE_SUCCEEDED(res))
            {
                pImg->_Init(ImgDesc);
                auto& DstImage = pDstImg->m_DXImage;
                ::HRESULT hr = DstImage.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, Meta.width, Meta.height, Meta.arraySize, Meta.mipLevels);
                if (hr == S_OK)
                {
                    hr = DirectX::ComputeNormalMap(pImg->m_DXImage.GetImages(), pImg->m_DXImage.GetImageCount(), Meta,
                        DirectX::CNMAP_DEFAULT, 0.0f, DXGI_FORMAT_R8G8B8A8_UNORM, DstImage);
                    if (hr == S_OK)
                    {
                        hRet = pDstImg->GetHandle();
                    }
                    else
                    {

                    }
                }
                else
                {
                }
            }
#else

#endif
            return hRet;
        }

#if VKE_USE_DIRECTXTEX
        DirectX::WICCodecs MapImageFormatToCodec(const IMAGE_FILE_FORMAT& fmt)
        {
            static const DirectX::WICCodecs aCodecs[] =
            {
                (DirectX::WICCodecs)0, // UNKNOWN,
                DirectX::WIC_CODEC_BMP, // BMP,
                (DirectX::WICCodecs)0, // DDS,
                DirectX::WIC_CODEC_GIF, // GIF,
                (DirectX::WICCodecs)0, // HDR,
                DirectX::WIC_CODEC_JPEG, // JPG,
                DirectX::WIC_CODEC_JPEG, // JPEG,
                DirectX::WIC_CODEC_PNG, // PNG,
                DirectX::WIC_CODEC_TIFF, // TIFF,
                DirectX::WIC_CODEC_TIFF, // TIF,
                (DirectX::WICCodecs)0, // TGA,
            };
            return aCodecs[fmt];
        }

#endif

        Result CImageManager::Save(const SSaveImageInfo& Info)
        {
            Result ret = VKE_FAIL;
#if VKE_USE_DIRECTXTEX
            DirectX::Image Img;
            if (Info.hImage != INVALID_HANDLE)
            {
                ImagePtr pImg = GetImage(Info.hImage);
                const auto& DxImage = pImg->m_DXImage;
                //const DirectX::Image* pDxImg = DxImage.GetImage(0, 0, 0);
                const auto& Metadata = DxImage.GetMetadata();

                size_t dataSize = DxImage.GetPixelsSize();

                Img.format = Metadata.format;
                Img.width = Metadata.width;
                Img.height = Metadata.height;
                Img.rowPitch = dataSize / Img.height;
                Img.slicePitch = dataSize;
                Img.pixels = DxImage.GetPixels();
            }
            else
            {
                auto pData = Info.pData;
                Img.format = MapPixelFormatToDXGIFormat(pData->format);
                Img.width = pData->Size.width;
                Img.height = pData->Size.height;
                Img.rowPitch = pData->rowPitch;
                Img.slicePitch = pData->slicePitch;
                Img.pixels = pData->pPixels;
            }

            Utils::TCString< wchar_t, Config::Resource::MAX_NAME_LENGTH > Name;
            Name.Convert( Info.pFileName );
            DirectX::WICCodecs codec = MapImageFormatToCodec(Info.format);

            if(codec != 0)
            {
                REFGUID guid = DirectX::GetWICCodec(codec);
                ::HRESULT hr = DirectX::SaveToWICFile(Img, DirectX::WIC_FLAGS_NONE, guid,
                    Name.GetData(), nullptr);

                if (hr == S_OK)
                {
                    ret = VKE_OK;
                }
            }
            else if (Info.format == ImageFileFormats::TGA)
            {
                ::HRESULT hr = DirectX::SaveToTGAFile(Img, Name.GetData(), nullptr);
                if (hr == S_OK)
                {
                    ret = VKE_OK;
                }
            }
            else if (Info.format == ImageFileFormats::HDR)
            {
                ::HRESULT hr = DirectX::SaveToHDRFile(Img, Name.GetData());
                if (hr == S_OK)
                {
                    ret = VKE_OK;
                }
            }
            else if (Info.format == ImageFileFormats::DDS)
            {
                ::HRESULT hr = DirectX::SaveToDDSFile(Img, DirectX::DDS_FLAGS_NONE, Name.GetData());
                if (hr == S_OK)
                {
                    ret = VKE_OK;
                }
            }

            if (VKE_FAILED(ret))
            {
                VKE_LOG_ERR("Unable to save image to file: " << Info.pFileName);
            }
#else
#error "implement"
#endif
            return ret;
        }

        void CImageManager::_GetTextureDesc(const CImage* pImg, RenderSystem::STextureDesc* pOut) const
        {
#if VKE_USE_DIRECTXTEX
            const auto& Metadata = pImg->m_DXImage.GetMetadata();

            pOut->arrayElementCount = (uint16_t)Metadata.arraySize;
            pOut->format = MapDXGIFormatToRenderSystemFormat(Metadata.format);
            pOut->Size.width = (RenderSystem::TextureSizeType)Metadata.width;
            pOut->Size.height = (RenderSystem::TextureSizeType)Metadata.height;
            pOut->memoryUsage = RenderSystem::MemoryUsages::GPU_ACCESS | RenderSystem::MemoryUsages::TEXTURE | RenderSystem::MemoryUsages::STATIC;
            pOut->mipmapCount = (uint16_t)Metadata.mipLevels;
            pOut->multisampling = RenderSystem::SampleCounts::SAMPLE_1;
            pOut->sliceCount = (uint16_t)Metadata.depth;
            pOut->type = MapTexDimmensionToTextureType(Metadata.dimension);
            pOut->usage = RenderSystem::TextureUsages::SAMPLED | RenderSystem::TextureUsages::TRANSFER_DST;
            pOut->Name = pImg->GetDesc().Name;
            VKE_RENDER_SYSTEM_SET_DEBUG_NAME(*pOut, pOut->Name.GetData());
#else
#error "implement"
#endif
        }

#if VKE_USE_DIRECTXTEX

        bool IsCompressed(DXGI_FORMAT fmt)
        {
            bool ret = false;
            switch (fmt)
            {
                case DXGI_FORMAT_BC1_TYPELESS:
                case DXGI_FORMAT_BC1_UNORM:
                case DXGI_FORMAT_BC1_UNORM_SRGB:
                case DXGI_FORMAT_BC2_TYPELESS:
                case DXGI_FORMAT_BC2_UNORM:
                case DXGI_FORMAT_BC2_UNORM_SRGB:
                case DXGI_FORMAT_BC3_TYPELESS:
                case DXGI_FORMAT_BC3_UNORM:
                case DXGI_FORMAT_BC3_UNORM_SRGB:
                case DXGI_FORMAT_BC4_SNORM:
                case DXGI_FORMAT_BC4_TYPELESS:
                case DXGI_FORMAT_BC4_UNORM:
                case DXGI_FORMAT_BC5_SNORM:
                case DXGI_FORMAT_BC5_TYPELESS:
                case DXGI_FORMAT_BC5_UNORM:
                case DXGI_FORMAT_BC6H_SF16:
                case DXGI_FORMAT_BC6H_TYPELESS:
                case DXGI_FORMAT_BC6H_UF16:
                case DXGI_FORMAT_BC7_TYPELESS:
                case DXGI_FORMAT_BC7_UNORM:
                case DXGI_FORMAT_BC7_UNORM_SRGB:
                {
                    ret = true;
                    break;
                }
            };
            return ret;
        }
#endif

        Result CImageManager::_Resize(const ImageSize& NewSize, CImage** ppInOut)
        {
            Result ret = VKE_FAIL;
#if VKE_USE_DIRECTXTEX
            CImage* pImg = *ppInOut;
            const auto& Meta = pImg->m_DXImage.GetMetadata();
            DirectX::ScratchImage NewImage;
            ::HRESULT hr = NewImage.Initialize2D(Meta.format, NewSize.width, NewSize.height, Meta.arraySize,
                Meta.mipLevels);

            if (hr == S_OK)
            {
                const bool needDecompress = IsCompressed(Meta.format);
                DirectX::ScratchImage TmpImg;
                DirectX::ScratchImage* pSrcImg = &pImg->m_DXImage;
                const DirectX::TexMetadata* pSrcMetadata = &pSrcImg->GetMetadata();

                if (needDecompress)
                {
                    hr = DirectX::Decompress(pImg->m_DXImage.GetImages(), pImg->m_DXImage.GetImageCount(), Meta,
                        DXGI_FORMAT_UNKNOWN, TmpImg);
                    if (hr == S_OK)
                    {
                        pSrcImg = &TmpImg;
                        pSrcMetadata = &TmpImg.GetMetadata();
                    }
                }

                hr = DirectX::Resize(pSrcImg->GetImages(), pSrcImg->GetImageCount(), *pSrcMetadata,
                    (size_t)NewSize.width, (size_t)NewSize.height, DirectX::TEX_FILTER_DEFAULT, NewImage);
                if (hr == S_OK)
                {
                    /*REFGUID guid = DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG);
                    DirectX::SaveToWICFile(*NewImage.GetImage(0,0,0), DirectX::WIC_FLAGS_NONE,
                        guid,
                        L"resize.png");*/

                    if (needDecompress)
                    {
                        DirectX::ScratchImage NewCompressed;
                        hr = DirectX::Compress(NewImage.GetImages(), NewImage.GetImageCount(), NewImage.GetMetadata(),
                            Meta.format, DirectX::TEX_COMPRESS_BC7_QUICK, 0.0f, NewCompressed);
                        if (hr == S_OK)
                        {
                            NewImage.Release();
                            NewImage = std::move(NewCompressed);
                        }
                    }

                    if (hr == S_OK)
                    {
                        pImg->m_DXImage.Release();
                        //hr = pImg->m_DXImage.Initialize(NewImage.GetMetadata());
                        pImg->m_DXImage = std::move(NewImage);
                        if (hr == S_OK)
                        {
                            const auto w = pImg->m_DXImage.GetMetadata().width;
                            ret = VKE_OK;
                        }
                        else
                        {
                            VKE_LOG_ERR("Unable to reinitialize image.");
                        }
                    }
                }
                else
                {
                    VKE_LOG_ERR("Unable to Resize DirectX Tex image.");
                }
            }
            else
            {
                VKE_LOG_ERR("Unable to initialize2D new image for resize.");
            }
#else
#error "implement"
#endif
            return ret;
        }

        Result CImageManager::Resize(const ImageSize& NewSize, ImagePtr* ppInOut)
        {
            CImage* pImg = ppInOut->Get();
            return _Resize(NewSize, &pImg);
        }

        Result CImageManager::Resize(const ImageSize& NewSize, ImageHandle* pInOut)
        {
            CImage* pImg = GetImage(*pInOut).Get();
            return _Resize( NewSize, &pImg );
        }

    } // Core
} // VKE