#include "RenderSystem/Vulkan/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        CCommandBuffer::CCommandBuffer()
        {
        }

        CCommandBuffer::~CCommandBuffer()
        {
            Destroy();
        }

        void CCommandBuffer::Destroy()
        {

        }

        Result CCommandBuffer::Create(CDevice* pDevice, CCommandBufferManager* pMgr, const handle_t& handle,
                                      CCommandBuffer* pPrimary)
        {
            m_pDevice = pDevice;
            m_pManager = pMgr;
            m_handle = handle;
            m_state = State::CREATED;
            return VKE_OK;
        }

        void CCommandBuffer::Begin()
        {
            m_state = State::BEGIN;
        }

        void CCommandBuffer::End()
        {
            m_state = State::END;
        }

        void CCommandBuffer::Submit()
        {
            m_state = State::SUBMITED;
        }
    } // rendersystem
} // vke