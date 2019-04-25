#pragma once
#include "Core/VKEPreprocessor.h"
#if VKE_VULKAN_RENDERER
#include "CDDI.h"
#include "Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CDescriptorSet.h"
#include "RenderSystem/CPipeline.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/Resources/CTexture.h"
#include "RenderSystem/Vulkan/Managers/CCommandBufferManager.h"
#include "RenderSystem/CQueue.h"
#include "RenderSystem/CContextBase.h"

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
        class CBuffer;


        

        class VKE_API CDeviceContext : public CContextBase
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
            friend class CDescriptorSetManager;
            friend class CBufferManager;
            friend class CBuffer;
            friend class CDDI;
            friend class CTextureManager;
            friend class CSubmitManager;
            friend class CCommandBufferManager;
            friend class CSwapChain;
            friend class CContextBase;
            friend class CStagingBufferManager;
            friend class CTransferContext;

        public:
            using GraphicsContextArray = Utils::TCDynamicArray< CGraphicsContext* >;
            using ComputeContextArray = Utils::TCDynamicArray< CComputeContext* >;
            using DataTransferContextArray = Utils::TCDynamicArray< CDataTransferContext* >;
            using RenderTargetArray = Utils::TCDynamicArray< CRenderTarget* >;
            using RenderPassArray = Utils::TCDynamicArray< CRenderPass* >;
            using RenderingPipeilneArray = Utils::TCDynamicArray< CRenderingPipeline* >;
            using GraphicsContextPool = Utils::TSFreePool< CGraphicsContext* >;
            using QueueArray = Utils::TCDynamicArray< CQueue >;
            using TransferContextArray = Utils::TCDynamicArray< CTransferContext* >;

            using QUEUE_TYPE = QueueTypes::TYPE;

            public:

                CDeviceContext(CRenderSystem*);
                ~CDeviceContext();

                Result  Create(const SDeviceContextDesc& Desc);
                void    Destroy();

                CGraphicsContext*   CreateGraphicsContext(const SGraphicsContextDesc& Desc);
                void                DestroyGraphicsContext(CGraphicsContext** ppCtxOut);
                CComputeContext*    CreateComputeContext(const SComputeContextDesc& Desc);
                CTransferContext*   CreateTransferContext( const STransferContextDesc& Desc );
                void                DestroyTransferContext( CTransferContext** ppCtxInOut );

                CTransferContext*   GetTransferContext( uint32_t idx = 0 );
                //CDataTransferContext*   CreateDataTransferContext(const SDataTransferContextDesc& Desc);

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

                PipelineRefPtr              CreatePipeline(const SPipelineCreateDesc& Desc);
                PipelineLayoutRefPtr        CreatePipelineLayout(const SPipelineLayoutDesc& Desc);
                void                        SetPipeline(CommandBufferPtr pCmdBuffer, PipelinePtr pPipeline);

                ShaderRefPtr                CreateShader(const SCreateShaderDesc& Desc);
                DescriptorSetRefPtr         CreateDescriptorSet(const SDescriptorSetDesc& Desc);
                DescriptorSetLayoutRefPtr   CreateDescriptorSetLayout(const SDescriptorSetLayoutDesc& Desc);
                BufferRefPtr                CreateBuffer( const SCreateBufferDesc& Desc );
                void                        DestroyBuffer( BufferPtr* ppInOut );
                //VertexBufferRefPtr          CreateBuffer( const SCreateVertexBufferDesc& Desc );

                ShaderRefPtr                GetShader( ShaderHandle hShader );
                DescriptorSetRefPtr         GetDescriptorSet( DescriptorSetHandle hSet );
                DescriptorSetLayoutRefPtr   GetDescriptorSetLayout( DescriptorSetLayoutHandle hSet );
                PipelineRefPtr              GetPipeline( PipelineHandle hPipeline );
                BufferRefPtr                GetBuffer( BufferHandle hBuffer );
                PipelineLayoutRefPtr        GetPipelineLayout( PipelineLayoutHandle hLayout );

                TextureHandle               CreateTexture( const SCreateTextureDesc& Desc );
                void                        DestroyTexture( TextureHandle hTex );
                TextureRefPtr               GetTexture( TextureHandle hTex );

                TextureViewHandle           CreateTextureView( const SCreateTextureViewDesc& Desc );
                void                        DestroyTextureView( TextureViewHandle hView );
                void                        DestroyTextureView( TextureViewPtr* ppView );
                TextureViewRefPtr           GetTextureView( TextureViewHandle hView );
                TextureViewRefPtr           GetTextureView( cstr_t pName );

                CDDI&                       DDI() { return m_DDI; }

                ShaderPtr                   GetDefaultShader( SHADER_TYPE type );
                DescriptorSetLayoutPtr      GetDefaultDescriptorSetLayout();
                PipelineLayoutPtr           GetDefaultPipelineLayout();

                Result                      ExecuteRemainingWork();

            protected:

                void                    _Destroy();
                //Vulkan::ICD::Device&    _GetICD() const;
                CGraphicsContext*       _CreateGraphicsContextTask(const SGraphicsContextDesc&);
                VkInstance              _GetInstance() const;
                Result                  _CreateCommandBuffers( const handle_t& hPool, uint32_t count, CCommandBuffer** ppBuffers );
                void                    _FreeCommandBuffers( const handle_t& hPool, uint32_t count, CCommandBuffer** ppBuffers );

                Result                  _AddTask(Threads::ITask*);

                void                    _NotifyDestroy(CGraphicsContext*);

                CDDI&                   _GetDDI() { return m_DDI; }

                QueueRefPtr             _AcquireQueue(QUEUE_TYPE type);

                RenderPassHandle        _CreateRenderPass( const SRenderPassDesc& Desc, bool ddiHandles );

                CDeviceMemoryManager&   _GetDeviceMemoryManager() { return *m_pDeviceMemMgr; }

                CCommandBuffer*         _GetCommandBuffer()
                {
                    VKE_ASSERT( m_pCurrentCommandBuffer != nullptr && m_pCurrentCommandBuffer->GetState() == CCommandBuffer::States::BEGIN, "" );
                    return m_pCurrentCommandBuffer;
                }

            protected:

                SDeviceContextDesc          m_Desc;
                QueueArray                  m_vQueues;
                //SInternalData*              m_pPrivate = nullptr;
                CRenderSystem*              m_pRenderSystem = nullptr;
                //GraphicsContextArray        m_vGraphicsContexts;
                GraphicsContextPool            m_GraphicsContexts;
                TransferContextArray        m_vpTransferContexts;
                ComputeContextArray         m_vpComputeContexts;
                CDeviceMemoryManager*       m_pDeviceMemMgr = nullptr;
                CCommandBufferManager       m_CmdBuffMgr;
                CDDI                        m_DDI;
                CCommandBuffer*             m_pCurrentCommandBuffer = nullptr;
                SDeviceInfo                 m_DeviceInfo;
                CAPIResourceManager*        m_pAPIResMgr = nullptr;
                CShaderManager*             m_pShaderMgr = nullptr;
                CBufferManager*             m_pBufferMgr = nullptr;
                CTextureManager*            m_pTextureMgr = nullptr;
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