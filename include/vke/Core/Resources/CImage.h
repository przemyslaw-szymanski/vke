#pragma once

#include "CResource.h"

namespace VKE
{
    namespace Core
    {
        struct BitsPerPixel
        {
            enum BPP : uint8_t
            {
                BPP_8 = 8,
                BPP_16 = 16,
                BPP_32 = 32,
                BPP_64 = 64,
                BPP_128 = 128,
                _MAX_COUNT = 5
            };
        };
        using BITS_PER_PIXEL = BitsPerPixel::BPP;

        struct SImageDesc
        {
            ImageDimmension     Size;
            BITS_PER_PIXEL      bitsPerPixel;
        };

        class VKE_API CImage : public CResource
        {
            friend class CImageManager;

            public:

            protected:

                Result      _Init( const SImageDesc& );
                void        _Destroy();

            protected:

                SImageDesc  m_Desc;
                uint8_t*    m_pData = nullptr;
        };
    } // Resources
    namespace Core
    {
        using ImagePtr = Utils::TCWeakPtr< CImage >;
        using ImageRefPtr = Utils::TCObjectSmartPtr< CImage >;
    } // Core
} // VKE