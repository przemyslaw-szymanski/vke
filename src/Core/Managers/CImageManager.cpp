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
            m_MemoryPool.Destroy();
        }

        hash_t CalcImageHash(const SLoadFileInfo& Info)
        {
            hash_t ret = 0;
            const hash_t h1 = CResource::CalcHash( Info.FileInfo );

            ret = h1;
            return ret;
        }

        ImageHandle CImageManager::Load(const SLoadFileInfo& Info)
        {
            // Check if such image is already loaded
            const hash_t hash = CalcImageHash( Info );
            CImage* pImage = nullptr;
            if( !m_Buffer.Find( hash, &pImage ) )
            {
                if( !m_Buffer.TryToReuse( 0, &pImage ) )
                {
                    if( VKE_SUCCEEDED( Memory::CreateObject( &m_MemoryPool, &pImage, this ) ) )
                    {
                        if( !m_Buffer.Add( hash, pImage ) )
                        {
                            Memory::DestroyObject( &m_MemoryPool, &pImage );
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR("Unable to create memory for CImage object: " << Info.FileInfo.pFileName);
                    }
                }
                if( pImage != nullptr )
                {
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
#if VKE_USE_DEVIL
            ilDeleteImage( (ILuint)pImg->m_hNative );
#elif VKE_USE_DIRECTXTEX
            pImg->m_DXImage.Release();
#endif
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
                Formats::UNDEFINED, // r32g32b32a32 typeless
                Formats::R32G32B32A32_SFLOAT, //
                Formats::R32G32B32A32_UINT, //
                Formats::R32G32B32A32_SINT, //
                Formats::UNDEFINED, // r32g32b32 typeless
                Formats::R32G32B32_SFLOAT, //
                Formats::R32G32B32_UINT, //
                Formats::R32G32B32_SINT, //
                Formats::UNDEFINED, // r16g16b16a16 typeless
                Formats::R16G16B16A16_SFLOAT, //
                Formats::R16G16B16A16_UNORM, //
                Formats::R16G16B16A16_UINT, //
                Formats::R16G16B16A16_SNORM, //
                Formats::R16G16B16A16_SINT, //
                Formats::UNDEFINED, // r32g32 typeless
                Formats::R32G32_SFLOAT, //
                Formats::R32G32_UINT, //
                Formats::R32G32_SINT, //
                Formats::UNDEFINED, // r32g8x24 typeless
                Formats::UNDEFINED, // d32 float s8x24 uint
                Formats::UNDEFINED, // r32 float x8x24 uint
                Formats::UNDEFINED, // x32 typeless g8x24 uint
                Formats::UNDEFINED, // r10g10b10a2 typeless
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
                Formats::B8G8R8A8_SNORM, //
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
            return ret;
        }

        ImageRefPtr CImageManager::GetImage(const ImageHandle& hImg)
        {
            CImage* pImg;
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

    } // Core
} // VKE