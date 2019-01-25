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
            Destroy();
        }

        void CBuffer::Destroy()
        {
            if( this->m_hObject != NULL_HANDLE )
            {
                _Destroy();
            }
        }

        void CBuffer::_Destroy()
        {
            CBuffer* pThis = this;
            m_pMgr->_DestroyBuffer( &pThis );
            this->m_hObject = NULL_HANDLE;
        }

        Result CBuffer::Init( const SBufferDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
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