#ifndef __VKE_CDEVICE_H__
#define __VKE_CDEVICE_H__

#include "VKEForwardDeclarations.h"
#include "RenderSystem/Common.h"
#include "Vulkan.h"
#include "CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CContext;
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
                    _MAX_COUNT
                };
            };

            using QUEUE_TYPE = QueueTypes::TYPE;

            using QueueVec = vke_vector< VkQueue >;
            using QueuePriorities = vke_vector< float >;
            using SwapChainVec = vke_vector< CSwapChain* >;

            struct SQueueFamily
            {
                QueueVec        vQueues;
                QueuePriorities vPriorities;
                uint32_t        index;
                bool            isGraphics;
                bool            isCompute;
            };
           
            public:

                CDevice(CRenderSystem* pRS);
                ~CDevice();

                Result Create(const SDeviceInfo& Info);
                Result CreateSwapChain(const SSwapChainInfo& Info);
                void Destroy();

                vke_force_inline
                CSwapChain*         GetSwapChain(uint32_t id = 0) const { return m_vSwapChains[id]; }

                VkDevice            GetDevice() const;

                VkPhysicalDevice    GetPhysicalDevice() const;

                vke_force_inline const SQueueFamily*
                                    GetQueues() const { return m_aQueues; }

                vke_force_inline
                const SQueueFamily& GetQueue() const { return m_aQueues[0]; }

                VkInstance          GetInstance() const;

                vke_force_inline
                CRenderSystem*      GetRenderSystem() const { return m_pRenderSystem; }

                vke_force_inline
                const SQueueFamily& GetQueueFamily(QUEUE_TYPE type) const { return m_aQueues[type]; }

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

            protected:

                SDeviceInfo             m_Info;
                VkFormatProperties      m_aFormatsProperties[ImageFormats::_MAX_COUNT];
                SQueueFamily            m_aQueues[QueueTypes::_MAX_COUNT];
                VkPhysicalDeviceMemoryProperties    m_vkMemProperty;
                VkPhysicalDeviceFeatures            m_vkFeatures;
                CRenderSystem*          m_pRenderSystem = nullptr;
                SwapChainVec            m_vSwapChains;
                SDeviceInternal*        m_pInternal = nullptr;
                CCommandBufferManager*  m_pCmdBuffMgr = nullptr;
                uint32_t                m_queueFamilyIndex = 0;
                VKE_DEBUG_CODE(VkDeviceCreateInfo m_DeviceInfo);
        };
    } // RenderSystem
} // VKE

#endif // __VKE_CDEVICE_H__