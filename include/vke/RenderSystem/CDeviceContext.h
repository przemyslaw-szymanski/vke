#pragma once
#include "Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CDescriptorSet.h"
#include "RenderSystem/CPipeline.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/Resources/CTexture.h"
#include "RenderSystem/Managers/CCommandBufferManager.h"
#include "RenderSystem/CContextBase.h"

namespace VKE
{
    namespace Threads
    {
        class CThreadPool;
    } // namespace Threads

    namespace RenderSystem
    {
        class CRenderSystem;
        class CGraphicsContext;
        class CComputeContext;
        class CDataTransferContext;
        class CResourceManager;
        class CRenderingPipeline;
        class CDescriptorSetManager;
        class CBuffer;

        class VKE_API CDeviceContext final //: public CContextBase
        {
            friend class CRenderSystem;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CDataTransferContext;
            friend class CResourceManager;
            friend class CRenderingPipeline;
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

        private:
            struct SMetricsSystem
            {
                Utils::CTimer         FpsTimer;
                Utils::CTimer         FrameTimer;
                uint32_t              frameCountPerSec = 0;
                uint32_t              totalFrameCount  = 0;
                uint32_t              fpsAccum         = 0;
                uint32_t              fpsFrameAccum    = 0;
                SDeviceContextMetrics Metrics;
            };

            using DescPoolArray = Utils::TCDynamicArray< handle_t >;

        public:
            using GraphicsContextArray     = Utils::TCDynamicArray< CGraphicsContext* >;
            using ComputeContextArray      = Utils::TCDynamicArray< CComputeContext* >;
            using DataTransferContextArray = Utils::TCDynamicArray< CDataTransferContext* >;
            using RenderTargetArray        = Utils::TCDynamicArray< CRenderTarget* >;
            using RenderingPipeilneArray   = Utils::TCDynamicArray< CRenderingPipeline* >;
            using GraphicsContextPool      = Utils::TSFreePool< CGraphicsContext* >;
            using QueueArray               = Utils::TCDynamicArray< CQueue >;
            using TransferContextArray     = Utils::TCDynamicArray< CTransferContext* >;
            using DDISemaphoreQueue        = Utils::TCFifo< NativeAPI::GPUFence >;
            using DDISemaphoreArray        = Utils::TCDynamicArray< NativeAPI::GPUFence >;
            using DDIEventPool             = Utils::TSFreePool< NativeAPI::Event >;
            using DDISemaphoreBoolMap      = vke_hash_map< NativeAPI::GPUFence, bool >;

            // using QUEUE_TYPE = QueueTypes::TYPE;

        public:
            CDeviceContext( CRenderSystem* );
            ~CDeviceContext();

            Result Create( const SDeviceContextDesc& Desc );
            void   Destroy();

            CGraphicsContext* CreateGraphicsContext( const SGraphicsContextDesc& Desc );
            void              DestroyGraphicsContext( CGraphicsContext** ppCtxOut );

            CGraphicsContext* GetGraphicsContext( const uint32_t& idx )
            {
                return m_GraphicsContexts[ idx ];
            }

            CComputeContext*  CreateComputeContext( const SComputeContextDesc& Desc );
            CTransferContext* CreateTransferContext( const STransferContextDesc& Desc );
            void              DestroyTransferContext( CTransferContext** ppCtxInOut );

            CTransferContext* GetTransferContext( uint32_t idx = 0 ) const;

            CRenderSystem* GetRenderSystem() const
            {
                return m_pRenderSystem;
            }

            NativeAPI::RenderPass CreateRenderPass( const SRenderPassDesc& Desc );

            void DestroyRenderPass( NativeAPI::RenderPass* phPass );

            CRenderTarget* GetRenderTarget( const RenderTargetHandle& hRenderTarget ) const
            {
                return m_vpRenderTargets[ static_cast< uint32_t >( hRenderTarget.handle ) ];
            }

            RenderTargetRefPtr GetRenderTarget( cstr_t pName );

            CAPIResourceManager& Resource()
            {
                return *m_pAPIResMgr;
            }

            void RenderFrame( WindowPtr pWnd );

            const SDeviceInfo& GetDeviceInfo() const
            {
                return m_DeviceInfo;
            }

            PipelineRefPtr       CreatePipeline( const SPipelineCreateDesc& Desc );
            PipelineLayoutRefPtr CreatePipelineLayout( const SPipelineLayoutDesc& Desc );
            PipelineRefPtr       GetLastCreatedPipeline() const;

            ShaderRefPtr              CreateShader( const SCreateShaderDesc& Desc );
            DescriptorSetLayoutHandle CreateDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc );
            BufferHandle              CreateBuffer( const SCreateBufferDesc& Desc );
            void                      DestroyBuffer( BufferPtr* ppInOut );
            void                      DestroyBuffer( BufferHandle* phBuffer );
            // VertexBufferRefPtr          CreateBuffer( const SCreateVertexBufferDesc& Desc );

            ShaderRefPtr                   GetShader( ShaderHandle hShader );
            NativeAPI::DescriptorSetLayout GetDescriptorSetLayout( DescriptorSetLayoutHandle hLayout );
            DescriptorSetLayoutHandle      GetDescriptorSetLayout( const DescriptorSetHandle& hSet );
            DescriptorSetLayoutHandle      GetDescriptorSetLayout( const SDescriptorSetLayoutDesc& Desc );
            PipelineRefPtr                 GetPipeline( PipelineHandle hPipeline );
            BufferRefPtr                   GetBuffer( BufferHandle hBuffer );
            BufferRefPtr                   GetBuffer( const VertexBufferHandle& hBuffer );
            BufferRefPtr                   GetBuffer( const IndexBufferHandle& hBuffer );
            PipelineLayoutRefPtr           GetPipelineLayout( PipelineLayoutHandle hLayout );

            TextureHandle CreateTexture( const SCreateTextureDesc& Desc );
            Result        LoadTexture( const Core::SLoadFileInfo& Info, TextureHandle* phOut );
            void          DestroyTexture( TextureHandle hTex );
            TextureRefPtr GetTexture( TextureHandle hTex );
            TextureRefPtr GetTexture( const RenderTargetHandle& hRT );

            TextureViewHandle CreateTextureView( const SCreateTextureViewDesc& Desc );
            void              DestroyTextureView( const TextureViewHandle& hView );
            void              DestroyTextureView( TextureViewPtr* ppView );
            TextureViewRefPtr GetTextureView( const TextureViewHandle& hView );
            TextureViewRefPtr GetTextureView( const RenderTargetHandle& hRT );
            TextureViewRefPtr GetTextureView( const TextureHandle& hTexture );

            RenderTargetHandle CreateRenderTarget( const SRenderTargetDesc& Desc );
            RenderTargetRefPtr GetRenderTarget( const RenderTargetHandle& hRT );
            void               DestroyRenderTarget( RenderTargetHandle* phRT );

            SamplerHandle CreateSampler( const SSamplerDesc& Desc );
            SamplerRefPtr GetSampler( const SamplerHandle& hSampler );
            void          DestroySampler( SamplerHandle* phSampler );

            EventHandle CreateEvent( const SEventDesc& Desc );

            NativeAPI::Event GetEvent( const EventHandle& hEvent )
            {
                return m_DDIEventPool[ static_cast< uint16_t >( hEvent.handle ) ];
            }

            void DestroyEvent( EventHandle* phEvent );
            bool IsEventSet( const EventHandle& hEvent );
            void ResetEvent( const EventHandle& hEvent );
            void SetEvent( const EventHandle& hEvent );

            bool IsFenceSignaled( NativeAPI::CPUFence hFence ) const
            {
                return m_DDI.IsSignaled( hFence );
            }

            bool IsReadyToUse( NativeAPI::CPUFence hFence ) const
            {
                return IsFenceSignaled( hFence );
            }

            bool IsLocked( NativeAPI::CPUFence hFence ) const
            {
                return !IsFenceSignaled( hFence );
            }

            CDDI& NativeAPI()
            {
                return m_DDI;
            }

            void Wait()
            {
                NativeAPI().WaitForDevice();
            }

            ShaderPtr                 GetDefaultShader( SHADER_TYPE type );
            DescriptorSetLayoutHandle GetDefaultDescriptorSetLayout();
            PipelineLayoutPtr         GetDefaultPipelineLayout();

            // Result                      ExecuteRemainingWork();

            void FreeUnusedAllocations();

            const SDeviceContextMetrics& GetMetrics() const
            {
                return m_MetricsSystem.Metrics;
            }

            const SDeviceFeatures& GetFeatures() const
            {
                return m_Features.Features;
            }

            uint32_t LockStagingBuffer( const uint32_t maxSize );
            Result   UpdateStagingBuffer( const SUpdateStagingBufferInfo& Info );
            Result   UnlockStagingBuffer( CContextBase* pCtx, const SUnlockBufferInfo& Info );
            Result   UploadMemoryToStagingBuffer( const SUpdateMemoryInfo& Info, SStagingBufferInfo* pOut );

            DescriptorSetHandle             CreateDescriptorSet( const SDescriptorSetDesc& Desc );
            const NativeAPI::DescriptorSet& GetDescriptorSet( const DescriptorSetHandle& hSet );

            void                UpdateDescriptorSet( BufferPtr pBuffer, DescriptorSetHandle* phInOut );
            void                UpdateDescriptorSet( const RenderTargetHandle& hRT, DescriptorSetHandle* phInOut );
            void                UpdateDescriptorSet( const SamplerHandle& hSampler, const RenderTargetHandle& hRT,
                                                     DescriptorSetHandle* phInOut );
            void                UpdateDescriptorSet( const SUpdateBindingsHelper& Info, DescriptorSetHandle* phInOut );
            void                UpdateDescriptorSet( SCopyDescriptorSetInfo& Info );
            void                FreeDescriptorSet( const DescriptorSetHandle& hSet );
            DescriptorSetHandle CreateResourceBindings( const SCreateBindingDesc& Desc );
            DescriptorSetHandle CreateResourceBindings( const SUpdateBindingsHelper& Info );

            void LogMemoryDebug() const;

            void GetFormatFeatures( TEXTURE_FORMAT, STextureFormatFeatures* ) const;

            NativeAPI::GPUFence CreateGPUFence( const SSemaphoreDesc& );
            void                DestroyGPUFence( NativeAPI::GPUFence* );
            NativeAPI::CPUFence CreateCPUFence( const SFenceDesc& );
            void                DestroyCPUFence( NativeAPI::CPUFence* );
            void                Reset( NativeAPI::CPUFence* );

        protected:
            void              _Destroy();
            CGraphicsContext* _CreateGraphicsContextTask( const SGraphicsContextDesc& );

            void _NotifyDestroy( CGraphicsContext* );

            const CDDI& _NativeAPI() const
            {
                return m_DDI;
            }

            CDDI& _NativeAPI()
            {
                return m_DDI;
            }

            QueueRefPtr _AcquireQueue( QUEUE_TYPE type, CContextBase* pCtx );

            RenderPassHandle _CreateRenderPass( const SRenderPassDesc& Desc, bool ddiHandles );
            RenderPassHandle _CreateRenderPass( const SSimpleRenderPassDesc& Desc );

            CDeviceMemoryManager& _GetDeviceMemoryManager()
            {
                return *m_pDeviceMemMgr;
            }

            CCommandBuffer* _GetCommandBuffer()
            {
                VKE_ASSERT2( m_pCurrentCommandBuffer != nullptr &&
                                 m_pCurrentCommandBuffer->GetState() == CCommandBuffer::States::BEGIN,
                             "" );
                return m_pCurrentCommandBuffer;
            }

            void _DestroyRenderPasses();

            void _OnFrameStart( CGraphicsContext* );
            void _OnFrameEnd( CGraphicsContext* );

            void _UpdateMetrics();

            void     _DestroyDescriptorSets( DescriptorSetHandle* phSets, const uint32_t count );
            void     _FreeDescriptorSets( DescriptorSetHandle* phSets, uint32_t count );
            Result   _CreateDescriptorPool( uint32_t descriptorCount );
            handle_t _CreateDescriptorPool( DescriptorSetLayoutHandle hLayout, uint32_t count );
            void     _DestroyDescriptorPools();

            Threads::CThreadPool* _GetThreadPool();

            void _LockGPUFence( NativeAPI::GPUFence* phApi );
            void _UnlockGPUFence( NativeAPI::GPUFence* phApi );
            bool _IsGPUFenceLocked( NativeAPI::GPUFence hApi );
            void _LogGPUFenceStatus();

        protected:
            SDeviceContextDesc m_Desc;
            SSettings          m_Features;
            QueueArray         m_vQueues;
            CRenderSystem*     m_pRenderSystem = nullptr;
            // GraphicsContextArray        m_vGraphicsContexts;
            GraphicsContextPool   m_GraphicsContexts;
            TransferContextArray  m_vpTransferContexts;
            ComputeContextArray   m_vpComputeContexts;
            CDeviceMemoryManager* m_pDeviceMemMgr = nullptr;
            // CCommandBufferManager       m_CmdBuffMgr;
            CDDI                 m_DDI;
            CCommandBuffer*      m_pCurrentCommandBuffer = nullptr;
            SDeviceInfo          m_DeviceInfo;
            Threads::SyncObject  m_SignaledSemaphoreSyncObj;
            DDISemaphoreArray    m_vDDISignaledSemaphores[ QueueTypes::_MAX_COUNT ];
            Threads::SyncObject  m_EventSyncObj;
            DDIEventPool         m_DDIEventPool;
            DDISemaphoreBoolMap  m_mLockedGPUFences;
            CAPIResourceManager* m_pAPIResMgr  = nullptr;
            CShaderManager*      m_pShaderMgr  = nullptr;
            CBufferManager*      m_pBufferMgr  = nullptr;
            CTextureManager*     m_pTextureMgr = nullptr;
            RenderTargetArray    m_vpRenderTargets;
            // RenderPassArray             m_vpRenderPasses;
            RenderingPipeilneArray m_vpRenderingPipelines;
            Threads::SyncObject    m_SyncObj;
            CPipelineManager*      m_pPipelineMgr = nullptr;
            CDescriptorSetManager* m_pDescSetMgr  = nullptr;
            bool                   m_canRender    = true;
            SMetricsSystem         m_MetricsSystem;
        };

        /* template<class T>
         Result CDeviceContext::_AddTask( Threads::THREAD_USAGES usages, Threads::THREAD_TYPE_INDEX idx,
         Threads::TSSimpleTask<T>& Task )
         {
             return m_pRenderSystem->GetEngine()->GetThreadPool()->AddTask( usages, index, Task );
         }*/

    } // namespace RenderSystem
} // namespace VKE
