#include "Core/Resources/CFile.h"
#include "Core/Managers/CFileManager.h"

namespace VKE
{
    namespace Core
    {
        CFile::CFile( CFileManager* pMgr ) : m_pMgr{ pMgr }
        {
        }

        CFile::~CFile()
        {
        }

        void CFile::operator delete( void* pFile )
        {
            CFile* pThis = static_cast< CFile* >( pFile );
            VKE_ASSERT2( pThis != nullptr, "Invalid pointer." );
            pThis->Release();
        }

        void CFile::Release()
        {
            // if( m_InitInfo.pData || !m_InitInfo.Buffer.IsEmpty() )
            {
                m_Data.vBuffer.Clear();
                m_Data.pData     = nullptr;
                m_Data.dataSize  = 0;
                m_pFileExtension = nullptr;
                if( this->GetRefCount() == 0 )
                {
                    m_pMgr->_FreeFile( this );
                }
            }
        }

        Result CFile::Init( const SFileInfo& Desc )
        {
            m_Desc = Desc;
            VKE_ASSERT2( m_Desc.FileName.IsEmpty() == false, "File name must be set." );

#if defined( VKE_DEFAULT_DATA_DIR )
            // Relative paths: prefer CWD, fall back to the development data directory.
            if( !Platform::File::Exists( m_Desc.FileName.GetData() ) )
            {
                char pPath[ Config::Resource::MAX_NAME_LENGTH ];
                vke_sprintf( pPath, sizeof( pPath ), "%s/%s", VKE_DEFAULT_DATA_DIR, m_Desc.FileName.GetData() );
                if( Platform::File::Exists( pPath ) )
                {
                    m_Desc.FileName = pPath;
                }
            }
#endif

            m_pFileExtension = strrchr( m_Desc.FileName.GetData(), '.' );
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

        hash_t CFile::CalcHash( const SFileInfo& Desc )
        {
            hash_t h1 = CResource::CalcHash( Desc );
            return h1;
        }

    } // namespace Core
} // namespace VKE