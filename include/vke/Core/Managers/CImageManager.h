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

            protected:

                Result          _Create(const SImageManagerDesc&);

                void            _Destroy();

                void            _DestroyImage( Core::CImage** ppImgInOut );

                Result          _CreateImage(CFile* pFile, CImage** ppInOut);
                void            _FreeImage(CImage*);

                Result          _InitDevIL();
                Result          _InitDirectXTex();

                Result          _CreateDirectXTexImage(const CFile* pFile, CImage** ppInOut);

                IMAGE_FILE_FORMAT    _DetermineFileFormat(const CFile* pFile) const;

            protected:

                ImageBuffer             m_Buffer;
                Memory::CFreeListPool   m_MemoryPool;
                CFileManager*           m_pFileMgr;
        };
    } // Core
} // VKE
