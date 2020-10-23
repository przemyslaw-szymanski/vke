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

        class VKE_API CCommandBufferBatch
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

                CCommandBufferBatch() { }
                CCommandBufferBatch(const CCommandBufferBatch& Other) :
                    m_vpCommandBuffers(Other.m_vpCommandBuffers),
                    m_vDDICommandBuffers(Other.m_vDDICommandBuffers),
                    m_vDDIWaitSemaphores(Other.m_vDDIWaitSemaphores),
                    m_hDDISignalSemaphore(Other.m_hDDISignalSemaphore),
                    m_hDDIFence(Other.m_hDDIFence),
                    m_pMgr(Other.m_pMgr),
                    m_currCmdBuffer(Other.m_currCmdBuffer),
                    m_SyncObj(Other.m_SyncObj),
                    m_submitted(Other.m_submitted)
                { }
                CCommandBufferBatch(CCommandBufferBatch&& Other) :
                    m_vpCommandBuffers( std::move( Other.m_vpCommandBuffers)),
                    m_vDDICommandBuffers(std::move(Other.m_vDDICommandBuffers)),
                    m_vDDIWaitSemaphores(std::move(Other.m_vDDIWaitSemaphores)),
                    m_hDDISignalSemaphore(Other.m_hDDISignalSemaphore),
                    m_hDDIFence(Other.m_hDDIFence),
                    m_pMgr(Other.m_pMgr),
                    m_currCmdBuffer(Other.m_currCmdBuffer),
                    m_SyncObj(Other.m_SyncObj),
                    m_submitted(Other.m_submitted)
                {
                }

                CCommandBufferBatch& operator=(const CCommandBufferBatch& Other)
                {
                    m_hDDIFence = Other.m_hDDIFence;

                    m_vDDIWaitSemaphores = Other.m_vDDIWaitSemaphores;
                    m_hDDISignalSemaphore = Other.m_hDDISignalSemaphore;
                    m_vpCommandBuffers = Other.m_vpCommandBuffers;
                    m_vDDICommandBuffers = Other.m_vDDICommandBuffers;

                    m_pMgr = Other.m_pMgr;
                    m_currCmdBuffer = Other.m_currCmdBuffer;

                    m_submitted = Other.m_submitted;
                    return *this;
                }

                CCommandBufferBatch& operator =(CCommandBufferBatch&& Other)
                {
                    m_hDDIFence = Other.m_hDDIFence;

                    m_vDDIWaitSemaphores = std::move(Other.m_vDDIWaitSemaphores);
                    m_hDDISignalSemaphore = Other.m_hDDISignalSemaphore;
                    m_vpCommandBuffers = std::move(Other.m_vpCommandBuffers);
                    m_vDDICommandBuffers = std::move(Other.m_vDDICommandBuffers);

                    m_pMgr = Other.m_pMgr;
                    m_currCmdBuffer = Other.m_currCmdBuffer;

                    m_submitted = Other.m_submitted;
                    return *this;
                }

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
                void    _Submit( CCommandBuffer* pCb );
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
            CDeviceContext* pCtx = nullptr;
        };

        class VKE_API CSubmitManager
        {
            friend class CGraphicsContext;
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

                CCommandBufferBatch* _GetNextBatch( CDeviceContext* pCtx, const handle_t& hCmdPool );

                CCommandBufferBatch* GetCurrentBatch( CDeviceContext* pCtx, const handle_t& hCmdPool )
                {
                    Threads::ScopedLock l( m_CurrentBatchSyncObj );
                    return _GetCurrentBatch( pCtx, hCmdPool );
                }

                void SignalSemaphore( DDISemaphore* phDDISemaphoreOut );
                void SetWaitOnSemaphore( const DDISemaphore& hSemaphore );

                Result                  ExecuteCurrentBatch( CDeviceContext* pCtx, QueuePtr pQueue, CCommandBufferBatch** ppOut );
                Result                  ExecuteBatch( CDeviceContext* pCtx, QueuePtr pQueue, CCommandBufferBatch** ppInOut );
                CCommandBufferBatch*    FlushCurrentBatch( CDeviceContext* pCtx, const handle_t& hCmdPool );
                Result                  WaitForBatch( CDeviceContext* pCtx, const uint64_t& timeout, CCommandBufferBatch* pBatch );

                void                    Submit( CDeviceContext* pCtx, const handle_t& hCmdPool, CCommandBuffer* pCb )
                {
                    Threads::ScopedLock l( m_CurrentBatchSyncObj );
                    _Submit( pCtx, hCmdPool, pCb );
                }

            protected:

                CCommandBufferBatch * _GetCurrentBatch( CDeviceContext* pCtx, const handle_t& hCmdPool );
                void _Submit( CDeviceContext* pCtx, const handle_t& hCmdPool, CCommandBuffer* pCb );
                Result _Submit( CDeviceContext* pCtx, QueuePtr pQueue, CCommandBufferBatch* pSubmit );
                void _FreeCommandBuffers( CDeviceContext* pCtx, const handle_t& hPool, CCommandBufferBatch* pSubmit);
                //void _CreateCommandBuffers(CCommandBufferBatch* pSubmit, uint32_t count);
                void _CreateSubmits( CDeviceContext* pCtx, uint32_t count );
                CCommandBufferBatch* _GetNextSubmit( CDeviceContext* pCtx, const handle_t& hCmdPool );
                CCommandBufferBatch* _GetNextSubmitFreeSubmitFirst( CDeviceContext* pCtx, const handle_t& hCmdPool );
                CCommandBufferBatch* _GetNextSubmitReadySubmitFirst( CDeviceContext* pCtx, const handle_t& hCmdPool );
                CCommandBufferBatch* _GetSubmit( CDeviceContext* pCtx, const handle_t& hCmdPool, uint32_t idx );

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
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER