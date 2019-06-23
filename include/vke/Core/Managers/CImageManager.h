#pragma once

#include "Core/VKECommon.h"
#include "Core/Resources/CImage.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace Core
    {
        class CFileManager;

        struct SLoadImageInfo
        {
            SCreateResourceDesc Create;
            SImageDesc          Image;
        };

        struct SImageManagerDesc
        {

        };

        class VKE_API CImageManager
        {
            using ImageHandleType = uint32_t;
            using ImagePool = Utils::TSFreePool< ImagePtr, ImageHandleType, 1 >;

            public:

                ImageHandle         LoadImage(const SLoadImageInfo& Info);
                ImageRefPtr         GetImage( const ImageHandle& hImg );
                void                DestroyImage( ImageHandle* phImg );

            protected:

                Result          _Create(const SImageManagerDesc&);
                void            _Destroy();

                void            _DestroyImage( Core::CImage** ppImgInOut );

            protected:

                CFileManager*   m_pFileMgr;
        };
    } // Core
} // RenderSystem