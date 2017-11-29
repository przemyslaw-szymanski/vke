#include "Core/Resources/CFile.h"

namespace VKE
{
    namespace Resources
    {
        void CFile::Release()
        {
            m_Buffer.Clear();
            m_pData = nullptr;
            m_dataSize = 0;
        }

        Result CFile::Init(const SFileInitInfo& Info)
        {
            m_InitInfo = Info;
            return VKE_OK;
        }

        
    } // Resources
} // VKE