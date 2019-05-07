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
            return ret;
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