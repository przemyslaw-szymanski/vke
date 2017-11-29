#include "Core/Resources/CFile.h"

namespace VKE
{
    namespace Resources
    {
        void CFile::Release()
        {
            m_InitInfo.Buffer.Clear();
            m_InitInfo.pData = nullptr;
            m_InitInfo.dataSize = 0;
        }

        Result CFile::Init(const SFileInitInfo& Info)
        {
            m_InitInfo = Info;
            return VKE_OK;
        }

        const CFile::DataType* CFile::GetData() const
        {
            const DataType* pData = nullptr;
            if( !m_InitInfo.Buffer.IsEmpty() )
            {
                pData = &m_InitInfo.Buffer[ 0 ];
            }
            else
            {
                pData = m_InitInfo.pData;
            }
            return pData;
        }

        uint32_t CFile::GetDataSize() const
        {
            uint32_t size = 0;
            if( !m_InitInfo.Buffer.IsEmpty() )
            {
                size = m_InitInfo.Buffer.GetCount();
            }
            else
            {
                size = m_InitInfo.dataSize;
            }
            return size;
        }
        
    } // Resources
} // VKE