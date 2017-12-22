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
                m_Data.vBuffer.Clear();
                m_Data.pData = nullptr;
                m_Data.dataSize = 0;
                m_pFileExtension = nullptr;
                if( this->GetRefCount() == 0 )
                {
                    m_pMgr->_FreeFile( this );
                }
            }
        }

        Result CFile::Init(const SFileDesc& Desc)
        {
            m_Desc = Desc;
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
            if( !m_Data.vBuffer.IsEmpty() )
            {
                pData = &m_Data.vBuffer[ 0 ];
            }
            else
            {
                pData = m_Data.pData;
            }
            return pData;
        }

        uint32_t CFile::GetDataSize() const
        {
            uint32_t size = 0;
            if( !m_Data.vBuffer.IsEmpty() )
            {
                size = m_Data.vBuffer.GetCount();
            }
            else
            {
                size = m_Data.dataSize;
            }
            return size;
        }

        hash_t CFile::CalcHash(const SFileDesc& Desc)
        {
            hash_t h1 = CResource::CalcHash( Desc.Base );
            return h1;
        }
        
    } // Resources
} // VKE