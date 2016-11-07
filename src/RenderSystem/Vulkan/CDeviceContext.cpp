#include "RenderSystem/CDeviceContext.h"
#if VULKAN_RENDERER
#include "Vulkan.h"
#include "CVkDeviceWrapper.h"
#include "RenderSystem/CRenderSystem.h"
#include "Core/Utils/CLogger.h"

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
        using QueueArray = Utils::TCDynamicArray< VkQueue, DEFAULT_QUEUE_FAMILY_PROPERTY_COUNT >;
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
            QueueTypeArray                      avQueueTypes;
            VkFormatProperties                  aFormatProperties[ TextureFormats::_MAX_COUNT ];
            VkPhysicalDeviceMemoryProperties    vkMemProperties;
            VkPhysicalDeviceProperties          vkProperties;
            VkPhysicalDeviceFeatures            vkFeatures;

            void operator=( const SDeviceProperties& Rhs )
            {
                vQueueFamilyProperties = Rhs.vQueueFamilyProperties;
                vQueueFamilies = Rhs.vQueueFamilies;

                for( uint32_t i = 0; i < QueueTypes::_MAX_COUNT; ++i )
                {
                    avQueueTypes[ i ] = Rhs.avQueueTypes[ i ];
                }

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
            m_pRenderSystem(pRS)
        {}

        CDeviceContext::~CDeviceContext()
        {
            Destroy();
        }

        void CDeviceContext::Destroy()
        {

        }

        Result CDeviceContext::Create(const SDeviceContextDesc& Desc)
        {
            const SPrivateToDeviceCtx* pPrivate = reinterpret_cast< const SPrivateToDeviceCtx* >(Desc.pPrivate);

            assert(m_pInternal == nullptr);
            Vulkan::ICD::Device ICD = { pPrivate->ICD.Global, pPrivate->ICD.Instance };

            Utils::TCDynamicArray< const char* > vExtensions;
            vExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

            VkPhysicalDevice vkPhysicalDevice = reinterpret_cast<VkPhysicalDevice>(Desc.pAdapterInfo->handle);
            VkInstance vkInstance = reinterpret_cast<VkInstance>(Desc.hAPIInstance);

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
            
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pInternal, vkDevice, ICD));

            for (auto& Family : DevProps.vQueueFamilies)
            {
                for (uint32_t q = 0; q < Family.vQueues.GetCount(); ++q)
                {
                    auto& vkQueue = Family.vQueues[ q ];
                    ICD.Device.vkGetDeviceQueue(vkDevice, Family.index, q, &vkQueue);
                }
            }

            m_pInternal->Desc = Desc;
            m_pInternal->Properties = DevProps;
            m_pInternal->Vulkan.vkDevice = vkDevice;
            m_pInternal->Vulkan.vkPhysicalDevice = vkPhysicalDevice;

            //VKE_RETURN_IF_FAILED(_CreateContexts());

            return VKE_OK;
        }

        handle_t CDeviceContext::CreateFramebuffer(const SFramebufferDesc& Info)
        {
            handle_t hFb;
            VkFramebuffer vkFb;

            auto& Objects = m_pInternal->Objects;
            auto& VkWrapper = m_pInternal->VkWrapper;

            Utils::TCDynamicArray< VkImageView > vImageViews;
            for (uint32_t i = 0; i < Info.aTextureViews.count; ++i)
            {
                handle_t hImgView = Info.aTextureViews[i];
                vImageViews.PushBack(Objects.vImageViews[hImgView].handle);
            }

            const auto& RenderPass = Objects.vRenderPasses[Info.hRenderPass];

            VkFramebufferCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
            ci.width = Info.Size.width;
            ci.height = Info.Size.height;
            ci.layers = 1;
            ci.renderPass = RenderPass.handle;
            ci.attachmentCount = vImageViews.GetCount();
            ci.pAttachments = &vImageViews[0];

            assert(ci.renderPass != VK_NULL_HANDLE);
            assert(ci.attachmentCount == RenderPass.CreateInfo.attachmentCount);

            VkResult vkResult = VkWrapper.CreateObject(ci, nullptr, &vkFb);
            assert(vkResult == VK_SUCCESS);

            hFb = Objects.vFramebuffers.PushBack({ vkFb VKE_DEBUG_CODE(, ci) });

            if (hFb != Utils::INVALID_POSITION)
            {
                return hFb;
            }

            VkWrapper.DestroyObject(nullptr, &vkFb);
            return NULL_HANDLE;
        }

        handle_t CDeviceContext::CreateTexture(const STextureDesc& Info)
        {
            VkImage vkImg;
            VkImageCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);

            auto& VkWrpaper = m_pInternal->VkWrapper;
            VkResult vkResult = VkWrpaper.CreateObject(ci, nullptr, &vkImg);
            assert(vkResult == VK_SUCCESS);
            handle_t hImg = m_pInternal->Objects.vImages.PushBack({ vkImg VKE_DEBUG_CODE(, ci) });
            if (hImg != Utils::INVALID_POSITION)
            {
                return hImg;
            }
            VkWrpaper.DestroyObject(nullptr, &vkImg);
            return NULL_HANDLE;
        }

        handle_t CDeviceContext::CreateTextureView(const STextureViewDesc& Info)
        {
            VkImageView vkImgView;
            VkImageViewCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);

            auto& VkWrpaper = m_pInternal->VkWrapper;
            VkResult vkResult = VkWrpaper.CreateObject(ci, nullptr, &vkImgView);
            assert(vkResult == VK_SUCCESS);
            handle_t hImg = m_pInternal->Objects.vImageViews.PushBack({ vkImgView VKE_DEBUG_CODE(, ci) });
            if (hImg != Utils::INVALID_POSITION)
            {
                return hImg;
            }
            VkWrpaper.DestroyObject(nullptr, &vkImgView);
            return NULL_HANDLE;
        }

        handle_t CDeviceContext::CreateRenderPass(const SRenderPassDesc& Info)
        {
            Utils::TCDynamicArray< VkAttachmentDescription > vAttachmentDescs;
            Utils::TCDynamicArray< VkSubpassDescription > vSubpassDescs;

            for (uint32_t i = 0; i < Info.aAttachmentDescs.count; ++i)
            {
                const SAttachmentDesc& Desc = Info.aAttachmentDescs.pData[i];
                VkAttachmentDescription vkDesc;
                vkDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                vkDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                vkDesc.flags = 0;
                vkDesc.format = Vulkan::GetFormat(Desc.format);
                vkDesc.samples = Vulkan::GetSampleCount(Desc.multisampling);
                vkDesc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                vkDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                vkDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                vkDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                vAttachmentDescs.PushBack(vkDesc);
            }

            VkAttachmentReference ColorAttachmentRef;
            ColorAttachmentRef.attachment = 0;
            ColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            for (uint32_t i = 0; i < Info.aSubpassDescs.count; ++i)
            {
                const SSubpassDesc& Desc = Info.aSubpassDescs.pData[i];
                VkSubpassDescription vkDesc;
                vkDesc.flags = 0;
                vkDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                vkDesc.inputAttachmentCount = 0;
                vkDesc.pInputAttachments = nullptr;
                vkDesc.colorAttachmentCount = 1;
                vkDesc.pColorAttachments = &ColorAttachmentRef;
                vkDesc.pResolveAttachments = nullptr;
                vkDesc.pDepthStencilAttachment = nullptr;
                vkDesc.preserveAttachmentCount = 0;
                vkDesc.pPreserveAttachments = nullptr;
                vSubpassDescs.PushBack(vkDesc);
            }

            VkRenderPass vkRp;
            VkRenderPassCreateInfo ci;
            Vulkan::InitInfo(&ci, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
            ci.attachmentCount = vAttachmentDescs.GetCount();
            ci.pAttachments = &vAttachmentDescs[0];
            ci.dependencyCount = 0;
            ci.pDependencies = nullptr;
            ci.subpassCount = vSubpassDescs.GetCount();
            ci.pSubpasses = &vSubpassDescs[0];

            auto& Wrapper = m_pInternal->VkWrapper;
            VkResult vkResult = Wrapper.CreateObject(ci, nullptr, &vkRp);
            assert(vkResult == VK_SUCCESS);
            handle_t hImg = m_pInternal->Objects.vRenderPasses.PushBack({ vkRp VKE_DEBUG_CODE(, ci) });
            if (hImg != Utils::INVALID_POSITION)
            {
                return hImg;
            }
            Wrapper.DestroyObject(nullptr, &vkRp);
            return NULL_HANDLE;
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
            auto& avQueueTypes = pOut->avQueueTypes;
            auto& vQueueFamilies = pOut->vQueueFamilies;

            Instance.vkGetPhysicalDeviceQueueFamilyProperties(In.vkPhysicalDevice, &propCount, &aProperties[0]);
            // Choose a family index
            for (uint32_t i = 0; i < propCount; ++i)
            {
                uint32_t isGraphics = aProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
                uint32_t isCompute = aProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
                uint32_t isTransfer = aProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;
                uint32_t isSparse = aProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT;

                SQueueFamily Family;
                Family.vQueues.Resize(aProperties[i].queueCount, VK_NULL_HANDLE);
                Family.vPriorities.Resize(aProperties[i].queueCount, 1.0f);
                Family.index = i;
                Family.isGraphics = true;
                Family.isCompute = isCompute != 0;
                Family.isTransfer = isTransfer != 0;
                Family.isSparse = isSparse != 0;

                if (isGraphics)
                {
                    avQueueTypes[QueueTypes::GRAPHICS].PushBack(i);
                }
                if (isCompute)
                {
                    avQueueTypes[QueueTypes::COMPUTE].PushBack(i);
                }
                if (isTransfer)
                {
                    avQueueTypes[QueueTypes::TRANSFER].PushBack(i);
                }
                if (isSparse)
                {
                    avQueueTypes[QueueTypes::SPARSE].PushBack(i);
                }

                vQueueFamilies.PushBack(Family);
            }
            if (avQueueTypes[QueueTypes::GRAPHICS].IsEmpty())
            {
                VKE_LOG_ERR("Unable to find a graphics queue");
                return VKE_ENOTFOUND;
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
#endif // VULKAN_RENDERER