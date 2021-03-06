#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/TCDynamicRingArray.h"
#include "Core/Utils/TCList.h"
#include "Core/Threads/Common.h"
#include "RenderSystem/Tasks/GraphicsContext.h"
#include "RenderSystem/CRenderTarget.h"
#include "Core/VKEForwardDeclarations.h"
#include "RenderSystem/Vulkan/Managers/CCommandBufferManager.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"
#include "RenderSystem/Vulkan/Wrappers/CCommandBuffer.h"
#include "RenderSystem/Vulkan/Managers/CPipelineManager.h"
#include "RenderSystem/CQueue.h"
#include "RenderSystem/CContextBase.h"
#include "RenderSystem/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CSwapChain;
        class CRenderQueue;
        class CDeviceContext;
        class CRenderTarget;
        class CResourceManager;
        class CSubmitManager;
        class CCommandBufferBatch;
        class CPipelineManager;

        namespace Managers
        {
            class CBackBufferManager;
        } // Managers

        struct SInternal;

        class VKE_API CGraphicsContext final : public CContextBase
        {
            friend class CRenderSystem;
            friend class CDeviceContext;
            friend class CSwapChain;
            friend class CRenderQueue;
            friend class CRenderTarget;
            friend class CResourceManager;
            friend class CSubmitManager;
            friend class CPipelineManager;
            friend struct Tasks::SGraphicsContext;
            friend class CDDI;
            struct SPrivate;

            static const uint32_t DEFAULT_CMD_BUFFER_COUNT = 32;
            using CommandBufferArray = Utils::TCDynamicArray< CommandBufferPtr, DEFAULT_CMD_BUFFER_COUNT >;
            using VkCommandBufferArray = Utils::TCDynamicArray< VkCommandBuffer, DEFAULT_CMD_BUFFER_COUNT >;
            using UintArray = Utils::TCDynamicArray< uint32_t, DEFAULT_CMD_BUFFER_COUNT >;
            //using SemaphoreArray = Utils::TCDynamicArray< VkSemaphore >;
            //using FenceArray = Utils::TCDynamicArray< VkFence >;
           // using SwapChainArray = Utils::TCDynamicArray< VkSwapchainKHR >;
            //using RenderQueueArray = Utils::TCDynamicArray< CRenderQueue* >;
            //using RenderTargetArray = Utils::TCDynamicArray< RenderTargetRefPtr >;

            template<class ResourceType>
            struct SResourceBuffer
            {
                using Map = vke_hash_map< handle_t, ResourceType >;
                Map         mBuffer;
                uint32_t    handleCounter = 0;
            };

            using RenderingPipelineBuffer = SResourceBuffer< CRenderingPipeline* >;

            struct ContextTasks
            {
                enum TASK : uint8_t
                {
                    BEGIN_FRAME,
                    END_FRAME,
                    PRESENT,
                    SWAP_BUFFERS,
                    _ENUM_COUNT
                };
            };
            using TASK = ContextTasks::TASK;
            using CurrentTask = TASK;

            enum class RenderState
            {
                NO_RENDER,
                BEGIN,
                END,
                PRESENT,
                SWAP_BUFFERS
            };

            struct SCommandBufferBatch
            {
                CommandBufferArray      vCmdBuffers;
                VkFence                 vkFence = VK_NULL_HANDLE;
                bool                    readyToExecute = false;

                void Reset()
                {
                    vCmdBuffers.Clear();
                    readyToExecute = false;
                }
            };
            using SubmitArray = Utils::TCDynamicArray< SCommandBufferBatch >;
            using SubmitList = std::list< SCommandBufferBatch >;

            //using SCommandBuffers = Utils::TSFreePool< VkCommandBuffer >;
            //using SFences = Utils::TSFreePool< VkFence >;
            //using SSemaphores = Utils::TSFreePool< VkSemaphore >;
            using SemaphoreArray = Utils::TCDynamicArray< DDISemaphore, 8 >;

            //using CommandBufferArrays = SCommandBuffers[ RenderQueueUsages::_MAX_COUNT ];

            /*struct SPresentData
            {
                SwapChainArray      vSwapChains;
                SemaphoreArray      vWaitSemaphores;
                UintArray           vImageIndices;
                Threads::SyncObject m_SyncObj;
                uint32_t            presentCount = 0;

                void Clear()
                {
                    vWaitSemaphores.Clear();
                    vImageIndices.Clear();
                    vSwapChains.Clear();
                    presentCount = 0;
                }
            };*/

            struct SExecuteData
            {
                //DDISemaphore            hDDISemaphoreBackBufferReady;
                SemaphoreArray          vWaitSemaphores;
                CCommandBufferBatch*    pBatch;
                uint32_t                ddiImageIndex;
            };
            //using ExecuteDataQueue = Utils::TCList< SExecuteData >;
            using ExecuteDataQueue = std::deque< SExecuteData >;

            public:

                CGraphicsContext(CDeviceContext* pCtx);
                CGraphicsContext( const CGraphicsContext& ) = delete;
                ~CGraphicsContext();

                Result                  Create(const SGraphicsContextDesc& Info);
                //void                    Destroy();

                void                    Resize(uint32_t width, uint32_t height);

                void                    RenderFrame();
                void                    FinishRendering();

                CCommandBuffer*         BeginFrame();
                void                    EndFrame();

                const VkICD::Device&    _GetICD() const;

                vke_force_inline
                CDeviceContext*         GetDeviceContext() const { return /*m_BaseCtx.*/m_pDeviceCtx; }

                CSwapChain*             GetSwapChain() const { return m_pSwapChain; }

                void                    SetEventListener(EventListeners::IGraphicsContext*);

                const
                SGraphicsContextDesc&   GetDesc() const { return m_Desc; }

                void                    Wait();

                QueueRefPtr             _GetQueue() { return /*m_BaseCtx.*/m_pQueue; }

                uint8_t                 GetBackBufferIndex() const { return /*m_BaseCtx.*/m_backBufferIdx; }

                Result                  ExecuteCommandBuffers( DDISemaphore* phDDISignalSemaphore );

                void                    SetTextureState( CSwapChain* pSwapChain, const TEXTURE_STATE& state );
                void                    SetTextureState( const TEXTURE_STATE& state, RenderTargetHandle* phRT ) { CContextBase::SetTextureState( state, phRT ); }
                void                    SetTextureState( const TEXTURE_STATE& state, TextureHandle* phTex ) { CContextBase::SetTextureState( state, phTex ); }

                void                    BindDefaultRenderPass();

            protected:

                void            _Destroy();

                void            _AddToPresent(CSwapChain*);


                TaskState       _RenderFrameTask();
                TaskState       _PresentFrameTask();
                TaskState       _SwapBuffersTask();
                TaskState       _ExecuteCommandBuffersTask();

                vke_force_inline
                void            _SetCurrentTask(TASK task);
                vke_force_inline
                TASK            _GetCurrentTask();

                VkInstance      _GetInstance() const;

                void            _WaitForFrameToFinish();

                /*template<typename ObjectT, typename VkStructT>
                ObjectT _CreateObject(const VkStructT& VkCreateInfo, Utils::TSFreePool< ObjectT >* pOut);
                template<typename ObjectBufferT>
                void _DestroyObjects( ObjectBufferT* pOut );*/

                CRenderingPipeline* _CreateRenderingPipeline(const SRenderingPipelineDesc& Desc);

                void            _ResizeSwapChainTask(uint32_t width, uint32_t height);

            protected:

                SGraphicsContextDesc        m_Desc;
                //CDeviceContext*             m_CommonCtx.pDeviceCtx = nullptr;
                //CDDI&                       m_DDI;
                //CContextBase                m_BaseCtx;
                CPipelineManager            m_PipelineMgr;
                //CSubmitManager              m_SubmitMgr;
                CSwapChain*                 m_pSwapChain = nullptr;
                SPresentData                m_PresentData;
                SPrivate*                   m_pPrivate = nullptr;
                Threads::SyncObject         m_SyncObj;
                EventListeners::IGraphicsContext*  m_pEventListener;
                Tasks::SGraphicsContext     m_Tasks;
                RenderingPipelineBuffer         m_RenderingPipelines;
                CRenderingPipeline*             m_pCurrRenderingPipeline = nullptr;
                CRenderingPipeline*             m_pDefaultRenderingPipeline = nullptr;
                //RenderTargetArray               m_vpRenderTargets;
                RenderState                     m_renderState = RenderState::NO_RENDER;
                uint16_t                        m_enabledRenderQueueCount = 0;
                std::atomic_bool                m_readyToPresent = false;
                std::atomic_bool                m_readyToExecute = false;
                //uint8_t                     m_currentBackBufferIdx = 0;
                //uint8_t                     m_prevBackBufferIdx = UNDEFINED_U8;
                std::atomic_bool                m_needQuit = false;
                std::atomic_bool                m_needBeginFrame = false;
                std::atomic_bool                m_needEndFrame = false;
                bool                        m_frameEnded = true;
                bool                        m_presentEnded = true;
                bool                        m_submitEnded = true;
                std::atomic_bool            m_needRenderFrame = false;
                bool                        m_stopRendering = false;
                CurrentTask                 m_CurrentTask = ContextTasks::BEGIN_FRAME;
                Threads::SyncObject         m_CurrentTaskSyncObj;
                //handle_t                    m_hCommandPool = INVALID_HANDLE;
                //VkCommandBuffer             m_vkCbTmp[ 2 ];
                //VkFence                     m_vkFenceTmp[2];
                //VkSemaphore                 m_vkSignals[ 2 ], m_vkWaits[2];
                CCommandBufferBatch*                    m_pTmpSubmit;
                uint32_t                    m_instnceId = 0;
                bool                        m_createdTmp = false;
                uint32_t                    m_currFrame = 0;

                SPresentInfo                m_PresentInfo;
                Threads::SyncObject         m_ExecuteQueueSyncObj;
                ExecuteDataQueue            m_qExecuteData;
        };

        void CGraphicsContext::_SetCurrentTask(TASK task)
        {
            m_CurrentTaskSyncObj.Lock();
            m_CurrentTask = task;
            m_CurrentTaskSyncObj.Unlock();
        }

        CGraphicsContext::TASK CGraphicsContext::_GetCurrentTask()
        {
            m_CurrentTaskSyncObj.Lock();
            CurrentTask t = m_CurrentTask;
            m_CurrentTaskSyncObj.Unlock();
            return t;
        }

    } // RenderSystem
} // vke
#endif // VKE_VULKAN_RENDERER