#pragma once

#include "RenderSystem/Common.h"

#include "libraw/libraw.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CImageManager
        {
            public:
                CImageManager();
            LibRaw m_LibRaw;
        };
    } // RenderSystem
} // VKE