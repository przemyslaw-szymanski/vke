#pragma once
#if VKE_VULKAN_RENDER_SYSTEM
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
            friend class CQueue;
            friend class CContextBase;

            // Max 10 command buffers per one submit
            static const uint16_t DEFAULT_COMMAND_BUFFER_COUNT = 16;
            using CommandBufferArray = Utils::TCDynamicArray< CCommandBuffer*, DEFAULT_COMMAND_BUFFER_COUNT >;
            using DDICommandBufferArray = Utils::TCDynamicArray< DDICommandBuffer, DEFAULT_COMMAND_BUFFER_COUNT >;
            using DDISemaphoreArray = Utils::TCDynamicArray< DDISemaphore, DEFAULT_COMMAND_BUFFER_COUNT >;

            public:

                void operator=( const CCommandBufferBatch& Other );
                void operator=( CCommandBufferBatch&& Other );
                //VkCommandBuffer GetCommandBuffer() { return m_vCommandBuffers[m_currCmdBuffer++]; }
                const DDISemaphore& GetSignaledSemaphore() const { return m_hDDISignalSemaphore; }
                void WaitOnSemaphore( const DDISemaphore& hDDISemaphore )
                {
                    m_vDDIWaitSemaphores.PushBack( hDDISemaphore );
                }

                void WaitOnSemaphores( DDISemaphoreArray&& vDDISemaphores )
                {
                    m_vDDIWaitSemaphores.Append( vDDISemaphores );
                }

                bool CanSubmit() const;

            private:

                void    _Clear();
                Result  _Submit( CCommandBuffer* pCb );
                //Result  _Flush( const uint64_t& timeout );

            private:


                CommandBufferArray      m_vpCommandBuffers;
                DDICommandBufferArray   m_vDDICommandBuffers;
                DDISemaphoreArray       m_vDDIWaitSemaphores;
                DDISemaphore            m_hDDISignalSemaphore = DDI_NULL_HANDLE;
                DDIFence                m_hDDIFence = DDI_NULL_HANDLE;
                CSubmitManager*         m_pMgr = nullptr;
                uint8_t                 m_currCmdBuffer = 0;
                Threads::SyncObject     m_SyncObj;
                bool                    m_submitted = false;
        };

        struct SSubmitManagerDesc
        {
            CContextBase* pCtx = nullptr;
        };

        struct NextSubmitBatchAlgorithms
        {
            enum ALGORITHM
            {
                FIRST_READY,
                FIRST_FREE,
                _MAX_COUNT
            };
        };
        using NEXT_SUBMIT_BATCH_ALGORITHM = NextSubmitBatchAlgorithms::ALGORITHM;

        class VKE_API CSubmitManager
        {
            friend class CGraphicsContext;
            friend class CTransferContext;
            friend class CComputeContext;
            friend class CCommandBufferBatch;
            friend class CCommandBuffer;
            friend class CContextBase;
            friend class CQueue;

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

                CSubmitManager();
                ~CSubmitManager();

                Result Create(const SSubmitManagerDesc& Desc);
                void Destroy(CDeviceContext* pCtx);

                template<NEXT_SUBMIT_BATCH_ALGORITHM>
                CCommandBufferBatch* _GetNextBatch( CContextBase* pCtx, const handle_t& hCmdPool );

                CCommandBufferBatch* GetCurrentBatch( CContextBase* pCtx, const handle_t& hCmdPool )
                {
                    Threads::ScopedLock l( m_CurrentBatchSyncObj );
                    return _GetCurrentBatch( pCtx, hCmdPool );
                }

                void SignalSemaphore( DDISemaphore* phDDISemaphoreOut );
                void SetWaitOnSemaphore( const DDISemaphore& hSemaphore );

                Result ExecuteCurrentBatch( CContextBase* pCtx, QueuePtr pQueue, CCommandBufferBatch** ppOut );
                Result ExecuteBatch( CContextBase* pCtx, QueuePtr pQueue, CCommandBufferBatch** ppInOut );
                CCommandBufferBatch* FlushCurrentBatch( CContextBase* pCtx, const handle_t& hCmdPool );
                Result WaitForBatch( CContextBase* pCtx, const uint64_t& timeout, CCommandBufferBatch* pBatch );

                Result Submit( CContextBase* pCtx, const handle_t& hCmdPool, CCommandBuffer* pCb )
                {
                    Threads::ScopedLock l( m_CurrentBatchSyncObj );
                    return _Submit( pCtx, hCmdPool, pCb );
                }

            protected:

                CCommandBufferBatch* _GetCurrentBatch( CContextBase* pCtx, const handle_t& hCmdPool );
              Result _Submit( CContextBase* pCtx, const handle_t& hCmdPool, CCommandBuffer* pCb );
                Result _Submit( CContextBase* pCtx, QueuePtr pQueue, CCommandBufferBatch* pSubmit );
              void _FreeCommandBuffers( CContextBase* pCtx, const handle_t& hPool, CCommandBufferBatch* pSubmit );
                //void _CreateCommandBuffers(CCommandBufferBatch* pSubmit, uint32_t count);
              void _CreateSubmits( CContextBase* pCtx, uint32_t count );
                //template<NEXT_SUBMIT_BATCH_ALGORITHM>
                //CCommandBufferBatch*    _GetNextSubmit( CDeviceContext* pCtx, const handle_t& hCmdPool );
              CCommandBufferBatch* _GetNextSubmitFreeSubmitFirst( CContextBase* pCtx, const handle_t& hCmdPool );
              CCommandBufferBatch* _GetNextSubmitReadySubmitFirst( CContextBase* pCtx, const handle_t& hCmdPool );
              CCommandBufferBatch* _GetSubmit( CContextBase* pCtx, const handle_t& hCmdPool, uint32_t idx );
              void _FreeBatch( CContextBase* pCtx, const handle_t& hCmdPool, CCommandBufferBatch** ppInOut );

            protected:

                SCommandBufferBatchBuffer   m_CommandBufferBatches;
                BatchPtrArray               m_vpPendingBatches;
                CCommandBufferBatch*        m_pCurrBatch = nullptr;
                //SSubmitManagerDesc          m_Desc;
                DDISemaphore                m_hDDIWaitSemaphore = DDI_NULL_HANDLE;
                Threads::SyncObject         m_CurrentBatchSyncObj;
                bool                        m_signalSemaphore = true;
                bool                        m_waitForSemaphores = true;
        };

        /*template<NEXT_SUBMIT_BATCH_ALGORITHM Algorithm>
        CCommandBufferBatch* CSubmitManager::_GetNextSubmit(CDeviceContext* pCtx, const handle_t& hCmdPool)
        {
            CCommandBufferBatch* pRet;
            if constexpr(Algorithm == NextSubmitBatchAlgorithms::FIRST_READY)
            {
                pRet = _GetNextSubmitReadySubmitFirst(pCtx, hCmdPool);
            }
            else if constexpr(Algorithm == NextSubmitBatchAlgorithms::FIRST_FREE)
            {
                pRet = _GetNextSubmitFreeSubmitFirst(pCtx, hCmdPool);
            }
            return pRet;
        }*/

        template<NEXT_SUBMIT_BATCH_ALGORITHM Algorithm>
        CCommandBufferBatch* CSubmitManager::_GetNextBatch( CContextBase* pCtx, const handle_t& hCmdPool )
        {
            CCommandBufferBatch* pBatch = nullptr;
            {
                if constexpr(Algorithm == NextSubmitBatchAlgorithms::FIRST_READY)
                {
                    pBatch = _GetNextSubmitReadySubmitFirst(pCtx, hCmdPool);
                }
                else if constexpr(Algorithm == NextSubmitBatchAlgorithms::FIRST_FREE)
                {
                    pBatch = _GetNextSubmitFreeSubmitFirst(pCtx, hCmdPool);
                }
                assert(pBatch);
            }
            assert(pBatch && "No free submit batch left");
            pBatch->_Clear();

            pBatch->m_submitted = false;

            return pBatch;
        }
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDER_SYSTEM