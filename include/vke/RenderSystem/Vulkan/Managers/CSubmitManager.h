#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CQueue.h"
#include "Core/Utils/TCFifo.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
        class CSubmitManager;
        class CCommandBuffer;

        class CCommandBufferBatch
        {
            friend class CSubmitManager;
            friend class CCommandBuffer;

            public:

                void operator=(const CCommandBufferBatch& Other);
                //VkCommandBuffer GetCommandBuffer() { return m_vCommandBuffers[m_currCmdBuffer++]; }
                const DDISemaphore& GetSignaledSemaphore() const { return m_hDDISignalSemaphore; }
                void WaitOnSemaphore( const DDISemaphore& hDDISemaphore ) { m_hDDIWaitSemaphore = ( hDDISemaphore ); }

                bool CanSubmit() const;

            private:

                void _Clear();
                void _Submit( CommandBufferPtr pCb );

            private:
                // Max 10 command buffers per one submit
                static const uint16_t DEFAULT_COMMAND_BUFFER_COUNT = 16;
                using CommandBufferArray = Utils::TCDynamicArray< CommandBufferPtr, DEFAULT_COMMAND_BUFFER_COUNT >;
                using DDICommandBufferArray = Utils::TCDynamicArray< DDICommandBuffer, DEFAULT_COMMAND_BUFFER_COUNT >;
                using DDISemaphoreArray = Utils::TCDynamicArray< DDISemaphore, DEFAULT_COMMAND_BUFFER_COUNT >;

                CommandBufferArray      m_vCommandBuffers;
                DDICommandBufferArray   m_vDDICommandBuffers;
                DDISemaphore            m_hDDIWaitSemaphore = DDI_NULL_HANDLE;
                DDISemaphore            m_hDDISignalSemaphore = DDI_NULL_HANDLE;
                DDIFence                m_hDDIFence = DDI_NULL_HANDLE;
                //DDISemaphore            m_hDDIWaitSemaphore = DDI_NULL_HANDLE;
                //DDISemaphore            m_hDDISignalSemaphore = DDI_NULL_HANDLE;
                CSubmitManager*         m_pMgr = nullptr;
                uint8_t                 m_currCmdBuffer = 0;
                //uint8_t                 m_submitCount = 0;
                Threads::SyncObject     m_SyncObj;
                bool                    m_submitted = false;
        };

        struct SSubmitManagerDesc
        {
            QueuePtr    pQueue;
            handle_t    hCmdBufferPool = 0;
        };

        class VKE_API CSubmitManager
        {
            friend class CGraphicsContext;
            friend class CCommandBufferBatch;
            friend class CCommandBuffer;

            static const uint32_t SUBMIT_COUNT = 32;

            using SubmitArray = Utils::TCDynamicArray< CCommandBufferBatch, SUBMIT_COUNT >;
            using SubmitPtrArray = Utils::TCDynamicArray< CCommandBufferBatch*, SUBMIT_COUNT >;
            using SubmitIdxQueue = Utils::TCFifo< CCommandBufferBatch*, SUBMIT_COUNT * 4 >;
            using BatchPtrArray = Utils::TCDynamicArray< CCommandBufferBatch*, SUBMIT_COUNT >;

            struct SCommandBufferBatchBuffer
            {
                SubmitArray     vSubmits;
                SubmitIdxQueue  qpSubmitted;
                uint32_t        currSubmitIdx = 0;
            }; 

            public:

                CSubmitManager(CDeviceContext* pCtx);
                ~CSubmitManager();

                Result Create(const SSubmitManagerDesc& Desc);
                void Destroy();

                CCommandBufferBatch* _GetNextBatch();
             
                CCommandBufferBatch* GetCurrentBatch();

                void SignalSemaphore( DDISemaphore* phDDISemaphoreOut );
                void SetWaitOnSemaphore( const DDISemaphore& hSemaphore );

                Result                  ExecuteCurrentBatch( CCommandBufferBatch** ppOut );
                Result                  ExecuteBatch( CCommandBufferBatch** ppInOut );
                CCommandBufferBatch*    FlushCurrentBatch();

            protected:

                void _Submit( CCommandBuffer* pCb );
                Result _Submit( CCommandBufferBatch* pSubmit );
                void _FreeCommandBuffers(CCommandBufferBatch* pSubmit);
                //void _CreateCommandBuffers(CCommandBufferBatch* pSubmit, uint32_t count);
                void _CreateSubmits(uint32_t count);
                CCommandBufferBatch* _GetNextSubmit();
                CCommandBufferBatch* _GetNextSubmitFreeSubmitFirst();
                CCommandBufferBatch* _GetNextSubmitReadySubmitFirst();
                CCommandBufferBatch* _GetSubmit(uint32_t idx);

            protected:

                SCommandBufferBatchBuffer   m_CommandBufferBatches;
                BatchPtrArray               m_vpPendingBatches;
                CCommandBufferBatch*        m_pCurrBatch = nullptr;
                CDeviceContext*             m_pCtx;
                SSubmitManagerDesc          m_Desc;
                DDISemaphore                m_hDDIWaitSemaphore = DDI_NULL_HANDLE;
                Threads::SyncObject         m_CurrentBatchSyncObj;
                bool                        m_signalSemaphore = true;
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER