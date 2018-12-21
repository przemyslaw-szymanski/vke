#include "RenderSystem/CDeviceContext.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Vulkan/CVkDeviceWrapper.h"
#include "RenderSystem/CRenderSystem.h"
#include "RenderSystem/CGraphicsContext.h"
#include "Core/Utils/CLogger.h"
#include "Core/Utils/Common.h"
#include "RenderSystem/Vulkan/PrivateDescs.h"
#include "CVkEngine.h"
#include "Core/Threads/ITask.h"
#include "Core/Threads/CThreadPool.h"
#include "RenderSystem/CRenderPass.h"
#include "RenderSystem/CRenderingPipeline.h"
#include "Core/Memory/CMemoryPoolManager.h"
#include "RenderSystem/Managers/CAPIResourceManager.h"
#include "RenderSystem/Managers/CShaderManager.h"
#include "Core/Platform/CWindow.h"
#include "RenderSystem/CSwapChain.h"
#include "RenderSystem/Vulkan/Managers/CPipelineManager.h"
#include "RenderSystem/Vulkan/Managers/CDescriptorSetManager.h"
#include "RenderSystem/Vulkan/Managers/CBufferManager.h"
#include "RenderSystem/Vulkan/Managers/CTextureManager.h"

namespace VKE
{
    Memory::CMemoryPoolManager g_MemPoolMgr;
    namespace RenderSystem
    {
        template<typename T>
        using ResourceBuffer = Utils::TCDynamicArray< T, 256 >;


        struct CDeviceContext::SInternalData
        {
            SInternalData(VkDevice vkDevice, const Vulkan::ICD::Device& vkICD) :
                ICD(vkICD), VkWrapper(vkDevice, ICD.Device) {}

            struct
            {
                VkPhysicalDevice    vkPhysicalDevice;
                VkDevice            vkDevice;
                VkInstance          vkInstance;
            } Vulkan;

            Vulkan::ICD::Device     ICD;
            Vulkan::CDeviceWrapper  VkWrapper;
            SDeviceContextDesc      Desc;
            SDeviceProperties       Properties;

            struct
            {
                ResourceBuffer< TSVkObject<VkFramebuffer, VkFramebufferCreateInfo > >       vFramebuffers;
                ResourceBuffer< TSVkObject<VkRenderPass, VkRenderPassCreateInfo > >         vRenderPasses;
                ResourceBuffer< TSVkObject<VkImage, VkImageCreateInfo > >                   vImages;
                ResourceBuffer< TSVkObject<VkImageView, VkImageViewCreateInfo > >           vImageViews;
                ResourceBuffer< TSVkObject<VkShaderModule, VkShaderModuleCreateInfo > >     vShaderModules;
                ResourceBuffer< TSVkObject<VkPipeline, VkGraphicsPipelineCreateInfo > >     vGraphicsPipelines;
                ResourceBuffer< TSVkObject<VkPipeline, VkComputePipelineCreateInfo > >      vComputePipelines;
                ResourceBuffer< TSVkObject<VkSampler, VkSamplerCreateInfo > >               vSamplers;
            } Objects;

            struct Tasks
            {
                struct CreateGraphicsContext : public Threads::ITask
                {
                    CDeviceContext* pCtx = nullptr;
                    CGraphicsContext* pGraphicsCtxOut = nullptr;
                    SGraphicsContextDesc Desc;

                    TaskState _OnStart(uint32_t /*threadId*/)
                    {
                        pGraphicsCtxOut = pCtx->_CreateGraphicsContextTask(Desc);
                        return TaskStateBits::OK;
                    }

                    void _OnGet(void** ppOut)
                    {
                        *ppOut = pGraphicsCtxOut;
                    }
                };
            };
        };

        struct SPropertiesInput
        {
            VkICD::Instance&    ICD;
            VkPhysicalDevice    vkPhysicalDevice;

            SPropertiesInput() = delete;
            void operator=(const SPropertiesInput&) = delete;
        };

        Result GetProperties(const SPropertiesInput& In, SDeviceProperties* pOut);
        Result CheckExtensions(VkPhysicalDevice, VkICD::Instance&, const Utils::TCDynamicArray<const char*>&);

        CDeviceContext::CDeviceContext(CRenderSystem* pRS) :
            m_pRenderSystem( pRS ),
            m_CmdBuffMgr( this )
        {}

        CDeviceContext::~CDeviceContext()
        {
            _Destroy();
        }

        void CDeviceContext::Destroy()
        {
            assert(m_pRenderSystem);
            if( m_pPrivate )
            {
                CDeviceContext* pCtx = this;
                m_pRenderSystem->DestroyDeviceContext(&pCtx);
            }
        }

        void CDeviceContext::_Destroy()
        {
            Threads::ScopedLock l(m_SyncObj);
            m_canRender = false;
            if( m_pPrivate )
            {
                //m_pVkDevice->Wait();
                m_DDI.WaitForDevice();

                m_pBufferMgr->Destroy();
                m_pPipelineMgr->Destroy();
                m_pShaderMgr->Destroy();
                m_pAPIResMgr->Destroy();
                m_pDescSetMgr->Destroy();

                Memory::DestroyObject( &HeapAllocator, &m_pBufferMgr );
                Memory::DestroyObject( &HeapAllocator, &m_pPipelineMgr );
                Memory::DestroyObject( &HeapAllocator, &m_pShaderMgr );
                Memory::DestroyObject( &HeapAllocator, &m_pAPIResMgr );
                Memory::DestroyObject( &HeapAllocator, &m_pDescSetMgr );

                for( auto& pRp : m_vpRenderPasses )
                {
                    Memory::DestroyObject(&HeapAllocator, &pRp);
                }
                m_vpRenderPasses.Clear();
                for( auto& pRT : m_vpRenderTargets )
                {
                    Memory::DestroyObject(&HeapAllocator, &pRT);
                }
                m_vpRenderTargets.Clear();

                for( auto& pRP : m_vpRenderingPipelines )
                {
                    Memory::DestroyObject(&HeapAllocator, &pRP);
                }
                m_vpRenderingPipelines.Clear();

                /*for( auto& pCtx : m_vGraphicsContexts )
                {
                    Memory::DestroyObject(&HeapAllocator, &pCtx);
                }*/
                for( auto& pCtx : m_GraphicsContexts.vPool )
                {
                    pCtx->_Destroy();
                    Memory::DestroyObject( &HeapAllocator, &pCtx );
                }
                for( auto& pCtx : m_GraphicsContexts.vFreeElements )
                {
                    pCtx->_Destroy();
                    Memory::DestroyObject( &HeapAllocator, &pCtx );
                }
                m_GraphicsContexts.vPool.Clear();
                m_GraphicsContexts.vFreeElements.Clear();

                //m_vGraphicsContexts.Clear()
                Memory::DestroyObject( &HeapAllocator, &m_pPrivate );
            }
        }

        Result CDeviceContext::Create(const SDeviceContextDesc& Desc)
        {
            //const SPrivateToDeviceCtx* pPrivate = reinterpret_cast< const SPrivateToDeviceCtx* >(Desc.pPrivate);
      
            //assert(m_pPrivate == nullptr);
            //Vulkan::ICD::Device ICD = { pPrivate->ICD.Global, pPrivate->ICD.Instance };
            m_Desc = Desc;
            Result ret = m_DDI.CreateDevice( this );
            //Utils::TCDynamicArray< const char* > vExtensions;
            //vExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

            //VkPhysicalDevice vkPhysicalDevice = reinterpret_cast<VkPhysicalDevice>(Desc.pAdapterInfo->handle);
            ////VkInstance vkInstance = reinterpret_cast<VkInstance>(Desc.hAPIInstance);
            //VkInstance vkInstance = m_pRenderSystem->_GetVkInstance();

            //{
            //    ICD.Instance.vkGetPhysicalDeviceFeatures( vkPhysicalDevice, &m_DeviceInfo.Features );
            //    //ICD.Instance.vkGetPhysicalDeviceFormatProperties( vkPhysicalDevice, &m_DeviceInfo.FormatProperties );
            //    ICD.Instance.vkGetPhysicalDeviceMemoryProperties( vkPhysicalDevice, &m_DeviceInfo.MemoryProperties );
            //    ICD.Instance.vkGetPhysicalDeviceProperties( vkPhysicalDevice, &m_DeviceInfo.Properties );
            //}

            //SPropertiesInput In = { ICD.Instance };
            //In.vkPhysicalDevice = vkPhysicalDevice;
            //SDeviceProperties DevProps;

            //VKE_RETURN_IF_FAILED(GetProperties(In, &DevProps));
            //VKE_RETURN_IF_FAILED(CheckExtensions(vkPhysicalDevice, ICD.Instance, vExtensions));

            //Utils::TCDynamicArray<VkDeviceQueueCreateInfo> vQis;
            //for (auto& Family : DevProps.vQueueFamilies)
            //{
            //    if (!Family.vQueues.IsEmpty())
            //    {
            //        VkDeviceQueueCreateInfo qi;
            //        Vulkan::InitInfo(&qi, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
            //        qi.flags = 0;
            //        qi.pQueuePriorities = &Family.vPriorities[0];
            //        qi.queueFamilyIndex = Family.index;
            //        qi.queueCount = static_cast<uint32_t>(Family.vQueues.GetCount());
            //        vQis.PushBack(qi);
            //    }
            //}

            ////VkPhysicalDeviceFeatures df = {};

            //VkDeviceCreateInfo di;
            //Vulkan::InitInfo(&di, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
            //di.enabledExtensionCount = vExtensions.GetCount();
            //di.enabledLayerCount = 0;
            //di.pEnabledFeatures = nullptr;
            //di.ppEnabledExtensionNames = &vExtensions[0];
            //di.ppEnabledLayerNames = nullptr;
            //di.pQueueCreateInfos = &vQis[0];
            //di.queueCreateInfoCount = static_cast<uint32_t>(vQis.GetCount());
            //di.flags = 0;

            //VkDevice vkDevice;
            //VK_ERR(ICD.Instance.vkCreateDevice(vkPhysicalDevice, &di, nullptr, &vkDevice));
            //
            //VKE_RETURN_IF_FAILED(Vulkan::LoadDeviceFunctions(vkDevice, ICD.Instance, &ICD.Device));
            //
            //VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pPrivate, vkDevice, ICD));

            //for( auto& Family : DevProps.vQueueFamilies )
            //{
            //    for( uint32_t q = 0; q < Family.vQueues.GetCount(); ++q )
            //    {
            //        VkQueue vkQueue;
            //        ICD.Device.vkGetDeviceQueue(vkDevice, Family.index, q, &vkQueue);
            //        Family.vQueues[ q ].vkQueue = vkQueue;
            //        Family.vQueues[ q ].familyIndex = Family.index;
            //    }
            //}

            /*m_pPrivate->Desc = Desc;
            m_pPrivate->Properties = DevProps;
            m_pPrivate->Vulkan.vkDevice = vkDevice;
            m_pPrivate->Vulkan.vkPhysicalDevice = vkPhysicalDevice;
            m_pPrivate->Vulkan.vkInstance = vkInstance*/;

            //if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &m_pVkDevice, vkDevice, ICD.Device)) )
            //{
            //    goto ERR;
            //}

            //if( VKE_FAILED( m_DDI.Init( this )) )
            //{
            //    goto ERR;
            //}

            
            {
                if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pAPIResMgr, this ) ) )
                {
                    VKE_LOG_ERR("Unable to allocate memory for CAPIResourceManager object.");
                    return VKE_ENOMEMORY;
                }
                RenderSystem::SResourceManagerDesc Desc;
                if( VKE_FAILED( m_pAPIResMgr->Create( Desc ) ) )
                {
                    return VKE_FAIL;
                }
            }
            {
                SCommandBufferManagerDesc Desc;
                if( VKE_SUCCEEDED( m_CmdBuffMgr.Create( Desc ) ) )
                {
                    SCommandPoolDesc Desc;
                    Desc.commandBufferCount = CCommandBufferManager::DEFAULT_COMMAND_BUFFER_COUNT; /// @todo hardcode...
                                                                                                   /// @todo store command pool handle
                    if( m_CmdBuffMgr.CreatePool( Desc ) == NULL_HANDLE )
                    {
                        goto ERR;
                    }
                }
            }
            {
                if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pBufferMgr, this ) ) )
                {
                    VKE_LOG_ERR( "Unable to allocate memory for CBufferManager object." );
                    return VKE_ENOMEMORY;
                }
                RenderSystem::SBufferManagerDesc Desc;
                if( VKE_FAILED( m_pBufferMgr->Create( Desc ) ) )
                {
                    return VKE_FAIL;
                }
            }

            {
                if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pShaderMgr, this ) ) )
                {
                    VKE_LOG_ERR( "Unable to allocate memory for CShaderManager object." );
                    return VKE_ENOMEMORY;
                }
                RenderSystem::SShaderManagerDesc Desc;
                if( VKE_FAILED( m_pShaderMgr->Create( Desc ) ) )
                {
                    return VKE_FAIL;
                }
            }

            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &m_pDescSetMgr, this ) ) )
                {
                    SDescriptorSetManagerDesc MgrDesc;
                    Memory::Copy<  DescriptorSetTypes::_MAX_COUNT >( MgrDesc.aMaxDescriptorSetCounts, Desc.aMaxDescriptorSetCounts );
                    if( VKE_FAILED( m_pDescSetMgr->Create( MgrDesc ) ) )
                    {
                        goto ERR;
                    }
                }
                else
                {
                    goto ERR;
                }
            }

            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &m_pPipelineMgr, this ) ) )
                {
                    SPipelineManagerDesc Desc;
                    Desc.maxPipelineCount = Config::RenderSystem::Pipeline::MAX_PIPELINE_COUNT;
                    if( VKE_FAILED( m_pPipelineMgr->Create( Desc ) ) )
                    {
                        goto ERR;
                    }
                }
                else
                {
                    goto ERR;
                }
            }
            
            m_vpRenderTargets.PushBack(nullptr);
            m_canRender = true;
            return VKE_OK;
ERR:
            _Destroy();
            return VKE_FAIL;
        }

        CGraphicsContext* CDeviceContext::CreateGraphicsContext(const SGraphicsContextDesc& Desc)
        {
            SInternalData::Tasks::CreateGraphicsContext CreateGraphicsContextTask;
            CreateGraphicsContextTask.Desc = Desc;
            CreateGraphicsContextTask.pCtx = this;
            m_pRenderSystem->GetEngine()->GetThreadPool()->AddTask( &CreateGraphicsContextTask );
            CGraphicsContext* pCtx = nullptr;
            CreateGraphicsContextTask.Get(&pCtx);
            return pCtx;
        }

        void CDeviceContext::DestroyGraphicsContext(CGraphicsContext** ppCtxOut)
        {
            Threads::ScopedLock l(m_SyncObj);
            m_canRender = false;
            //auto idx = m_vGraphicsContexts.Find(*ppCtxOut);
            auto idx = m_GraphicsContexts.vPool.Find( *ppCtxOut );
            if( idx != m_GraphicsContexts.vPool.NPOS )
            {
                CGraphicsContext* pCtx = m_GraphicsContexts.vPool[ idx ];
                assert(pCtx);
                //pCtx->_Destroy();
                //Memory::DestroyObject(&HeapAllocator, &pCtx);
                //m_vGraphicsContexts.Remove(idx);
                pCtx->FinishRendering();
                m_GraphicsContexts.vPool.Remove( idx );
                m_GraphicsContexts.vFreeElements.PushBack( pCtx );
                ppCtxOut = nullptr;
            }

            //if( m_vGraphicsContexts.IsEmpty() && m_vComputeContexts.IsEmpty() )
            if( m_GraphicsContexts.vPool.IsEmpty() && m_vComputeContexts.IsEmpty() )
            {
                CDeviceContext* pCtx = this;
                m_pRenderSystem->DestroyDeviceContext(&pCtx);
            }
            m_canRender = true;
        }

        CGraphicsContext* CDeviceContext::_CreateGraphicsContextTask(const SGraphicsContextDesc& Desc)
        {
            // Find context
            //for( auto pCtx : m_vGraphicsContexts )
            for( auto& pCtx : m_GraphicsContexts.vPool )
            {
                if( pCtx->GetDesc().SwapChainDesc.pWindow == Desc.SwapChainDesc.pWindow )
                {
                    VKE_LOG_ERR("Graphics context for window: " << Desc.SwapChainDesc.pWindow->GetDesc().hWnd << " already created.");
                    return nullptr;
                }
            }

            // Find a proper queue
            auto& vQueueFamilies = m_pPrivate->Properties.vQueueFamilies;

            // Get graphics family
            const SQueueFamily* pGraphicsFamily = nullptr;
            const SQueueFamily* pComputeFamily = nullptr;
            const SQueueFamily* pTransferFamily = nullptr;
            const SQueueFamily* pSparseFamily = nullptr;
            for( uint32_t i = vQueueFamilies.GetCount(); i-- > 0;)
            {
                const auto& Family = vQueueFamilies[ i ];
                if( Family.isGraphics )
                {
                    if( Desc.SwapChainDesc.pWindow.IsValid() )
                    {
                        if( Family.isPresent )
                        {
                            pGraphicsFamily = &Family;
                            break;
                        }
                    }
                    else
                    {
                        pGraphicsFamily = &Family;
                    }
                }
                if( Family.isCompute )
                {
                    pComputeFamily = &Family;
                }
                if( Family.isTransfer )
                {
                    pTransferFamily = &Family;
                }
                if( Family.isSparse )
                {
                    pSparseFamily = &Family;
                }
            }
            
            if( !pGraphicsFamily )
            {
                VKE_LOG_ERR("This GPU does not support graphics queue.");
                return nullptr;
            }

            CGraphicsContext* pCtx;
            if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pCtx, this)) )
            {
                VKE_LOG_ERR("Unable to create object CGraphicsContext. No memory.");
                return nullptr;
            }

            auto& vQueues = pGraphicsFamily->vQueues;
            // Select queue based on current graphics context
            //Vulkan::SQueue* pQueue = &vQueues[ m_vGraphicsContexts.GetCount() % vQueues.GetCount() ];
            Vulkan::SQueue* pQueue = &vQueues[ m_GraphicsContexts.vPool.GetCount() % vQueues.GetCount() ];
            // Add reference count. If refCount > 1 multithreaded case should be handled as more than
            // one graphicsContext refers to this queue
            // _AddRef/_RemoveRef should not be called externally
            pQueue->_AddRef();
            if( Desc.SwapChainDesc.pWindow.IsValid() )
            {
                // Add swapchain ref count if this context uses swapchain
                pQueue->m_swapChainCount++;
            }

            SGraphicsContextDesc CtxDesc = Desc;
            SGraphicsContextPrivateDesc PrvDesc;
            PrvDesc.pICD = &m_pPrivate->ICD;
            PrvDesc.pQueue = pQueue;
            PrvDesc.vkDevice = m_pPrivate->Vulkan.vkDevice;
            PrvDesc.vkInstance = m_pPrivate->Vulkan.vkInstance;
            PrvDesc.vkPhysicalDevice = m_pPrivate->Vulkan.vkPhysicalDevice;
            CtxDesc.pPrivate = &PrvDesc;

            if( VKE_FAILED(pCtx->Create(CtxDesc)) )
            {
                Memory::DestroyObject(&HeapAllocator, &pCtx);
            }
            //if( m_vGraphicsContexts.PushBack(pCtx) == Utils::INVALID_POSITION )
            if( m_GraphicsContexts.vPool.PushBack( pCtx ) == Utils::INVALID_POSITION )
            {
                VKE_LOG_ERR("Unable to add GraphicsContext to the buffer.");
                Memory::DestroyObject(&HeapAllocator, &pCtx);
            }
            return pCtx;
        }

        VkInstance CDeviceContext::_GetInstance() const
        {
            //return m_pRenderSystem->_GetInstance();
            return VK_NULL_HANDLE;
        }

        Vulkan::ICD::Device& CDeviceContext::_GetICD() const
        {
            return m_pPrivate->ICD;
        }

        void CDeviceContext::_NotifyDestroy(CGraphicsContext* pCtx)
        {
            assert(pCtx);
            assert(pCtx->m_pQueue);
            //if( pCtx->m_pQueue->GetRefCount() > 0 )
            {
                pCtx->m_pQueue->_RemoveRef();
                pCtx->m_pQueue = nullptr;
            }
        }

        void CDeviceContext::RenderFrame(WindowPtr pWnd)
        {
            //Threads::SyncObject l( m_SyncObj );
            if( m_canRender )
            {
				//const uint32_t count = m_GraphicsContexts.vPool.GetCount();
    //            for(uint32_t i = 0; i < count; ++i )
    //            {
    //                //m_vGraphicsContexts[ i ]->RenderFrame();
    //                m_GraphicsContexts.vPool[ i ]->RenderFrame();
    //            }
                pWnd->GetSwapChain()->GetGraphicsContext()->RenderFrame();
            }
        }

        Result CDeviceContext::_AddTask( Threads::ITask* pTask )
        {
            return m_pRenderSystem->GetEngine()->GetThreadPool()->AddTask( pTask );
        }

        VkImageLayout ConvertInitialLayoutToOptimalLayout(VkImageLayout vkInitial)
        {
            static const VkImageLayout aVkLayouts[] =
            {
                VK_IMAGE_LAYOUT_UNDEFINED, // undefined -> undefined
                VK_IMAGE_LAYOUT_UNDEFINED, // general -> undefined
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, // color attachment -> color attachment
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, // depth -> depth
                VK_IMAGE_LAYOUT_UNDEFINED, // depth read only -> undefined
                VK_IMAGE_LAYOUT_UNDEFINED, // transfer src -> undefined
                VK_IMAGE_LAYOUT_UNDEFINED, // n/a
                VK_IMAGE_LAYOUT_UNDEFINED, // n/a
                VK_IMAGE_LAYOUT_UNDEFINED
            };
            return aVkLayouts[ vkInitial ];
        }

        VkImageLayout ConvertInitialLayoutToReadLayout(VkImageLayout vkInitial)
        {
            static const VkImageLayout aVkLayouts[] =
            {
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // color attachment -> read only
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, // depth attachment -> read only
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, // depth read only -> depth read only
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, // read only -> read only
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_UNDEFINED
            };
            return aVkLayouts[ vkInitial ];
        }

        /*Result CDeviceContext::UpdateRenderTarget(const RenderTargetHandle& hRT, const SRenderTargetDesc& Desc)
        {
            return m_vpRenderTargets[ hRT ]->Update(Desc);
        }

        RenderTargetHandle CDeviceContext::CreateRenderTarget(const SRenderTargetDesc& Desc)
        {
            CRenderTarget* pRT;
            if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pRT, this)) )
            {
                VKE_LOG_ERR("Unable to create memory for render target.");
                return NULL_HANDLE;
            }

            if( VKE_FAILED(pRT->Create(Desc)) )
            {
                Memory::DestroyObject(&HeapAllocator, &pRT);
                return NULL_HANDLE;
            }

            return RenderTargetHandle( m_vpRenderTargets.PushBack(pRT) );
        }*/

        RenderPassHandle CDeviceContext::CreateRenderPass(const SRenderPassDesc& Desc)
        {
            CRenderPass* pPass;
            RenderPassHandle hPass = NULL_HANDLE;
            if( VKE_SUCCEEDED(Memory::CreateObject(&HeapAllocator, &pPass, this)) )
            {
                if( VKE_SUCCEEDED( pPass->Create( Desc ) ) )
                {
                    hPass = RenderPassHandle( m_vpRenderPasses.PushBack(pPass) );
                    pPass->m_hObjHandle = hPass.handle;
                }
                else
                {
                    Memory::DestroyObject( &HeapAllocator, &pPass );
                }
            }
            else
            {
                VKE_LOG_ERR("Unable to create memory for render pass.");
            }
            return hPass;
        }

        CRenderPass* CDeviceContext::GetRenderPass(const RenderPassHandle& hPass) const
        {
            return m_vpRenderPasses[ hPass.handle ];
        }

        PipelineRefPtr CDeviceContext::CreatePipeline(const SPipelineCreateDesc& Desc)
        {
            return m_pPipelineMgr->CreatePipeline( Desc );
        }

        PipelineLayoutRefPtr CDeviceContext::CreatePipelineLayout(const SPipelineLayoutDesc& Desc)
        {
            return m_pPipelineMgr->CreateLayout( Desc );
        }

        void CDeviceContext::SetPipeline(CommandBufferPtr pCmdBuffer, PipelinePtr pPipeline)
        {
            static const VkPipelineBindPoint aVkBinds[] =
            {
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                VK_PIPELINE_BIND_POINT_COMPUTE
            };

            const VkPipelineBindPoint vkBind = aVkBinds[ pPipeline->GetType() ];
            m_pPrivate->ICD.Device.vkCmdBindPipeline( pCmdBuffer->m_vkCommandBuffer, vkBind, pPipeline->m_vkPipeline );
        }

        ShaderRefPtr CDeviceContext::CreateShader(const SShaderCreateDesc& Desc)
        {
            return m_pShaderMgr->CreateShader(Desc);
        }

        DescriptorSetRefPtr CDeviceContext::CreateDescriptorSet(const SDescriptorSetDesc& Desc)
        {
            return m_pDescSetMgr->CreateSet( Desc );
        }

        DescriptorSetLayoutRefPtr CDeviceContext::CreateDescriptorSetLayout(const SDescriptorSetLayoutDesc& Desc)
        {
            return m_pDescSetMgr->CreateLayout( Desc );
        }

        BufferRefPtr CDeviceContext::CreateBuffer( const SCreateBufferDesc& Desc )
        {
            return m_pBufferMgr->CreateBuffer( Desc );
        }

        ShaderRefPtr CDeviceContext::GetShader( ShaderHandle hShader )
        {
            return m_pShaderMgr->GetShader( hShader );
        }

        DescriptorSetRefPtr CDeviceContext::GetDescriptorSet( DescriptorSetHandle hSet )
        {
            return m_pDescSetMgr->GetDescriptorSet( hSet );
        }

        DescriptorSetLayoutRefPtr CDeviceContext::GetDescriptorSetLayout( DescriptorSetLayoutHandle hSet )
        {
            return m_pDescSetMgr->GetDescriptorSetLayout( hSet );
        }

        PipelineRefPtr CDeviceContext::GetPipeline( PipelineHandle hPipeline )
        {
            return m_pPipelineMgr->GetPipeline( hPipeline );
        }

        BufferRefPtr CDeviceContext::GetBuffer( BufferHandle hBuffer )
        {
            return m_pBufferMgr->GetBuffer( hBuffer );
        }

        TextureHandle CDeviceContext::CreateTexture( const SCreateTextureDesc& Desc )
        {
            return m_pTextureMgr->CreateTexture( Desc.Texture );
        }

        TextureRefPtr CDeviceContext::GetTexture( TextureHandle hTex )
        {
            return m_pTextureMgr->GetTexture( hTex );
        }

        void CDeviceContext::DestroyTexture( TexturePtr* ppTex )
        {
            m_pTextureMgr->FreeTexture( ppTex );
        }

        void CDeviceContext::DestroyTexture( TextureHandle hTex )
        {
            TexturePtr pTex = GetTexture( hTex );
            m_pTextureMgr->FreeTexture( &pTex );
        }

        Result CDeviceContext::_CreateCommandBuffers( uint32_t count, CommandBufferPtr* ppArray )
        {
            return m_CmdBuffMgr.CreateCommandBuffers< VKE_THREAD_SAFE >( count, ppArray );
        }

        void CDeviceContext::_FreeCommandBuffers( uint32_t count, CommandBufferPtr* ppArray )
        {
            m_CmdBuffMgr.FreeCommandBuffers< VKE_THREAD_SAFE >( 1, &ppArray );
        }

        /*RenderingPipelineHandle CDeviceContext::CreateRenderingPipeline(const SRenderingPipelineDesc& Desc)
        {
            CRenderingPipeline* pRP;
            if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pRP, this)) )
            {
                VKE_LOG_ERR("Unable to create CRenderingPipeline object. No memory.");
                return NULL_HANDLE;
            }
            if( VKE_FAILED(pRP->Create(Desc)) )
            {
                Memory::DestroyObject(&HeapAllocator, &pRP);
                return NULL_HANDLE;
            }
            return RenderingPipelineHandle( m_vpRenderingPipelines.PushBack(pRP) );
        }*/

        Result GetProperties(const SPropertiesInput& In, SDeviceProperties* pOut)
        {
            auto& Instance = In.ICD;
            
            uint32_t propCount = 0;
            Instance.vkGetPhysicalDeviceQueueFamilyProperties(In.vkPhysicalDevice, &propCount, nullptr);
            if (propCount == 0)
            {
                VKE_LOG_ERR("No device queue family properties");
                return VKE_FAIL;
            }
            
            pOut->vQueueFamilyProperties.Resize(propCount);
            auto& aProperties = pOut->vQueueFamilyProperties;
            auto& vQueueFamilies = pOut->vQueueFamilies;

            Instance.vkGetPhysicalDeviceQueueFamilyProperties(In.vkPhysicalDevice, &propCount, &aProperties[0]);
            // Choose a family index
            for (uint32_t i = 0; i < propCount; ++i)
            {
                //uint32_t isGraphics = aProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                uint32_t isCompute = aProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
                uint32_t isTransfer = aProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
                uint32_t isSparse = aProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
                uint32_t isGraphics = aProperties[ i ].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                VkBool32 isPresent = VK_FALSE;
#if VKE_USE_VULKAN_WINDOWS
                isPresent = Instance.vkGetPhysicalDeviceWin32PresentationSupportKHR(In.vkPhysicalDevice, i);           
#elif VKE_USE_VULKAN_LINUX
                isPresent = Instance.vkGetPhysicalDeviceXcbPresentationSupportKHR(s_physical_device, i,
                                                                                  xcb_connection, visual_id);
#elif VKE_USE_VULKAN_ANDROID
#error "implement"
#endif

                SQueueFamily Family;
                Family.vQueues.Resize(aProperties[i].queueCount);
                Family.vPriorities.Resize(aProperties[i].queueCount, 1.0f);
                Family.index = i;
                Family.isGraphics = isGraphics != 0;
                Family.isCompute = isCompute != 0;
                Family.isTransfer = isTransfer != 0;
                Family.isSparse = isSparse != 0;
                Family.isPresent = isPresent == VK_TRUE;

                vQueueFamilies.PushBack(Family);
            }

            Instance.vkGetPhysicalDeviceMemoryProperties(In.vkPhysicalDevice, &pOut->vkMemProperties);
            Instance.vkGetPhysicalDeviceFeatures(In.vkPhysicalDevice, &pOut->vkFeatures);

            for (uint32_t i = 0; i < RenderSystem::Formats::_MAX_COUNT; ++i)
            {
                const auto& fmt = RenderSystem::g_aFormats[i];
                Instance.vkGetPhysicalDeviceFormatProperties(In.vkPhysicalDevice, fmt,
                    &pOut->aFormatProperties[i]);
            }

            return VKE_OK;
        }

        Result CheckExtensions(VkPhysicalDevice vkPhysicalDevice, VkICD::Instance& Instance,
            const Utils::TCDynamicArray<const char *>& vExtensions)
        {
            uint32_t count = 0;
            VK_ERR(Instance.vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &count, nullptr));
            
            Utils::TCDynamicArray< VkExtensionProperties > vProperties(count);

            VK_ERR(Instance.vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &count,
                &vProperties[0]));

            std::string ext;
            Result err = VKE_OK;

            for (uint32_t e = 0; e < vExtensions.GetCount(); ++e)
            {
                ext = vExtensions[e];
                bool found = false;
                for (uint32_t p = 0; p < count; ++p)
                {
                    if (ext == vProperties[p].extensionName)
                    {
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    VKE_LOG_ERR("Extension: " << ext << " is not supported by the device.");
                    err = VKE_ENOTFOUND;
                }
            }

            return err;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER