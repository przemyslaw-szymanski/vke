#pragma once

#include "Core/VKEForwardDeclarations.h"
#include "RenderSystem/Vulkan/Common.h"
#include "Vulkan.h"
#include "CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CContext;
        class CDeviceContext;
        class CSwapChain;
        class CCommandBuffer;
        class CCommandBufferManager;

        struct SDeviceInternal;

        class CDevice
        {
            friend class CRenderSystem;
            friend class CContext;

            public:

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

            using QueueVec = vke_vector< VkQueue >;
            using QueuePriorities = vke_vector< float >;
            using SwapChainVec = vke_vector< CSwapChain* >;
            using ContextVec = vke_vector< CContext* >;

            struct SQueueFamily
            {
                QueueVec        vQueues;
                QueuePriorities vPriorities;
                uint32_t        index;
                bool            isGraphics;
                bool            isCompute;
                bool            isTransfer;
                bool            isSparse;
            };

            using QueueFamilyVec = vke_vector< SQueueFamily >;
            using UintVec = vke_vector< uint32_t >;
           
            public:

                CDevice(CRenderSystem* pRS);
                ~CDevice();

                Result Create(const SDeviceInfo& Info);
                Result CreateSwapChain(const SSwapChainInfo& Info);
                void Destroy();

                Result              CreateContext(const SContextInfo& Info);

                VkDevice            GetAPIDevice() const;

                VkPhysicalDevice    GetPhysicalDevice() const;

                

                VkInstance          GetAPIInstance() const;

                vke_force_inline
                CRenderSystem*      GetRenderSystem() const { return m_pRenderSystem; }

                uint32_t            GetQueueIndex(QUEUE_TYPE eType) const;

                VkQueue             GetQueue(uint32_t familyIndex) const;

                VkQueue             GetQueue(QUEUE_TYPE eType) const;

                vke_force_inline
                VkQueue             GetQueue(uint32_t familyIndex, uint32_t index) const
                                    { return m_vQueueFamilies[familyIndex].vQueues[index]; }

                const
                ICD::Device& GetDeviceFunctions() const;
                const
                ICD::Global& GetGlobalFunctions() const;
                const
                ICD::Instance&   GetInstanceFunctions() const;

                Result          BeginFrame(const CSwapChain*);
                Result          EndFrame(const CSwapChain*);
                Result          SubmitFrame(const CSwapChain*);

                CommandBufferPtr    CreateCommandBuffer();
                Result              DestroyCommandBuffer(CommandBufferPtr* ppOut);

            protected:

                Result       _GetProperties(VkPhysicalDevice vkPhysicalDevice);
                Result       _CheckExtensions(VkPhysicalDevice vkPhysicalDevice, cstr_t* ppExtensions,
                                             uint32_t extCount);

                Result          _CreateContexts();

                CDeviceContext* _GetDeviceContext() const;

            protected:

                SDeviceInfo             m_Info;
                VkFormatProperties      m_aFormatsProperties[ImageFormats::_MAX_COUNT];
                QueueFamilyVec          m_vQueueFamilies;
                UintVec                 m_aQueueTypes[QueueTypes::_MAX_COUNT];
                VkPhysicalDeviceMemoryProperties    m_vkMemProperty;
                VkPhysicalDeviceFeatures            m_vkFeatures;
                CRenderSystem*          m_pRenderSystem = nullptr;
                SwapChainVec            m_vpSwapChains;
                ContextVec              m_vpContexts;
                SDeviceInternal*        m_pInternal = nullptr;
                CCommandBufferManager*  m_pCmdBuffMgr = nullptr;
                uint32_t                m_queueFamilyIndex = 0;
                VKE_DEBUG_CODE(VkDeviceCreateInfo m_DeviceInfo);
        };
    } // RenderSystem
} // VKE
