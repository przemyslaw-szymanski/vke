#pragma once
#include "Core/VKEPreprocessor.h"
#if VKE_VULKAN_RENDERER
#include "Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CDescriptorSet.h"

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
        class CDescriptorSetManager;

        class VKE_API CDeviceContext
        {
            struct SInternalData;
            friend class CRenderSystem;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CDataTransferContext;
            friend class CResourceManager;
            friend class CRenderingPipeline;
            friend class CRenderPass;
            friend class CRenderSubPass;
            friend class CRenderTarget;
            friend class CDeviceMemoryManager;
            friend class CResourceBarrierManager;
            friend class CAPIResourceManager;
            friend class CShaderManager;
            friend class CPipelineManager;
            friend class CCommandBuffer;
            friend class CcommandBufferManager;

        public:
            using GraphicsContextArray = Utils::TCDynamicArray< CGraphicsContext* >;
            using ComputeContextArray = Utils::TCDynamicArray< CComputeContext* >;
            using DataTransferContextArray = Utils::TCDynamicArray< CDataTransferContext* >;
            using RenderTargetArray = Utils::TCDynamicArray< CRenderTarget* >;
            using RenderPassArray = Utils::TCDynamicArray< CRenderPass* >;
            using RenderingPipeilneArray = Utils::TCDynamicArray< CRenderingPipeline* >;
            using GraphicsContexts = Utils::TSFreePool< CGraphicsContext* >;

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

            struct SDeviceInfo
            {
                VkPhysicalDeviceProperties          Properties;
                VkPhysicalDeviceMemoryProperties    MemoryProperties;
                VkPhysicalDeviceFeatures            Features;
                VkPhysicalDeviceLimits              Limits;
            };

            using QUEUE_TYPE = QueueTypes::TYPE;

            public:

                CDeviceContext(CRenderSystem*);
                ~CDeviceContext();

                Result  Create(const SDeviceContextDesc& Desc);
                void    Destroy();

                CGraphicsContext*   CreateGraphicsContext(const SGraphicsContextDesc& Desc);
                void                DestroyGraphicsContext(CGraphicsContext** ppCtxOut);
                CComputeContext*    CreateComputeContext(const SComputeContextDesc& Desc);
                CDataTransferContext*   CreateDataTransferContext(const SDataTransferContextDesc& Desc);

                //const GraphicsContextArray& GetGraphicsContexts() const { return m_vGraphicsContexts; }
                //const ComputeContextArray& GetComputeContexts() const { return m_vComputeContexts; }
                //const DataTransferContextArray& GetDataTransferContexts() const { return m_vDataTransferContexts; }

                CRenderSystem*  GetRenderSystem() const { return m_pRenderSystem; }

                
                //RenderingPipelineHandle CreateRenderingPipeline(const SRenderingPipelineDesc& Desc);
                //RenderTargetHandle CreateRenderTarget(const SRenderTargetDesc& Desc);
                //Result UpdateRenderTarget(const RenderTargetHandle& hRT, const SRenderTargetDesc& Desc);
                RenderPassHandle    CreateRenderPass(const SRenderPassDesc& Desc);
                CRenderPass*        GetRenderPass(const RenderPassHandle& hPass) const;

                CRenderTarget* GetRenderTarget(const RenderTargetHandle& hRenderTarget) const
                {
                    return m_vpRenderTargets[ static_cast<uint32_t>(hRenderTarget.handle) ];
                }

                CAPIResourceManager& Resource() { return *m_pAPIResMgr; }
                void RenderFrame(WindowPtr pWnd);

                const SDeviceInfo& GetDeviceInfo() const { return m_DeviceInfo; }

                PipelineRefPtr      CreatePipeline(const SPipelineCreateDesc& Desc);
                void                SetPipeline(CommandBufferPtr pCmdBuffer, PipelinePtr pPipeline);

                ShaderRefPtr        CreateShader(const SShaderCreateDesc& Desc);
                DescriptorSetRefPtr CreateDescriptorSet(const SDescriptorSetDesc& Desc);

            protected:

                void                    _Destroy();
                Vulkan::CDeviceWrapper& _GetDevice() const { return *m_pVkDevice; }
                Vulkan::ICD::Device&    _GetICD() const;
                CGraphicsContext*       _CreateGraphicsContextTask(const SGraphicsContextDesc&);
                VkInstance              _GetInstance() const;

                void        _NotifyDestroy(CGraphicsContext*);

            protected:

                SInternalData*              m_pPrivate = nullptr;
                CRenderSystem*              m_pRenderSystem = nullptr;
                //GraphicsContextArray        m_vGraphicsContexts;
                GraphicsContexts            m_GraphicsContexts;
                ComputeContextArray         m_vComputeContexts;
                Vulkan::CDeviceWrapper*     m_pVkDevice;
                SDeviceInfo                 m_DeviceInfo;
                CAPIResourceManager*        m_pAPIResMgr = nullptr;
                CShaderManager*             m_pShaderMgr = nullptr;
                RenderTargetArray           m_vpRenderTargets;
                RenderPassArray             m_vpRenderPasses;
                RenderingPipeilneArray      m_vpRenderingPipelines;
                Threads::SyncObject         m_SyncObj;
                CPipelineManager*           m_pPipelineMgr = nullptr;
                CDescriptorSetManager*      m_pDescSetMgr = nullptr;
                bool                        m_canRender = true;
                //ComputeContextArray         m_vComputeContexts;
                //DataTransferContextArray    m_vDataTransferContexts;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER