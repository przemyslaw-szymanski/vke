#include "RenderSystem/Vulkan/CRenderQueue.h"
#include "RenderSystem/Vulkan/CDevice.h"
#include "RenderSystem/Vulkan/CCommandBuffer.h"
#include "Core/VKEForwardDeclarations.h"

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
            for( uint32_t i = 0; i < COMMAND_BUFFER_COUNT; ++i )
            {
                m_pDevice->DestroyCommandBuffer( &m_pCmdBuffers[ i ] );
            }
        }

        Result CRenderQueue::Create(const SGraphicsQueueInfo& Info)
        {
            m_Info = Info;
            for( uint32_t i = 0; i < COMMAND_BUFFER_COUNT; ++i )
            {
                m_pCmdBuffers[ i ] = m_pDevice->CreateCommandBuffer();
            }
            return VKE_OK;
        }

        void CRenderQueue::Begin()
        {
            GetCommandBuffer()->Begin();
        }

        void CRenderQueue::Draw()
        {
            // For each is visible object
            // size_t drawcallCount = m_vDrawcalls.size();
            // for(uint32_t i = 0; i < drawcallCount; ++i)
        }

        void CRenderQueue::End()
        {
            GetCommandBuffer()->End();
        }

        void CRenderQueue::Submit()
        {
            GetCommandBuffer()->Submit();
            GetNextCommandBuffer();
        }

        CommandBufferPtr CRenderQueue::GetNextCommandBuffer()
        {
            m_currCmdBuffId = (m_currCmdBuffId+1) % COMMAND_BUFFER_COUNT;
            return GetCommandBuffer();
        }

    } // RenderSystem
} // VKE