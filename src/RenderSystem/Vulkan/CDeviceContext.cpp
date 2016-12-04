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

namespace VKE
{
    namespace RenderSystem
    {
        template<typename T>
        using ResourceBuffer = Utils::TCDynamicArray< T, 256 >;

        template<typename VkObj, typename VkCreateInfo>
        struct TSVkObject
        {
            VkObj   handle;
            VKE_DEBUG_CODE(VkCreateInfo CreateInfo);
        };

        struct QueueTypes
        {
            enum TYPE
            {
                GRAPHICS,
                COMPUTE,
                TRANSFER,
                SPARSE,
                _MAX_COUNT
            };
        };

        using QUEUE_TYPE = QueueTypes::TYPE;

        static const uint32_t DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT = 16;
        using QueueArray = Utils::TCDynamicArray< Vulkan::SQueue, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueuePriorityArray = Utils::TCDynamicArray< float, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueueFamilyPropertyArray = Utils::TCDynamicArray< VkQueueFamilyProperties, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;

        struct SQueueFamily
        {
            QueueArray          vQueues;
            QueuePriorityArray  vPriorities;
            uint32_t            index;
            bool                isGraphics;
            bool                isCompute;
            bool                isTransfer;
            bool                isSparse;
            bool                isPresent;
        };

        struct SPrivateToDeviceCtx
        {
            Vulkan::ICD::Instance&   ICD;

            SPrivateToDeviceCtx(Vulkan::ICD::Instance& I) : ICD(I) {}
            void operator=(const SPrivateToDeviceCtx&) = delete;
        };

        using QueueFamilyArray = Utils::TCDynamicArray< SQueueFamily >;
        using UintArray = Utils::TCDynamicArray< uint32_t, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
        using QueueTypeArray = UintArray[ QueueTypes::_MAX_COUNT ];

        struct SDeviceProperties
        {
            QueueFamilyPropertyArray            vQueueFamilyProperties;
            QueueFamilyArray                    vQueueFamilies;
            VkFormatProperties                  aFormatProperties[ TextureFormats::_MAX_COUNT ];
            VkPhysicalDeviceMemoryProperties    vkMemProperties;
            VkPhysicalDeviceProperties          vkProperties;
            VkPhysicalDeviceFeatures            vkFeatures;

            void operator=( const SDeviceProperties& Rhs )
            {
                vQueueFamilyProperties = Rhs.vQueueFamilyProperties;
                vQueueFamilies = Rhs.vQueueFamilies;

                Memory::Copy<VkFormatProperties, TextureFormats::_MAX_COUNT>( aFormatProperties, Rhs.aFormatProperties );
                Memory::Copy( &vkMemProperties, &Rhs.vkMemProperties );
                Memory::Copy( &vkProperties, &Rhs.vkProperties );
                Memory::Copy( &vkFeatures, &Rhs.vkFeatures );
            }
        };

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

                    Status _OnStart(uint32_t threadId)
                    {
                        pGraphicsCtxOut = pCtx->_CreateGraphicsContext(Desc);
                        return Status::OK;
                    }

                    void _OnGet(void* pOut)
                    {
                        pOut = pGraphicsCtxOut;
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
            m_pRenderSystem(pRS),
            m_ResMgr(this)
        {}

        CDeviceContext::~CDeviceContext()
        {
            Destroy();
        }

        void CDeviceContext::Destroy()
        {
            for( auto& pRT : m_vpRenderTargets )
            {
                Memory::DestroyObject(&HeapAllocator, &pRT);
            }
            m_vpRenderTargets.FastClear();

            for( auto& pRP : m_vpRenderingPipelines )
            {
                Memory::DestroyObject(&HeapAllocator, &pRP);
            }
            m_vpRenderingPipelines.FastClear();

            for( auto& pCtx : m_vGraphicsContexts )
            {
                Memory::DestroyObject(&HeapAllocator, &pCtx);
            }

            m_vGraphicsContexts.FastClear();
            Memory::DestroyObject(&HeapAllocator, &m_pPrivate);
            Memory::DestroyObject(&HeapAllocator, &m_pVkDevice);
        }

        Result CDeviceContext::Create(const SDeviceContextDesc& Desc)
        {
            const SPrivateToDeviceCtx* pPrivate = reinterpret_cast< const SPrivateToDeviceCtx* >(Desc.pPrivate);
      
            assert(m_pPrivate == nullptr);
            Vulkan::ICD::Device ICD = { pPrivate->ICD.Global, pPrivate->ICD.Instance };

            Utils::TCDynamicArray< const char* > vExtensions;
            vExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

            VkPhysicalDevice vkPhysicalDevice = reinterpret_cast<VkPhysicalDevice>(Desc.pAdapterInfo->handle);
            //VkInstance vkInstance = reinterpret_cast<VkInstance>(Desc.hAPIInstance);
            VkInstance vkInstance = m_pRenderSystem->_GetVkInstance();

            SPropertiesInput In = { ICD.Instance };
            In.vkPhysicalDevice = vkPhysicalDevice;
            SDeviceProperties DevProps;

            VKE_RETURN_IF_FAILED(GetProperties(In, &DevProps));
            VKE_RETURN_IF_FAILED(CheckExtensions(vkPhysicalDevice, ICD.Instance, vExtensions));

            Utils::TCDynamicArray<VkDeviceQueueCreateInfo> vQis;
            for (auto& Family : DevProps.vQueueFamilies)
            {
                if (!Family.vQueues.IsEmpty())
                {
                    VkDeviceQueueCreateInfo qi;
                    Vulkan::InitInfo(&qi, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
                    qi.flags = 0;
                    qi.pQueuePriorities = &Family.vPriorities[0];
                    qi.queueFamilyIndex = Family.index;
                    qi.queueCount = static_cast<uint32_t>(Family.vQueues.GetCount());
                    vQis.PushBack(qi);
                }
            }

            //VkPhysicalDeviceFeatures df = {};

            VkDeviceCreateInfo di;
            Vulkan::InitInfo(&di, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);
            di.enabledExtensionCount = vExtensions.GetCount();
            di.enabledLayerCount = 0;
            di.pEnabledFeatures = nullptr;
            di.ppEnabledExtensionNames = &vExtensions[0];
            di.ppEnabledLayerNames = nullptr;
            di.pQueueCreateInfos = &vQis[0];
            di.queueCreateInfoCount = static_cast<uint32_t>(vQis.GetCount());
            di.flags = 0;

            VkDevice vkDevice;
            VK_ERR(ICD.Instance.vkCreateDevice(vkPhysicalDevice, &di, nullptr, &vkDevice));
            
            VKE_RETURN_IF_FAILED(Vulkan::LoadDeviceFunctions(vkDevice, ICD.Instance, &ICD.Device));
            
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pPrivate, vkDevice, ICD));

            for (auto& Family : DevProps.vQueueFamilies)
            {
                for (uint32_t q = 0; q < Family.vQueues.GetCount(); ++q)
                {
                    VkQueue vkQueue;
                    ICD.Device.vkGetDeviceQueue(vkDevice, Family.index, q, &vkQueue);
                    Family.vQueues[ q ].vkQueue = vkQueue;
                    Family.vQueues[ q ].familyIndex = Family.index;
                }
            }

            m_pPrivate->Desc = Desc;
            m_pPrivate->Properties = DevProps;
            m_pPrivate->Vulkan.vkDevice = vkDevice;
            m_pPrivate->Vulkan.vkPhysicalDevice = vkPhysicalDevice;
            m_pPrivate->Vulkan.vkInstance = vkInstance;

            if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &m_pVkDevice, vkDevice, ICD.Device)) )
            {
                return VKE_ENOMEMORY;
            }

            {
                RenderSystem::SResourceManagerDesc Desc;
                
                if( VKE_FAILED(m_ResMgr.Create(Desc)) )
                {
                    return VKE_FAIL;
                }
            }
            
            m_vpRenderTargets.PushBack(nullptr);

            return VKE_OK;
        }

        CGraphicsContext* CDeviceContext::CreateGraphicsContext(const SGraphicsContextDesc& Desc)
        {
            SInternalData::Tasks::CreateGraphicsContext CreateGraphicsContextTask;
            CreateGraphicsContextTask.Desc = Desc;
            CreateGraphicsContextTask.pCtx = this;
            m_pRenderSystem->GetEngine()->GetThreadPool()->AddTask(Constants::Threads::ID_BALANCED,
                                                                   &CreateGraphicsContextTask);
            CGraphicsContext* pCtx;
            CreateGraphicsContextTask.Get(&pCtx);
            return pCtx;
        }

        CGraphicsContext* CDeviceContext::_CreateGraphicsContext(const SGraphicsContextDesc& Desc)
        {
            // Find context
            for( auto pCtx : m_vGraphicsContexts )
            {
                if( pCtx->GetDesc().SwapChainDesc.hWnd == Desc.SwapChainDesc.hWnd )
                {
                    VKE_LOG_ERR("Graphics context for window: " << Desc.SwapChainDesc.hWnd << " already created.");
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
                    if( Desc.SwapChainDesc.hWnd )
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
            Vulkan::SQueue* pQueue = &vQueues[ m_vGraphicsContexts.GetCount() % vQueues.GetCount() ];
            // Add reference count. If refCount > 1 multithreaded case should be handled as more than
            // one graphicsContext refers to this queue
            // _AddRef/_RemoveRef should not be called externally
            pQueue->_AddRef();
            if( Desc.SwapChainDesc.hWnd )
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
            if( m_vGraphicsContexts.PushBack(pCtx) == Utils::INVALID_POSITION )
            {
                VKE_LOG_ERR("Unable to add GraphicsContext to the buffer.");
                Memory::DestroyObject(&HeapAllocator, &pCtx);
            }
            return pCtx;
        }

        FramebufferHandle CDeviceContext::CreateFramebuffer(const SFramebufferDesc& Desc)
        {
            return m_ResMgr.CreateFramebuffer(Desc);
        }

        TextureHandle CDeviceContext::CreateTexture(const STextureDesc& Desc)
        {
            return m_ResMgr.CreateTexture(Desc);
        }

        TextureViewHandle CDeviceContext::CreateTextureView(const STextureViewDesc& Desc)
        {
            return m_ResMgr.CreateTextureView(Desc);
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
            }
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

        void ConvertTexDescToAttachmentDesc(const VkImageCreateInfo& TexDesc,
                                            RENDER_TARGET_WRITE_ATTACHMENT_USAGE usage,
                                            VkAttachmentDescription* pOut)
        {
            pOut->initialLayout = Vulkan::Convert::ImageUsageToInitialLayout(TexDesc.usage);
            pOut->finalLayout = Vulkan::Convert::ImageUsageToFinalLayout(TexDesc.usage);
            pOut->format = TexDesc.format;
            pOut->loadOp = Vulkan::Convert::UsageToLoadOp(usage);
            pOut->storeOp = Vulkan::Convert::UsageToStoreOp(usage);
            pOut->samples = TexDesc.samples;
        }

        void ConvertTexDescToAttachmentDesc(const VkImageCreateInfo& TexDesc,
                                            RENDER_TARGET_READ_ATTACHMENT_USAGE usage,
                                            VkAttachmentDescription* pOut)
        {
            pOut->initialLayout = Vulkan::Convert::ImageUsageToInitialLayout(TexDesc.usage);
            pOut->finalLayout = Vulkan::Convert::ImageUsageToFinalLayout(pOut->initialLayout);
            pOut->format = TexDesc.format;
            pOut->loadOp = Vulkan::Convert::UsageToLoadOp(usage);
            pOut->storeOp = Vulkan::Convert::UsageToStoreOp(usage);
            pOut->samples = TexDesc.samples;
        }

        RenderTargetHandle CDeviceContext::CreateRenderTarget(const SRenderTargetDesc& Desc)
        {
            CRenderTarget* pRT;
            if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pRT, this)) )
            {
                VKE_LOG_ERR("Unable to create memory for render target.");
                return NULL_HANDLE;
            }

            const auto& vWriteAttachments = Desc.vWriteAttachments;
            const auto& vReadAttachments = Desc.vReadAttachments;

            Utils::TCDynamicArray< const VkImageCreateInfo* > vpTexDescs;
            Utils::TCDynamicArray< VkAttachmentDescription > vVkAtDescs;
            using AttachmentRefArray = Utils::TCDynamicArray < VkAttachmentReference >;

            AttachmentRefArray vWriteColorRefs;
            VkAttachmentReference vkDepthWriteRef, vkDepthReadRef;
            AttachmentRefArray vInputRefs;

            VkAttachmentDescription vkAtDesc;
            vkAtDesc.flags = 0;
            vkAtDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            vkAtDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            uint32_t writeCount = 0, readCount = 0;

            for( uint32_t i = 0; i < vWriteAttachments.GetCount(); ++i )
            {
                const auto& AtDesc = vWriteAttachments[ i ];
                const VkImageCreateInfo* pTexDesc;

                if( !AtDesc.hTextureView )
                {
                    VkImage vkImage;
                    auto hTex = m_ResMgr.CreateTexture(AtDesc.TexDesc, &vkImage);
                    assert(hTex);
                    VkImageView vkView;
                    auto hView = m_ResMgr.CreateTextureView(hTex, &vkView);
                    assert(hView);
                    pTexDesc = &m_ResMgr.GetTextureDesc(hTex);
                    pRT->m_vImgViews.PushBack(vkView);
                    pRT->m_vTextureHandles.PushBack(hTex);
                    pRT->m_vTextureViewHandles.PushBack(hView);
                }
                else
                {
                    assert(!AtDesc.hTextureView.IsNativeHandle() && "Native handles are not supported here");
                    VkImageView vkView = m_ResMgr.GetTextureView(AtDesc.hTextureView);
                    const auto& ViewDesc = m_ResMgr.GetTextureViewDesc(AtDesc.hTextureView);
                    VkImage vkImg = ViewDesc.image;
                    pRT->m_vImgViews.PushBack(vkView);

                    auto hTex = m_ResMgr._FindTexture(vkImg);
                    if( hTex )
                    {
                        pRT->m_vTextureHandles.PushBack(hTex);
                        pTexDesc = &m_ResMgr.GetTextureDesc(hTex);
                    }
                    else
                    {
                        pTexDesc = &m_ResMgr.GetTextureDesc(vkImg);
                    }

                    pRT->m_vTextureViewHandles.PushBack(AtDesc.hTextureView);
                }

                ConvertTexDescToAttachmentDesc(*pTexDesc, AtDesc.usage, &vkAtDesc);
                writeCount = vVkAtDescs.PushBack(vkAtDesc);

                VkAttachmentReference Ref;
                Ref.attachment = writeCount;
                Ref.layout = vkAtDesc.initialLayout;
                vWriteColorRefs.PushBack(Ref);
            }

            for( uint32_t i = 0; i < vReadAttachments.GetCount(); ++i )
            {
                // Find if this attachment is as write attachment
                bool bFoundWrite = false;
                for( uint32_t j = 0; j < vWriteAttachments.GetCount(); ++j )
                {
                    if( vWriteAttachments[ j ].hTextureView == vReadAttachments[ i ].hTextureView )
                    {
                        const auto& Ref = vWriteColorRefs[ j ];
                        vInputRefs.PushBack(Ref);
                        bFoundWrite = true;
                        break;
                    }
                }

                if(bFoundWrite)
                    continue;

                const auto& AtDesc = vReadAttachments[ i ];         
                const auto& TexDesc = m_ResMgr.FindTextureDesc(AtDesc.hTextureView);
                ConvertTexDescToAttachmentDesc(TexDesc, AtDesc.usage, &vkAtDesc);
                readCount = vVkAtDescs.PushBack(vkAtDesc);
                
                VkAttachmentReference Ref;
                Ref.attachment = readCount;
                Ref.layout = vkAtDesc.initialLayout;
                vInputRefs.PushBack(Ref);

                VkImageView vkView = m_ResMgr.GetTextureView(AtDesc.hTextureView);
                pRT->m_vImgViews.PushBack(vkView);
            }

            for( uint32_t i = 0; i < vVkAtDescs.GetCount(); ++i )
            {
                const auto& AtDesc = vVkAtDescs[ i ];             
            }

            if( Desc.DepthStencilAttachment.hWrite )
            {
                const auto& TexDesc = m_ResMgr.FindTextureDesc(Desc.DepthStencilAttachment.hWrite);
                ConvertTexDescToAttachmentDesc(TexDesc, Desc.DepthStencilAttachment.usage, &vkAtDesc);       
                vkDepthWriteRef.attachment = vVkAtDescs.PushBack(vkAtDesc);
                vkDepthWriteRef.layout = vkAtDesc.finalLayout;

                VkImageView vkView = m_ResMgr.GetTextureView(Desc.DepthStencilAttachment.hWrite);
                pRT->m_vImgViews.PushBack(vkView);
            }

            if( Desc.DepthStencilAttachment.hRead )
            {
                if( Desc.DepthStencilAttachment.hRead == Desc.DepthStencilAttachment.hWrite )
                {
                    vkDepthReadRef = vkDepthWriteRef;
                    vInputRefs.PushBack(vkDepthReadRef);
                }
                else if( !Desc.DepthStencilAttachment.hWrite )
                {
                    const auto& TexDesc = m_ResMgr.FindTextureDesc(Desc.DepthStencilAttachment.hRead);
                    ConvertTexDescToAttachmentDesc(TexDesc, Desc.DepthStencilAttachment.usage, &vkAtDesc);
                    vkDepthReadRef.attachment = vVkAtDescs.PushBack(vkAtDesc);
                    vkDepthReadRef.layout = vkAtDesc.finalLayout;
                    vInputRefs.PushBack(vkDepthReadRef);
                    VkImageView vkView = m_ResMgr.GetTextureView(Desc.DepthStencilAttachment.hRead);
                    pRT->m_vImgViews.PushBack(vkView);
                }
            }

            /// @todo for now only one subpass is supported
            VkSubpassDescription vkSpDesc = { 0 };
            vkSpDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            if( !vWriteColorRefs.IsEmpty() )
            {
                vkSpDesc.colorAttachmentCount = vWriteColorRefs.GetCount();
                vkSpDesc.pColorAttachments = &vWriteColorRefs[ 0 ];
            }
            if( !vInputRefs.IsEmpty() )
            {
                vkSpDesc.inputAttachmentCount = vInputRefs.GetCount();
                vkSpDesc.pInputAttachments = &vInputRefs[ 0 ];
            }
            if( Desc.DepthStencilAttachment.hWrite )
                vkSpDesc.pDepthStencilAttachment = &vkDepthWriteRef;

            {
                VkRenderPassCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
                ci.attachmentCount = vVkAtDescs.GetCount();
                ci.pAttachments = &vVkAtDescs[ 0 ];
                ci.dependencyCount = 0;
                ci.pDependencies = nullptr;
                ci.subpassCount = 1;
                ci.pSubpasses = &vkSpDesc;
                ci.flags = 0;
                VK_ERR(m_pVkDevice->CreateObject(ci, nullptr, &pRT->m_vkRenderPass));
            }
            {
                VkFramebufferCreateInfo ci;
                Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
                ci.flags = 0;
                ci.attachmentCount = pRT->m_vImgViews.GetCount();
                ci.pAttachments = &pRT->m_vImgViews[ 0 ];
                ci.width = Desc.Size.width;
                ci.height = Desc.Size.height;
                ci.layers = 1;
                ci.renderPass = pRT->m_vkRenderPass;
                VK_ERR(m_pVkDevice->CreateObject(ci, nullptr, &pRT->m_vkFramebuffer));
            }

            return RenderTargetHandle( m_vpRenderTargets.PushBack(pRT) );
        }

        RenderPassHandle CDeviceContext::CreateRenderPass(const SRenderPassDesc& Desc)
        {
            CRenderPass* pPass;
            if( VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pPass)) )
            {
                return NULL_HANDLE;
            }
            pPass->m_Desc = Desc;
            return RenderPassHandle( m_vpRenderPasses.PushBack(pPass) );
        }

        RenderingPipelineHandle CDeviceContext::CreateRenderingPipeline(const SRenderingPipelineDesc& Desc)
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
        }

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
                uint32_t isGraphics = aProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                uint32_t isCompute = aProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
                uint32_t isTransfer = aProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
                uint32_t isSparse = aProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;
                bool isPresent = false;
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
                Family.isGraphics = true;
                Family.isCompute = isCompute != 0;
                Family.isTransfer = isTransfer != 0;
                Family.isSparse = isSparse != 0;
                Family.isPresent = isPresent;

                vQueueFamilies.PushBack(Family);
            }

            Instance.vkGetPhysicalDeviceMemoryProperties(In.vkPhysicalDevice, &pOut->vkMemProperties);
            Instance.vkGetPhysicalDeviceFeatures(In.vkPhysicalDevice, &pOut->vkFeatures);

            for (uint32_t i = 0; i < RenderSystem::TextureFormats::_MAX_COUNT; ++i)
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