#pragma once

#include "Core/VKECommon.h"
#include "Core/Resources/CImage.h"
#include "Core/Utils/TCDynamicArray.h"

#include "Core/Managers/CResourceManager.h"
#include "Core/Memory/TCFreeListManager.h"
#include "Core/Threads/ITask.h"
#include "Core/Threads/CTaskGroup.h"

namespace VKE
{
    namespace Core
    {
        class CFileManager;
        class CFile;

        using ImageSize = TSExtent<image_dimm_t>;

        struct SLoadImageInfo
        {
            SLoadFileInfo   LoadInfo;
        };

        struct SImageManagerDesc
        {
            Core::CFileManager* pFileMgr = nullptr;
            uint32_t            maxImageCount = 2048;
        };

        struct SCreateCopyImageInfo
        {
            ImageHandle     hSrcImage;
            PIXEL_FORMAT    dstFormat = PixelFormats::UNDEFINED;
            ImageSize       SrcOffset; // pixel offset in src image
            ImageSize       DstSize; // num of pixels to copy
        };

        struct SCopyImageInfo
        {
            ImageHandle hSrcImage;
            ImageHandle hDstImage;
            ImageSize   SrcSize; // number of src image pixels to copy
            ImageSize   Offset; // number of src image pixels offset
            ImageSize   DstOffset; // number of dst image pixels offset
        };

        struct SImageRegion
        {
            ImageSize   Size;
            ImageSize   Offset;
        };

        struct SSliceImageInfo
        {
            using SRegion = SImageRegion;
            using ImageRegionArray = Utils::TCDynamicArray< SImageRegion >;
            ImageHandle         hSrcImage;
            ImageRegionArray    vRegions;
            cstr_t              pSavePath = nullptr;
            cstr_t              pSaveName = nullptr;
        };

        struct SImageDataDesc
        {
            ImageSize       Size;
            PIXEL_FORMAT    format;
            uint32_t        rowPitch;
            uint32_t        slicePitch;
            uint8_t*        pPixels = nullptr;
        };

        struct SSaveImageInfo
        {
            ImageHandle         hImage = INVALID_HANDLE;
            SImageDataDesc*     pData = nullptr;
            IMAGE_FILE_FORMAT   format;
            cstr_t              pFileName;
        };

        class VKE_API CImageManager
        {
            friend class CVkEngine;
            friend class CImage;
            using ImageHandleType = uint32_t;
            using ImagePool = Utils::TSFreePool< ImagePtr, ImageHandleType, 1 >;
            using ImageBuffer = Core::TSVectorResourceBuffer< CImage*, CImage* >;

            public:

                CImageManager();
                ~CImageManager();

                ImageHandle         Load(const SLoadFileInfo& Info);
                ImageRefPtr         GetImage( const ImageHandle& hImg );
                void                DestroyImage( ImageHandle* phImg );

                // Create new image and copy pixels
                ImageHandle         Copy(const SCreateCopyImageInfo& Info);
                Result              Copy(const SCopyImageInfo& Info);

                Result              Resize(const ImageHandle& hImg, image_dimm_t width, image_dimm_t height);

                Result              Slice(const SSliceImageInfo& Info, ImageHandle* pOut);

                ImageHandle         CreateNormalMap(const ImageHandle& hSrcImg);

                Result              Save(const SSaveImageInfo& Info);

                Result              Resize(const ImageSize& NewSize, ImageHandle* pInOut);
                Result              Resize(const ImageSize& NewSize, ImagePtr* ppInOut);

            protected:

                Result          _Create(const SImageManagerDesc&);

                void            _Destroy();

                void            _DestroyImage( Core::CImage** ppImgInOut );

                Result          _CreateImage(CFile* pFile, CImage** ppInOut);
                Result          _CreateImage(const hash_t& hash, CImage** ppOut);
                void            _FreeImage(CImage*);

                Result          _InitDevIL();
                Result          _InitDirectXTex();

                Result          _CreateDirectXTexImage(const CFile* pFile, CImage** ppInOut);

                IMAGE_FILE_FORMAT    _DetermineFileFormat(const CFile* pFile) const;

                void             _GetTextureDesc(const CImage* pImg, RenderSystem::STextureDesc* pOut) const;

                Result          _Resize(const ImageSize& NewSize, CImage** ppInOut);

            protected:

                CImage*         _Copy( const SCreateCopyImageInfo& );
                Result          _SliceCompressed( CImage* pSrcImg, const SSliceImageInfo&, ImageHandle* );
                Result          _Decompress( CImage* pImg, void** ppOut );

            protected:

                Threads::SyncObject     m_SyncObj;
                ImageBuffer             m_Buffer;
                Memory::CFreeListPool   m_MemoryPool;
                CFileManager*           m_pFileMgr;
        };
    } // Core
} // VKE
