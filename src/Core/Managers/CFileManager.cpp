#include "Core/Managers/CFileManager.h"

namespace VKE
{
    namespace Core
    {
        CFileManager::CFileManager(CVKEngine* pEngine) :
            m_pEngine{ pEngine }
        {}

        CFileManager::~CFileManager()
        {
            Destroy();
            m_pEngine = nullptr;
        }

        void CFileManager::Destroy()
        {

        }

        Result CFileManager::Create(const SFileManagerDesc& Desc)
        {
            Result res = VKE_OK;
            m_Desc = Desc;
            return res;
        }

        FilePtr CFileManager::Load(cstr_t pPath)
        {
            FilePtr pFile;
            return pFile;
        }
    }
}