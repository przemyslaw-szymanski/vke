#include "RenderSystem/CRenderQueue.h"
#include "RenderSystem/CDevice.h"
#include "RenderSystem/CCommandBuffer.h"
#include "VKEForwardDeclarations.h"

namespace VKE
{
    namespace RenderSystem
    {
        CRenderQueue::CRenderQueue(CDevice* pDev) :
            m_pDevice(pDev)
        {
            assert(m_pDevice);
        }

        CRenderQueue::~CRenderQueue()
        {
            Destroy();
        }

        void CRenderQueue::Destroy()
        {
            m_pDevice->DestroyCommandBuffer(&m_pCmdBuff);
        }

        Result CRenderQueue::Create(const SRenderQueueInfo& Info)
        {
            m_Info = Info;
            m_pCmdBuff = m_pDevice->CreateCommandBuffer();
            return VKE_OK;
        }

        void CRenderQueue::Begin()
        {
            m_pCmdBuff->Begin();
        }

        void CRenderQueue::End()
        {
            m_pCmdBuff->End();
        }

    } // RenderSystem
} // VKE