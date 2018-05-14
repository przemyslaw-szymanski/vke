#include "RenderSystem/Managers/CImageManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        CImageManager::CImageManager()
        {
            m_LibRaw.open_file("test.raw");
        }
    } // RenderSystem
} // VKE