#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class CSubmitManager;

        class CSubmit
        {
            friend class CSubmitManager;

            public:
                
                void Submit(const VkCommandBuffer& vkCb);

                //VkCommandBuffer GetCommandBuffer() { return m_vCommandBuffers[m_currCmdBuffer++]; }
                const VkSemaphore& GetSignaledSemaphore() const { return m_vkSignalSemaphore; }

            private:

                void _Clear();

            private:
                // Max 10 command buffers per one submit
                using CommandBufferArray = Utils::TCDynamicArray< VkCommandBuffer, 10 >;
                CommandBufferArray  m_vCommandBuffers;
                VkFence             m_vkFence = VK_NULL_HANDLE;
                VkSemaphore         m_vkWaitSemaphore = VK_NULL_HANDLE;
                VkSemaphore         m_vkSignalSemaphore = VK_NULL_HANDLE;
                CSubmitManager*     m_pMgr = nullptr;
                uint8_t             m_currCmdBuffer = 0;
                uint8_t             m_submitCount = 0;
                bool                m_submitted = false;
        };

        struct SSubmitManagerDesc
        {
        };

        class CSubmitManager
        {
            friend class CGraphicsContext;
            friend class CSubmit;

            static const uint32_t SUBMIT_COUNT = 8;

            using SubmitArray = Utils::TCDynamicArray< CSubmit, SUBMIT_COUNT >;
            using SubmitPtrArray = Utils::TCDynamicArray< CSubmit*, SUBMIT_COUNT >;

            struct SSubmitBuffer
            {
                SubmitArray     vSubmits;
                uint32_t        currSubmitIdx = 0;
            }; 

            public:

                CSubmitManager(CGraphicsContext* pCtx);
                ~CSubmitManager();

                Result Create(const SSubmitManagerDesc& Desc);
                void Destroy();

                CSubmit* GetNextSubmit(uint32_t cmdBufferCount, const VkSemaphore& vkWaitSemaphore);

            protected:

                void _Submit(CSubmit* pSubmit);
                void _FreeCommandBuffers(CSubmit* pSubmit);
                void _CreateCommandBuffers(CSubmit* pSubmit, uint32_t count);
                void _CreateSubmits(uint32_t count);

            protected:

                SSubmitBuffer       m_Submits;
                CSubmit*            m_pCurrSubmit = nullptr;
                CGraphicsContext*   m_pCtx;
                Vulkan::Queue       m_pQueue = nullptr;
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER