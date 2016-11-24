#pragma once
#include "Core/VKEPreprocessor.h"
#if VKE_VULKAN_RENDERER
#include "Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Vulkan/CResourceManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CRenderSystem;
        class CGraphicsContext;
        class CComputeContext;
        class CDataTransferContext;
        class CResourceManager;

        class VKE_API CDeviceContext
        {
            struct SInternalData;
            friend class CRenderSystem;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CDataTransferContext;
            friend class CResourceManager;

        public:
            using GraphicsContextArray = Utils::TCDynamicArray< CGraphicsContext* >;
            using ComputeContextArray = Utils::TCDynamicArray< CComputeContext* >;
            using DataTransferContextArray = Utils::TCDynamicArray< CDataTransferContext* >;
            using RenderTargetArray = Utils::TCDynamicArray< CRenderTarget* >;

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

            public:

                CDeviceContext(CRenderSystem*);
                ~CDeviceContext();

                Result  Create(const SDeviceContextDesc& Desc);
                void    Destroy();

                CGraphicsContext*   CreateGraphicsContext(const SGraphicsContextDesc& Desc);
                CComputeContext*    CreateComputeContext(const SComputeContextDesc& Desc);
                CDataTransferContext*   CreateDataTransferContext(const SDataTransferContextDesc& Desc);

                //const GraphicsContextArray& GetGraphicsContexts() const { return m_vGraphicsContexts; }
                //const ComputeContextArray& GetComputeContexts() const { return m_vComputeContexts; }
                //const DataTransferContextArray& GetDataTransferContexts() const { return m_vDataTransferContexts; }

                CRenderSystem*  GetRenderSystem() const { return m_pRenderSystem; }

                handle_t CreateFramebuffer(const SFramebufferDesc& Desc);
                handle_t CreateTexture(const STextureDesc& Desc);
                handle_t CreateTextureView(const STextureViewDesc& Desc);
                handle_t CreateRenderPass(const SRenderPassDesc& Desc);
                handle_t CreateRenderTarget(const SRenderTargetDesc& Desc);

            protected:

                Vulkan::CDeviceWrapper& _GetDevice() const { return *m_pVkDevice; }
                Vulkan::ICD::Device&    _GetICD() const;
                CGraphicsContext*   _CreateGraphicsContext(const SGraphicsContextDesc&);
                VkInstance  _GetInstance() const;

                void        _NotifyDestroy(CGraphicsContext*);

            protected:

                SInternalData*              m_pPrivate = nullptr;
                CRenderSystem*              m_pRenderSystem = nullptr;
                GraphicsContextArray        m_vGraphicsContexts;
                Vulkan::CDeviceWrapper*     m_pVkDevice;
                CResourceManager            m_ResMgr;
                RenderTargetArray           m_vpRenderTargets;
                //ComputeContextArray         m_vComputeContexts;
                //DataTransferContextArray    m_vDataTransferContexts;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER