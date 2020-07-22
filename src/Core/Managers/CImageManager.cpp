#include "Core/Managers/CImageManager.h"
#include "Core/Managers/CFileManager.h"

namespace VKE
{
    namespace Core
    {
        ImageHandle CImageManager::LoadImage(const SLoadImageInfo& Info)
        {
            ImageHandle hRet = INVALID_HANDLE;
            FilePtr pFile = m_pFileMgr->LoadFile( Info.File );

            return hRet;
        }
    } // Core
} // VKE