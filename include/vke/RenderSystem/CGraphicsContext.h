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
#include "RenderSystem/Vulkan/CCommandBufferManager.h"
#include "RenderSystem/Vulkan/CSubmitManager.h"
#include "RenderSystem/Vulkan/Wrappers/CCommandBuffer.h"

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
        class CSubmit;

        namespace Managers
        {
            class CBackBufferManager;
        } // Managers

        struct SInternal;

        class VKE_API CGraphicsContext
        {
            friend class CRenderSystem;
            friend class CDeviceContext;
            friend class CSwapChain;
            friend class CRenderQueue;
            friend class CRenderTarget;
            friend class CResourceManager;
            friend class CSubmitManager;
            friend struct Tasks::SGraphicsContext;
            struct SPrivate;

            static const uint32_t DEFAULT_CMD_BUFFER_COUNT = 32;
            using CommandBufferArray = Utils::TCDynamicRingArray< VkCommandBuffer, DEFAULT_CMD_BUFFER_COUNT >;
            using UintArray = Utils::TCDynamicArray< uint32_t, DEFAULT_CMD_BUFFER_COUNT >;
            using SemaphoreArray = Utils::TCDynamicArray< VkSemaphore >;
            using FenceArray = Utils::TCDynamicArray< VkFence >;
            using SwapChainArray = Utils::TCDynamicArray< VkSwapchainKHR >;
            using RenderQueueArray = Utils::TCDynamicArray< CRenderQueue* >;
            using RenderTargetArray = Utils::TCDynamicArray< RenderTargetRefPtr >;
            using RenderingPipelineVec = Utils::TCDynamicArray< CRenderingPipeline* >;

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

            struct SSubmit
            {
                CommandBufferArray      vCmdBuffers;
                VkFence                 vkFence = VK_NULL_HANDLE;
                bool                    readyToExecute = false;

                void Reset()
                {
                    vCmdBuffers.Clear<false>();
                    readyToExecute = false;
                }
            };
            using SubmitArray = Utils::TCDynamicArray< SSubmit >;
            using SubmitList = std::list< SSubmit >;

            using SCommandBuffers = Utils::TSFreePool< VkCommandBuffer >;
            using SFences = Utils::TSFreePool< VkFence >;
            using SSemaphores = Utils::TSFreePool< VkSemaphore >;

            using CommandBufferArrays = SCommandBuffers[ RenderQueueUsages::_MAX_COUNT ];

            struct SPresentData
            {
                SwapChainArray      vSwapChains;
                SemaphoreArray      vWaitSemaphores;
                UintArray           vImageIndices;
                Threads::SyncObject m_SyncObj;
                uint32_t            presentCount = 0;

                void Clear()
                {
                    vWaitSemaphores.Clear<false /*do not call dtor*/>();
                    vImageIndices.Clear<false /*do not call dtor*/>();
                    vSwapChains.Clear<false /*do not call dtor*/>();
                    presentCount = 0;
                }
            };

            public:

                CGraphicsContext(CDeviceContext* pCtx);
                ~CGraphicsContext();

                Result Create(const SGraphicsContextDesc& Info);
                void Destroy();

                void Resize(uint32_t width, uint32_t height);

                void RenderFrame();
                void FinishRendering();

                Result  CreateSwapChain(const SSwapChainDesc& Desc);

                const Vulkan::ICD::Device&    _GetICD() const;

                vke_force_inline
                CDeviceContext*        GetDeviceContext() const { return m_pDeviceCtx; }

                CRenderQueue* CreateRenderQueue(const SRenderQueueDesc&);
                Result ExecuteRenderQueue(CRenderQueue*);
                Result ExecuteRenderQueue(const handle_t&);

                Vulkan::Wrapper::CCommandBuffer CreateCommandBuffer()
                {
                    return Vulkan::Wrapper::CCommandBuffer( m_VkDevice.GetICD(), _CreateCommandBuffer() );
                }

                CSwapChain* GetSwapChain() const { return m_pSwapChain; }
                CSubmit* GetNextSubmit(uint32_t submitCount, const VkSemaphore& vkBackBufferAcquireSemaphore)
                {
                    return _GetNextSubmit( submitCount, vkBackBufferAcquireSemaphore );
                }

                void SetEventListener(EventListeners::IGraphicsContext*);

                const SGraphicsContextDesc& GetDesc() const { return m_Desc; }

                void Wait();

                Vulkan::CDeviceWrapper& _GetDevice() const { return m_VkDevice; }
                Vulkan::Queue _GetQueue() const { return m_pQueue; }

            protected:         

                void            _Destroy();
                Result          _CreateSwapChain(const SSwapChainDesc&);
                VkCommandBuffer _CreateCommandBuffer();
                
                vke_force_inline
                void            _CreateCommandBuffers(uint32_t count, VkCommandBuffer* pArray)
                {
                    m_CmdBuffMgr.CreateCommandBuffers<true /*Thread Safe*/>(count, pArray);
                }

                void            _ReleaseCommandBuffer(VkCommandBuffer vkCb);
                void            _FreeCommandBuffer(const VkCommandBuffer&);

                vke_force_inline
                void            _FreeCommandBuffers(uint32_t count, VkCommandBuffer* pArray)
                {
                    m_CmdBuffMgr.FreeCommandBuffers<true /*Thread Safe*/>(count, pArray);
                }

                void            _SubmitCommandBuffers(const CommandBufferArray&, VkFence);
                VkFence         _CreateFence(VkFenceCreateFlags flags);
                void            _DestroyFence(VkFence* pVkFence);
                VkSemaphore     _CreateSemaphore();
                void            _DestroySemaphore(VkSemaphore* pVkSemaphore);
                
                void            _AddToPresent(CSwapChain*);

                vke_force_inline
                CSubmit*        _GetNextSubmit(uint8_t cmdBufferCount, const VkSemaphore& vkWait)
                {
                    return m_SubmitMgr.GetNextSubmit(cmdBufferCount, vkWait);
                }

                Result          _AllocateCommandBuffers(VkCommandBuffer* pBuffers, uint32_t count);

                void            _EnableRenderQueue(CRenderQueue*, bool);
                void            _ExecuteSubmit(SSubmit*);

                TaskState      _RenderFrameTask();
                TaskState      _PresentFrameTask();
                TaskState      _SwapBuffersTask();

                vke_force_inline
                void            _SetCurrentTask(TASK task);
                vke_force_inline
                TASK            _GetCurrentTask();

                VkInstance      _GetInstance() const;

                template<typename ObjectT, typename VkStructT>
                ObjectT _CreateObject(const VkStructT& VkCreateInfo, Utils::TSFreePool< ObjectT >* pOut);
                template<typename ObjectBufferT>
                void _DestroyObjects( ObjectBufferT* pOut );

            protected:

                SGraphicsContextDesc        m_Desc;
                CDeviceContext*             m_pDeviceCtx = nullptr;
                Vulkan::CDeviceWrapper&     m_VkDevice;
                RenderQueueArray            m_vpRenderQueues;
                CommandBufferArrays         m_avCmdBuffers;
                CCommandBufferManager       m_CmdBuffMgr;
                CSubmitManager              m_SubmitMgr;
                CSwapChain*                 m_pSwapChain = nullptr;
                Vulkan::Queue               m_pQueue = nullptr;
                SFences                     m_Fences;
                SSemaphores                 m_Semaphores;
                VkCommandPool               m_vkCommandPool = VK_NULL_HANDLE;
                SPresentData                m_PresentData;
                SPrivate*                   m_pPrivate = nullptr;
                Threads::SyncObject         m_SyncObj;
                EventListeners::IGraphicsContext*  m_pEventListener;
                Tasks::SGraphicsContext     m_Tasks;
                Managers::CBackBufferManager*   m_pBackBufferMgr = nullptr;
                CRenderingPipeline*             m_pCurrRenderingPipeline = nullptr;
                RenderTargetArray               m_vpRenderTargets;
                RenderState                     m_renderState = RenderState::NO_RENDER;
                uint16_t                        m_enabledRenderQueueCount = 0;
                bool                            m_readyToPresent = false;
                bool                        m_needQuit = false;
                bool                        m_needBeginFrame = false;
                bool                        m_needEndFrame = false;
                bool                        m_frameEnded = true;
                bool                        m_needRenderFrame = false;
                CurrentTask                 m_CurrentTask = ContextTasks::BEGIN_FRAME;
                Threads::SyncObject         m_CurrentTaskSyncObj;
                VkCommandBuffer             m_vkCbTmp[ 2 ];
                VkFence                     m_vkFenceTmp[2];
                VkSemaphore                 m_vkSignals[ 2 ], m_vkWaits[2];
                CSubmit*                    m_pTmpSubmit;
                bool                        m_createdTmp = false;
                uint32_t                    m_currFrame = 0;
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

        template<typename ObjectT, typename VkStructT>
        ObjectT CGraphicsContext::_CreateObject( const VkStructT& VkCreateInfo,
                                                 Utils::TSFreePool< ObjectT >* pOut)
        {
            ObjectT obj;
            if( !pOut->vFreeElements.PopBack( &obj ) )
            {
                const auto count = pOut->vPool.GetMaxCount();
                for( uint32_t i = 0; i < count; ++i )
                {
                    VK_ERR( m_VkDevice.CreateObject( VkCreateInfo, nullptr, &obj ) );
                    pOut->vPool.PushBack( obj );
                    pOut->vFreeElements.PushBack( obj );
                }
                return _CreateObject( VkCreateInfo, pOut );
            }
            return obj;
        }

        template<typename ObjectBufferT>
        void CGraphicsContext::_DestroyObjects(ObjectBufferT* pOut)
        {
            auto& vPool = pOut->vPool;
            for( auto& obj : vPool )
            {
                m_VkDevice.DestroyObject( nullptr, &obj );
            }
            pOut->vPool.FastClear();
            pOut->vFreeElements.FastClear();
        }

    } // RenderSystem
} // vke
#endif // VKE_VULKAN_RENDERER