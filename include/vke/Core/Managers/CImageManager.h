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
            ImageHandle hSrcImage;
            ExtentU32   SrcOffset; // pixel offset in src image
            ExtentU32   DstSize; // num of pixels to copy
        };

        struct SCopyImageInfo
        {
            ImageHandle hSrcImage;
            ImageHandle hDstImage;
            ExtentU32   SrcSize; // number of src image pixels to copy
            ExtentU32   Offset; // number of src image pixels offset
            ExtentU32   DstOffset; // number of dst image pixels offset
        };

        struct SImageRegion
        {
            ExtentU32   Size;
            ExtentU32   Offset;
        };

        struct SSliceImageInfo
        {
            using ImageRegionArray = Utils::TCDynamicArray< SImageRegion >;
            ImageHandle         hSrcImage;
            ImageRegionArray    vRegions;
        };

        struct SImageDataDesc
        {
            ExtentU32       Size;
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

            protected:

                ImageBuffer             m_Buffer;
                Memory::CFreeListPool   m_MemoryPool;
                CFileManager*           m_pFileMgr;
        };
    } // Core
} // VKE
