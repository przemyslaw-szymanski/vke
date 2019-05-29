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
#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"
#include "RenderSystem/CTransferContext.h"

namespace VKE
{
    //Memory::CMemoryPoolManager g_MemPoolMgr;
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
                        VKE_ASSERT( pGraphicsCtxOut != nullptr, "" );
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
            CContextBase( this )
            , m_pRenderSystem( pRS )
            , m_CmdBuffMgr( this )
        {}

        CDeviceContext::~CDeviceContext()
        {
            _Destroy();
        }

        void CDeviceContext::Destroy()
        {
            assert(m_pRenderSystem);
            //if( m_pPrivate )
            if( !m_canRender )
            {
                CDeviceContext* pCtx = this;
                m_pRenderSystem->DestroyDeviceContext( &pCtx );
            }
        }

        void CDeviceContext::_Destroy()
        {
            Threads::ScopedLock l(m_SyncObj);
            m_canRender = false;
            //if( m_pPrivate )
            if( !m_canRender && m_pDeviceMemMgr != nullptr )
            {
                //m_pVkDevice->Wait();
                m_DDI.WaitForDevice();

                for( auto& pCtx : m_GraphicsContexts.vPool )
                {
                    pCtx->_Destroy();
                    Memory::DestroyObject( &HeapAllocator, &pCtx );
                }
                m_GraphicsContexts.vPool.Clear();
                m_GraphicsContexts.vFreeElements.Clear();

                for( auto& pCtx : m_vpTransferContexts )
                {
                    pCtx->_Destroy();
                    Memory::DestroyObject( &HeapAllocator, &pCtx );
                }
                m_vpTransferContexts.Clear();

                m_CmdBuffMgr.Destroy();
                if( m_pBufferMgr != nullptr )
                {
                    m_pBufferMgr->Destroy();
                }
                if( m_pPipelineMgr != nullptr )
                {
                    m_pPipelineMgr->Destroy();
                }
                if( m_pShaderMgr != nullptr )
                {
                    m_pShaderMgr->Destroy();
                }
                if( m_pDescSetMgr != nullptr )
                {
                    m_pDescSetMgr->Destroy();
                }

                Memory::DestroyObject( &HeapAllocator, &m_pBufferMgr );
                Memory::DestroyObject( &HeapAllocator, &m_pTextureMgr );
                Memory::DestroyObject( &HeapAllocator, &m_pPipelineMgr );
                Memory::DestroyObject( &HeapAllocator, &m_pShaderMgr );
                //Memory::DestroyObject( &HeapAllocator, &m_pAPIResMgr );
                Memory::DestroyObject( &HeapAllocator, &m_pDescSetMgr );

                _DestroyRenderPasses();

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
                
                m_pDeviceMemMgr->Destroy();
                Memory::DestroyObject( &HeapAllocator, &m_pDeviceMemMgr );

                //m_vGraphicsContexts.Clear()
                //Memory::DestroyObject( &HeapAllocator, &m_pPrivate );
            }
        }

        Result CDeviceContext::Create(const SDeviceContextDesc& Desc)
        {
            m_Desc = Desc;
            Result ret = m_DDI.CreateDevice( this );
            if( VKE_FAILED( ret ) )
            {
                return ret;
            }
            
            {
                if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pDeviceMemMgr, this ) ) )
                {
                    goto ERR;
                }
                SDeviceMemoryManagerDesc DeviceMemDesc;
                if( VKE_FAILED( m_pDeviceMemMgr->Create( DeviceMemDesc ) ) )
                {
                    goto ERR;
                }
            }

            {
                SCommandBufferManagerDesc Desc;
                if( VKE_FAILED( m_CmdBuffMgr.Create( Desc ) ) )
                {
                    goto ERR;
                }
            }

            {
                auto pQueue = _AcquireQueue( QueueTypes::ALL );

                SCommandBufferPoolDesc PoolDesc;
                PoolDesc.commandBufferCount = 32;
                PoolDesc.queueFamilyIndex = pQueue->GetFamilyIndex();

                SContextBaseDesc Desc;
                Desc.hCommandBufferPool = m_CmdBuffMgr.CreatePool( PoolDesc );
                Desc.pQueue = pQueue;
                Desc.descPoolSize = 0;
                if( VKE_FAILED( CContextBase::Create( Desc ) ) )
                {
                    goto ERR;
                }

                this->m_initComputeShader = false;
                this->m_initGraphicsShaders = false;
                m_pCurrentCommandBuffer = this->_CreateCommandBuffer();
                m_pCurrentCommandBuffer->m_pBaseCtx->_BeginCommandBuffer( &m_pCurrentCommandBuffer );
                m_pCurrentCommandBuffer->m_state = CCommandBuffer::States::BEGIN;
            }

            {
                STransferContextDesc Desc;
                auto pTransferCtx = CreateTransferContext( Desc );
                if( pTransferCtx == nullptr )
                {
                    goto ERR;
                }
            }
            
            {
                if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pBufferMgr, this ) ) )
                {
                    VKE_LOG_ERR( "Unable to allocate memory for CBufferManager object." );
                    goto ERR;
                }
                RenderSystem::SBufferManagerDesc Desc;
                if( VKE_FAILED( m_pBufferMgr->Create( Desc ) ) )
                {
                    goto ERR;
                }
            }
            
            {
                if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &m_pTextureMgr, this ) ) )
                {
                    VKE_LOG_ERR( "Unable to allocate memory for CTextureManager object." );
                    return VKE_ENOMEMORY;
                }
                RenderSystem::STextureManagerDesc Desc;
                if( VKE_FAILED( m_pTextureMgr->Create( Desc ) ) )
                {
                    goto ERR;
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
                    goto ERR;
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
            /*SInternalData::Tasks::CreateGraphicsContext CreateGraphicsContextTask;
            CreateGraphicsContextTask.Desc = Desc;
            CreateGraphicsContextTask.pCtx = this;
            m_pRenderSystem->GetEngine()->GetThreadPool()->AddTask( &CreateGraphicsContextTask );
            CGraphicsContext* pCtx = nullptr;
            CreateGraphicsContextTask.Get(&pCtx);
            return pCtx;*/
            return _CreateGraphicsContextTask( Desc );
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
                assert( pCtx );
                
                pCtx->_GetQueue()->m_contextRefCount--; // remove context reference count
                pCtx->_Destroy();
                m_GraphicsContexts.Free( idx );
                ppCtxOut = nullptr;
            }

            //if( m_vGraphicsContexts.IsEmpty() && m_vComputeContexts.IsEmpty() )
            if( m_GraphicsContexts.vPool.IsEmpty() && m_vpComputeContexts.IsEmpty() )
            {
                CDeviceContext* pCtx = this;
                m_pRenderSystem->DestroyDeviceContext(&pCtx);
            }
            m_canRender = true;
        }

        CTransferContext* CDeviceContext::CreateTransferContext( const STransferContextDesc& Desc )
        {
            CTransferContext* pCtx = nullptr;
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pCtx, this ) ) )
            {
                // Get next free graphics queue
                QueueRefPtr pQueue = _AcquireQueue( QueueTypes::TRANSFER );
                if( pQueue.IsNull() )
                {
                    VKE_LOG_ERR( "This GPU does not support graphics queue." );
                    return nullptr;
                }

                STransferContextDesc TransferDesc = Desc;
                TransferDesc.CmdBufferPoolDesc.queueFamilyIndex = pQueue->GetFamilyIndex();
                SContextBaseDesc BaseDesc;
                BaseDesc.hCommandBufferPool = m_CmdBuffMgr.CreatePool( TransferDesc.CmdBufferPoolDesc );
                BaseDesc.pQueue = pQueue;
                BaseDesc.descPoolSize = 0;
                TransferDesc.pPrivate = &BaseDesc;

                if( VKE_SUCCEEDED( pCtx->Create( TransferDesc ) ) )
                {
                    Threads::ScopedLock l( m_SyncObj );
                    m_vpTransferContexts.PushBack( pCtx );
                }
                else
                {
                    pQueue->_RemoveContextRef();
                    Memory::DestroyObject( &HeapAllocator, &pCtx );
                    pCtx = nullptr;
                }
            }
            return pCtx;
        }

        CTransferContext* CDeviceContext::GetTransferContext( uint32_t idx /* = 0 */ )
        {
            return m_vpTransferContexts[idx];
        }

        CGraphicsContext* CDeviceContext::_CreateGraphicsContextTask(const SGraphicsContextDesc& Desc)
        {
            // Find context
            for( auto& pCtx : m_GraphicsContexts.vPool )
            {
                if( pCtx->GetDesc().SwapChainDesc.pWindow == Desc.SwapChainDesc.pWindow )
                {
                    VKE_LOG_ERR("Graphics context for window: " << Desc.SwapChainDesc.pWindow->GetDesc().hWnd << " already created.");
                    return nullptr;
                }
            }

            // Get next free graphics queue
            QueueRefPtr pQueue = _AcquireQueue( QueueTypes::GRAPHICS );
            if( pQueue.IsNull() )
            {
                VKE_LOG_ERR( "This GPU does not support graphics queue." );
                return nullptr;
            }

            CGraphicsContext* pCtx;
            if( VKE_FAILED( Memory::CreateObject( &HeapAllocator, &pCtx, this ) ) )
            {
                VKE_LOG_ERR("Unable to create object CGraphicsContext. No memory.");
                pQueue->_RemoveContextRef();
                return nullptr;
            }

            if( Desc.SwapChainDesc.pWindow.IsValid() )
            {
                // Add swapchain ref count if this context uses swapchain
                pQueue->m_swapChainCount++;
            }

            SGraphicsContextDesc CtxDesc = Desc;
            SGraphicsContextPrivateDesc PrvDesc;
            PrvDesc.pQueue = QueueRefPtr( pQueue );
            CtxDesc.CmdBufferPoolDesc.queueFamilyIndex = pQueue->GetFamilyIndex();
            PrvDesc.hCmdPool = m_CmdBuffMgr.CreatePool( CtxDesc.CmdBufferPoolDesc );
            CtxDesc.pPrivate = &PrvDesc;

            if( VKE_FAILED( pCtx->Create( CtxDesc ) ) )
            {
                Memory::DestroyObject( &HeapAllocator, &pCtx );
            }

            if( m_GraphicsContexts.vPool.PushBack( pCtx ) == Utils::INVALID_POSITION )
            {
                VKE_LOG_ERR("Unable to add GraphicsContext to the buffer.");
                Memory::DestroyObject( &HeapAllocator, &pCtx );
            }
            return pCtx;
        }

        QueueRefPtr CDeviceContext::_AcquireQueue(QUEUE_TYPE type)
        {
            // Find a proper queue
            const auto& vQueueFamilies = m_DDI.GetDeviceQueueInfos();
            
            QueueRefPtr pRet;
            // Get graphics family

            for( uint32_t i = vQueueFamilies.GetCount(); i-- > 0;)
            {
                const auto& Family = vQueueFamilies[i];
                if( ( Family.type & type ) == type )
                {
                    // Calc next queue index like: 0,1,2,3...0,1,2,3
                    const uint32_t currentQueueCount = m_vQueues.GetCount();
                    const uint32_t idx = (currentQueueCount) % Family.vQueues.GetCount();
                    DDIQueue hDDIQueue = Family.vQueues[idx];
                    CQueue* pQueue = nullptr;

                    // Find if this queue is already being used
                    for( uint32_t i = 0; i < currentQueueCount; ++i )
                    {
                        if( m_vQueues[ i ].GetDDIObject() == hDDIQueue )
                        {
                            pQueue = &m_vQueues[ i ];
                            break;
                        }
                    }
                    if( pQueue == nullptr )
                    {
                        CQueue Queue;
                        SQueueInitInfo Info;
                        Info.hDDIQueue = Family.vQueues[idx]; // get next queue
                        Info.familyIndex = Family.index;
                        Info.type = Family.type;
                        Info.pContext = this;
                        Queue.Init( Info );
                        m_vQueues.PushBack( Queue );
                        pQueue = &m_vQueues.Back();
                        VKE_LOG( "Acquire Queue: " << Info.hDDIQueue << " of type: " << type );

                        Result res;
                        {
                            SSubmitManagerDesc Desc;
                            Desc.pCtx = this;
                            res = pQueue->_CreateSubmitManager( &Desc );
                        }
                    }
                    
                    pRet = QueueRefPtr( pQueue );
                    pRet->_AddContextRef(); // add ref count for another context
                    break;
                }
            }

            return pRet;
        }

        VkInstance CDeviceContext::_GetInstance() const
        {
            //return m_pRenderSystem->_GetInstance();
            return VK_NULL_HANDLE;
        }

        void CDeviceContext::_NotifyDestroy(CGraphicsContext* pCtx)
        {
            VKE_ASSERT( pCtx != nullptr, "GraphicsContext must not be destroyed." );
            VKE_ASSERT( pCtx->_GetQueue().IsValid(), "Queue must not be destroyed." );
            //if( pCtx->m_pQueue->GetRefCount() > 0 )
            {
                pCtx->/*m_BaseCtx.*/m_pQueue = nullptr;
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

        PipelineRefPtr CDeviceContext::CreatePipeline( const SPipelineCreateDesc& Desc )
        {
            return m_pPipelineMgr->CreatePipeline( Desc );
        }

        RenderPassHandle CDeviceContext::CreateRenderPass(const SRenderPassDesc& Desc)
        {
            return _CreateRenderPass( Desc, false );
        }

        RenderPassHandle CDeviceContext::_CreateRenderPass( const SRenderPassDesc& Desc, bool ddiHandles )
        {
            CRenderPass* pPass;
            RenderPassHandle hRet = NULL_HANDLE;
            hash_t hash = CRenderPass::CalcHash( Desc );
            auto Itr = m_mRenderPasses.find( hash );
            if( Itr != m_mRenderPasses.end() )
            {
                hRet.handle = hash;
            }
            else
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pPass, this ) ) )
                {
                    m_mRenderPasses[hash] = pPass;

                    Result res = VKE_FAIL;
                    {
                        res = pPass->Create( Desc );
                    }

                    if( VKE_SUCCEEDED( res ) )
                    {
                        hRet.handle = hash;
                        pPass->m_hObject = hRet.handle;
                    }
                    else
                    {
                        Memory::DestroyObject( &HeapAllocator, &pPass );
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to create memory for render pass." );
                }
            }
            return hRet;
        }

        void CDeviceContext::_DestroyRenderPasses()
        {
            for( auto& Pair : m_mRenderPasses )
            {
                auto pCurr = Pair.second.Release();
                pCurr->_Destroy( true );
                Memory::DestroyObject( &HeapAllocator, &pCurr );
            }
            m_mRenderPasses.clear();
        }

        RenderPassRefPtr CDeviceContext::GetRenderPass(const RenderPassHandle& hPass)
        {
            return m_mRenderPasses[hPass.handle];
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
            //m_pPrivate->ICD.Device.vkCmdBindPipeline( pCmdBuffer->m_vkCommandBuffer, vkBind, pPipeline->m_vkPipeline );
            SBindPipelineInfo Info;
            Info.pCmdBuffer = pCmdBuffer.Get();
            Info.pPipeline = pPipeline.Get();
            m_DDI.Bind( Info );
        }

        PipelineLayoutRefPtr CDeviceContext::GetPipelineLayout( PipelineLayoutHandle hLayout )
        {
            return m_pPipelineMgr->GetPipelineLayout( hLayout );
        }

        ShaderRefPtr CDeviceContext::CreateShader(const SCreateShaderDesc& Desc)
        {
            return m_pShaderMgr->CreateShader(Desc);
        }

        DescriptorSetLayoutHandle CDeviceContext::CreateDescriptorSetLayout(const SDescriptorSetLayoutDesc& Desc)
        {
            return m_pDescSetMgr->CreateLayout( Desc );
        }

        BufferRefPtr CDeviceContext::CreateBuffer( const SCreateBufferDesc& Desc )
        {
            return m_pBufferMgr->CreateBuffer( Desc );
        }

        void CDeviceContext::DestroyBuffer( BufferPtr* ppInOut )
        {
            m_pBufferMgr->DestroyBuffer( ppInOut );
        }

        ShaderRefPtr CDeviceContext::GetShader( ShaderHandle hShader )
        {
            return m_pShaderMgr->GetShader( hShader );
        }

        DDIDescriptorSetLayout CDeviceContext::GetDescriptorSetLayout( DescriptorSetLayoutHandle hSet )
        {
            return m_pDescSetMgr->GetLayout( hSet );
        }

        PipelineRefPtr CDeviceContext::GetPipeline( PipelineHandle hPipeline )
        {
            return m_pPipelineMgr->GetPipeline( hPipeline );
        }

        BufferRefPtr CDeviceContext::GetBuffer( BufferHandle hBuffer )
        {
            return m_pBufferMgr->GetBuffer( hBuffer );
        }

        BufferRefPtr CDeviceContext::GetBuffer( const VertexBufferHandle& hBuffer )
        {
            return m_pBufferMgr->GetBuffer( hBuffer );
        }

        BufferRefPtr CDeviceContext::GetBuffer( const IndexBufferHandle& hBuffer )
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

        TextureRefPtr CDeviceContext::GetTexture( const RenderTargetHandle& hRT )
        {
            RenderTargetPtr pRT = m_pTextureMgr->GetRenderTarget( hRT );
            VKE_ASSERT( pRT != nullptr, "" );
            TextureRefPtr pTex = m_pTextureMgr->GetTexture( pRT->GetTexture() );
            return pTex;
        }

        TextureViewRefPtr CDeviceContext::GetTextureView( const TextureViewHandle& hView )
        {
            return m_pTextureMgr->GetTextureView( hView );
        }

        TextureViewRefPtr CDeviceContext::GetTextureView( const RenderTargetHandle& hRT )
        {
            return m_pTextureMgr->GetTextureView( hRT );
        }

        TextureViewRefPtr CDeviceContext::GetTextureView( const TextureHandle& hTexture )
        {
            return m_pTextureMgr->GetTextureView( hTexture );
        }

        void CDeviceContext::DestroyTexture( TextureHandle hTex )
        {
            m_pTextureMgr->DestroyTexture( &hTex );
        }

        TextureViewHandle CDeviceContext::CreateTextureView( const SCreateTextureViewDesc& Desc )
        {
            return m_pTextureMgr->CreateTextureView( Desc.TextureView );
        }

        RenderTargetHandle CDeviceContext::CreateRenderTarget( const SRenderTargetDesc& Desc )
        {
            return m_pTextureMgr->CreateRenderTarget( Desc );
        }

        RenderTargetRefPtr CDeviceContext::GetRenderTarget( const RenderTargetHandle& hRT )
        {
            return m_pTextureMgr->GetRenderTarget( hRT );
        }

        void CDeviceContext::DestroyRenderTarget( RenderTargetHandle* phRT )
        {
            m_pTextureMgr->DestroyRenderTarget( phRT );
        }

        SamplerHandle CDeviceContext::CreateSampler( const SSamplerDesc& Desc )
        {
            return m_pTextureMgr->CreateSampler( Desc );
        }

        SamplerRefPtr CDeviceContext::GetSampler( const SamplerHandle& hSampler )
        {
            return m_pTextureMgr->GetSampler( hSampler );
        }

        void CDeviceContext::DestroySampler( SamplerHandle* phSampler )
        {
            m_pTextureMgr->DestroySampler( phSampler );
        }

        Result CDeviceContext::_CreateCommandBuffers( const handle_t& hPool, uint32_t count, CCommandBuffer** ppArray )
        {
            return m_CmdBuffMgr.CreateCommandBuffers< VKE_THREAD_SAFE >( hPool, count, ppArray );
        }

        void CDeviceContext::_FreeCommandBuffers( const handle_t& hPool, uint32_t count, CCommandBuffer** ppArray )
        {
            m_CmdBuffMgr.FreeCommandBuffers< VKE_THREAD_SAFE >( hPool, count, ppArray );
        }

        ShaderPtr CDeviceContext::GetDefaultShader( SHADER_TYPE type )
        {
            return m_pShaderMgr->GetDefaultShader( type );
        }

        DescriptorSetLayoutHandle CDeviceContext::GetDefaultDescriptorSetLayout()
        {
            return m_pDescSetMgr->GetDefaultLayout();
        }

        PipelineLayoutPtr CDeviceContext::GetDefaultPipelineLayout()
        {
            return m_pPipelineMgr->GetDefaultLayout();
        }

        Result CDeviceContext::ExecuteRemainingWork()
        {
            Result ret = VKE_FAIL;
            VKE_ASSERT( m_pCurrentCommandBuffer != nullptr && m_pCurrentCommandBuffer->GetState() == CCommandBuffer::States::BEGIN, "" );
            ret = m_pCurrentCommandBuffer->End( CommandBufferEndFlags::EXECUTE | CommandBufferEndFlags::WAIT | CommandBufferEndFlags::DONT_SIGNAL_SEMAPHORE, nullptr );
            return ret;
        }

        void CDeviceContext::_PushSignaledSemaphore( const DDISemaphore& hDDISemaphore )
        {
            Threads::ScopedLock l( m_SignaledSemaphoreSyncObj );
            m_vDDISignaledSemaphores.PushBack( hDDISemaphore );
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