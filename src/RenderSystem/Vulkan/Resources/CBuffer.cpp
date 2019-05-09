#include "RenderSystem/Vulkan/Resources/CBuffer.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Managers/CBufferManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        CBuffer::CBuffer( CBufferManager* pMgr ) :
            m_pMgr( pMgr )
        {

        }

        CBuffer::~CBuffer()
        {
            //Destroy();
        }

        void CBuffer::_Destroy()
        {
            CBuffer* pThis = this;
            this->m_hObject = NULL_HANDLE;
        }

        Result CBuffer::Init( const SBufferDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            // Note m_Desc.size will be changed if Desc.backBuffering is set
            // or buffer is used as uniform buffer
            m_chunkSize = m_Desc.size;

            m_ResourceBindingInfo.count = 1;
            m_ResourceBindingInfo.index = 0;
            m_ResourceBindingInfo.offset = 0;
            m_ResourceBindingInfo.range = m_chunkSize;

            return ret;
        }

        uint32_t GetNextIndexInRingBuffer(const uint32_t currIdx, const uint32_t count)
        {
            uint32_t ret = (currIdx + 1) % count;
            return ret;
        }

        uint32_t CBuffer::SetChunk( uint32_t idx )
        {
            VKE_ASSERT( idx < m_Desc.chunkCount, "" );
            m_ResourceBindingInfo.offset = idx * m_chunkSize;
            m_currentChunk = idx;
            return m_ResourceBindingInfo.offset;
        }

        uint32_t CBuffer::SetNextChunk()
        {
            m_currentChunk = GetNextIndexInRingBuffer( m_currentChunk, m_Desc.chunkCount );
            SetChunk( m_currentChunk );
            return m_ResourceBindingInfo.offset;
        }

        hash_t CBuffer::CalcHash( const SBufferDesc& Desc )
        {
            SHash Hash;
            Hash.Combine( Desc.size, Desc.usage );
            return Hash.value;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER