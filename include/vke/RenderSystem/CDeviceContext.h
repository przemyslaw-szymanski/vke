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
        class CRenderingPipeline;
        class CRenderPass;
        class CRenderSubPass;

        class VKE_API CDeviceContext
        {
            struct SInternalData;
            friend class CRenderSystem;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CDataTransferContext;
            friend class CResourceManager;
            friend class CRenderingPipeline;
            friend class CRenderSubPass;
            friend class CRenderTarget;

        public:
            using GraphicsContextArray = Utils::TCDynamicArray< CGraphicsContext* >;
            using ComputeContextArray = Utils::TCDynamicArray< CComputeContext* >;
            using DataTransferContextArray = Utils::TCDynamicArray< CDataTransferContext* >;
            using RenderTargetArray = Utils::TCDynamicArray< CRenderTarget* >;
            using RenderPassArray = Utils::TCDynamicArray< CRenderPass* >;
            using RenderingPipeilneArray = Utils::TCDynamicArray< CRenderingPipeline* >;

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

                FramebufferHandle CreateFramebuffer(const SFramebufferDesc& Desc);
                TextureHandle CreateTexture(const STextureDesc& Desc);
                TextureViewHandle CreateTextureView(const STextureViewDesc& Desc);
                RenderingPipelineHandle CreateRenderingPipeline(const SRenderingPipelineDesc& Desc);
                RenderTargetHandle CreateRenderTarget(const SRenderTargetDesc& Desc);
                Result UpdateRenderTarget(const RenderTargetHandle& hRT, const SRenderTargetDesc& Desc);

                CRenderTarget* GetRenderTarget(const RenderTargetHandle& hRenderTarget) const
                {
                    return m_vpRenderTargets[ hRenderTarget.handle ];
                }

                CResourceManager& GetResourceManager() { return m_ResMgr; }

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
                RenderPassArray             m_vpRenderPasses;
                RenderingPipeilneArray      m_vpRenderingPipelines;
                //ComputeContextArray         m_vComputeContexts;
                //DataTransferContextArray    m_vDataTransferContexts;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER