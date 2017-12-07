#include "Core/Resources/CFile.h"
#include "Core/Managers/CFileManager.h"

namespace VKE
{
    namespace Resources
    {
        CFile::CFile(CFileManager* pMgr) :
            CResource( 0 )
            , m_pMgr{ pMgr }
        {

        }

        CFile::~CFile()
        {

        }

        void CFile::operator delete(void* pFile)
        {
            CFile* pThis = static_cast< CFile* >( pFile );
            VKE_ASSERT( pThis != nullptr, "Invalid pointer." );
            pThis->Release();
        }

        void CFile::Release()
        {
            //if( m_InitInfo.pData || !m_InitInfo.Buffer.IsEmpty() )
            {
                m_InitInfo.Buffer.Clear();
                m_InitInfo.pData = nullptr;
                m_InitInfo.dataSize = 0;
                m_pFileExtension = nullptr;
                //if( this->GetRefCount() == 0 )
                {
                    m_pMgr->_FreeFile( this );
                }
            }
        }

        Result CFile::Init(const SFileInitInfo& Info)
        {
            m_InitInfo = Info;
            VKE_ASSERT( m_Desc.Base.pFileName, "File name must be set." );
            m_pFileExtension = strrchr( m_Desc.Base.pFileName, '.' );
            if( m_pFileExtension )
            {
                m_pFileExtension = m_pFileExtension + 1;
            }
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