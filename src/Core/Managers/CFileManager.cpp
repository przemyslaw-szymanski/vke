#include "Core/Managers/CFileManager.h"
#include "Core/Platform/CPlatform.h"

namespace VKE
{
    namespace Core
    {
        CFileManager::CFileManager(/*CVKEngine* pEngine*/) :
            m_pEngine{ nullptr }
        {}

        CFileManager::~CFileManager()
        {
            Destroy();
            m_pEngine = nullptr;
        }

        void CFileManager::Destroy()
        {
            for( uint32_t i = 0; i < m_FileBuffer.Buffer.vPool.GetCount(); ++i )
            {
                CFile* pFile = m_FileBuffer.Buffer.vPool[ i ];
                Memory::DestroyObject( &m_FileAllocator, &pFile );
            }
            m_FileBuffer.Clear();
        }

        Result CFileManager::Create(const SFileManagerDesc& Desc)
        {
            Result res = VKE_OK;
            m_Desc = Desc;
            if( VKE_SUCCEEDED( res = m_FileAllocator.Create( Desc.maxFileCount, sizeof( CFile ), 1 ) ) )
            {

            }
            return res;
        }

        FilePtr CFileManager::Load(const SFileDesc& Desc)
        {
            FilePtr pFile = _CreateFile( Desc );
            pFile->m_Desc = Desc;
            if( VKE_SUCCEEDED( _LoadFromFile( &pFile ) ) )
            {

            }
            else
            {
                pFile->Release();
            }
            return pFile;
        }

        FilePtr CFileManager::_CreateFile(const SFileDesc& Desc)
        {
            CFile* pFile = nullptr;
            Threads::ScopedLock l( m_SyncObj );
            hash_t hash = CFile::CalcHash( Desc );
            m_FileBuffer.Get(hash, )
            /*if( !m_FileBuffer.vFreeElements.PopBack( &pFile ) )
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_FileAllocator, &pFile, this ) ) )
                {
                    m_FileBuffer.vPool.PushBack( pFile );
                }
                else
                {
                    VKE_LOG_ERR("Unable to create memory for CFile object.");
                }
            }*/
            return FilePtr( pFile );
        }

        Result CFileManager::_LoadFromFile(FilePtr* pInOut)
        {
            Result res = VKE_FAIL;
            CFile* pFile = ( *pInOut ).Get();
            const auto& Desc = pFile->m_Desc.Base;
            VKE_ASSERT( Desc.pFileName, "FileName must be valid existing file name." );
            handle_t hFile = Platform::File::Open( Desc.pFileName, Platform::File::Modes::READ );
            if( hFile != 0 )
            {
                SFileInitInfo Info;
                uint32_t dataSize = Platform::File::GetSize( hFile );
                if( Info.Buffer.Resize( dataSize + 1 ) )
                {
                    Platform::File::SReadData Data;
                    Data.pData = &Info.Buffer[ 0 ];
                    Data.readByteCount = dataSize;
                    uint32_t readByteCount = Platform::File::Read( hFile, &Data );
                    if( readByteCount == dataSize )
                    {
                        Info.Buffer[ dataSize ] = 0;
                        pFile->Init( Info );
                        res = VKE_OK;
                    }
                    else
                    {
                        VKE_LOG_ERR("Unable to read whole file: " << Desc.pFileName);
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to resize SFileInitInfo::Buffer." );
                }
                Platform::File::Close( &hFile );
            }
            
            return res;
        }

        void CFileManager::_FreeFile(CFile* pFile)
        {
            {
                Threads::ScopedLock l( m_SyncObj );
                m_FileBuffer.vFreeElements.PushBack( pFile );
            }
        }
    }
}