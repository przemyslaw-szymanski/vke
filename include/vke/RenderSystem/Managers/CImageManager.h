#pragma once

#include "RenderSystem/Common.h"
#include "ThirdParty/DevIL/DevIL/include/IL/il.h"
//#include "libraw/libraw.h"

namespace VKE
{
    namespace RenderSystem
    {
        class VKE_API CImageManager
        {
            public:
                CImageManager();
                ~CImageManager();
            //LibRaw m_LibRaw;
        };
    } // RenderSystem
} // VKE