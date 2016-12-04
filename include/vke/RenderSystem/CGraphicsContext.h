#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/TCDynamicRingArray.h"
#include "Core/Utils/TCList.h"
#include "Core/Threads/Common.h"
#include "RenderSystem/Tasks/GraphicsContext.h"
#include "RenderSystem/CRenderTarget.h"
#include "Core/VKEForwardDeclarations.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CSwapChain;
        class CRenderQueue;
        class CDeviceContext;
        class CRenderTarget;
        class CResourceManager;
        struct SInternal;

        class VKE_API CGraphicsContext
        {
            friend class CRenderSystem;
            friend class CDeviceContext;
            friend class CSwapChain;
            friend class CRenderQueue;
            friend class CRenderTarget;
            friend class CResourceManager;
            struct SPrivate;

            static const uint32_t DEFAULT_CMD_BUFFER_COUNT = 32;
            using CommandBufferArray = Utils::TCDynamicRingArray< VkCommandBuffer, DEFAULT_CMD_BUFFER_COUNT >;
            using UintArray = Utils::TCDynamicArray< uint32_t, DEFAULT_CMD_BUFFER_COUNT >;
            using SemaphoreArray = Utils::TCDynamicArray< VkSemaphore >;
            using FenceArray = Utils::TCDynamicArray< VkFence >;
            using SwapChainArray = Utils::TCDynamicArray< VkSwapchainKHR >;
            using RenderQueueArray = Utils::TCDynamicArray< CRenderQueue* >;
            using RenderTargetArray = Utils::TCDynamicArray< RenderTargetRefPtr >;

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

            struct SCommnadBuffers
            {
                CommandBufferArray  vCmdBuffers;
                CommandBufferArray  vFreeCmdBuffers;
            };

            struct SFences
            {
                FenceArray  vFences;
                FenceArray  vFreeFences;
            };

            using CommandBufferArrays = SCommnadBuffers[ RenderQueueUsages::_MAX_COUNT ];

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
                bool PresentFrame();

                Result  CreateSwapChain(const SSwapChainDesc& Desc);

                const Vulkan::ICD::Device&    _GetICD() const;

                vke_force_inline
                CDeviceContext*        GetDeviceContext() const { return m_pDeviceCtx; }

                CRenderQueue* CreateRenderQueue(const SRenderQueueDesc&);
                Result ExecuteRenderQueue(CRenderQueue*);
                Result ExecuteRenderQueue(const handle_t&);

                void SetEventListener(EventListeners::IGraphicsContext*);

                const SGraphicsContextDesc& GetDesc() const { return m_Desc; }

                void Wait();

                Vulkan::CDeviceWrapper& _GetDevice() const { return m_VkDevice; }
                Vulkan::Queue _GetQueue() const { return m_pQueue; }

            protected:         

                
                Result          _CreateSwapChain(const SSwapChainDesc&);
                VkCommandBuffer _CreateCommandBuffer(RENDER_QUEUE_USAGE usage);
                void            _ReleaseCommandBuffer(RENDER_QUEUE_USAGE usage, VkCommandBuffer vkCb);
                void            _FreeCommandBuffer(RENDER_QUEUE_USAGE usage, const VkCommandBuffer&);
                void            _FreeCommandBuffers(RENDER_QUEUE_USAGE usage, const CommandBufferArray&);
                void            _SubmitCommandBuffers(const CommandBufferArray&, VkFence);
                VkFence         _CreateFence();
                void            _DestroyFence(VkFence vkFence);
                
                void            _AddToPresent(CSwapChain*);

                SSubmit*        _GetNextSubmit();
                VkCommandBuffer _GetNextCommandBuffer(RENDER_QUEUE_USAGE usage);

                Result          _AllocateCommandBuffers(CommandBufferArray*);

                void            _EnableRenderQueue(CRenderQueue*, bool);
                void            _ExecuteSubmit(SSubmit*);

                bool            _BeginFrame();
                void            _EndFrame();

                VkInstance      _GetInstance() const;

            protected:

                SGraphicsContextDesc        m_Desc;
                CDeviceContext*             m_pDeviceCtx = nullptr;
                Vulkan::CDeviceWrapper&     m_VkDevice;
                RenderQueueArray            m_vpRenderQueues;
                CommandBufferArrays         m_avCmdBuffers;
                SubmitList                  m_lSubmits;
                SSubmit*                    m_pCurrSubmit = nullptr;
                CSwapChain*                 m_pSwapChain = nullptr;
                Vulkan::Queue               m_pQueue = nullptr;
                SFences                     m_Fences;
                VkCommandPool               m_vkCommandPool = VK_NULL_HANDLE;
                SPresentData                m_PresentData;
                SPrivate*                   m_pPrivate = nullptr;
                Threads::SyncObject         m_SyncObj;
                EventListeners::IGraphicsContext*  m_pEventListener;
                Tasks::SGraphicsContext     m_Tasks;
                RenderTargetArray           m_vpRenderTargets;
                uint16_t                    m_enabledRenderQueueCount = 0;
                bool                        m_readyToPresent = false;
                bool                        m_presentDone = false;
                VkCommandBuffer             m_vkCbTmp = VK_NULL_HANDLE;
                VkFence                     m_vkFenceTmp[2];
                uint32_t                    m_currFrame = 0;
        };
    } // RenderSystem
} // vke