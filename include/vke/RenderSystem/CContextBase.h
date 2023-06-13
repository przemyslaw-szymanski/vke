#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/CQueue.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/Vulkan/Managers/CCommandBufferManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        struct SContextBaseDesc
        {
            QueueRefPtr         pQueue;
            uint32_t            descPoolSize = Config::RenderSystem::Pipeline::MAX_DESCRIPTOR_SET_COUNT;
        };

        struct VKE_API SCreateBindingDesc
        {
            friend class CContextBase;

            void AddBinding( const SDescriptorSetLayoutDesc::SBinding& Binding );
            void AddBinding( const SResourceBinding& Binding, const BufferPtr& pBuffer );
            void AddBinding( const STextureBinding& Binding );
            void AddBinding( const SSamplerBinding& Binding );
            void AddBinding( const SSamplerTextureBinding& Binding );

            void AddConstantBuffer( uint8_t index, PIPELINE_STAGES stages );
            void AddStorageBuffer( uint8_t index, PIPELINE_STAGES stages, const uint16_t& arrayElementCount );
            void AddTextures( uint8_t index, PIPELINE_STAGES stages, uint16_t count = 1u );
            void AddSamplers( uint8_t index, PIPELINE_STAGES stages, uint16_t count = 1u );
            void AddSamplerAndTexture( uint8_t index, PIPELINE_STAGES stages );

            SDescriptorSetLayoutDesc    LayoutDesc;
            VKE_RENDER_SYSTEM_DEBUG_NAME;
        };

        // Implementation in CDeviceContext.cpp
        class VKE_API CContextBase
        {
            friend class CDDI;
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CCommandBuffer;
            friend class CBufferManager;
            friend class CTransferContext;
            friend class CCommandBufferManager;
            friend class CSubmitManager;

            protected:

            struct SPreparationData
            {
                CCommandBuffer* pCmdBuffer = nullptr;
                DDIFence        hDDIFence = DDI_NULL_HANDLE;
            };

            using DescPoolArray = Utils::TCDynamicArray< handle_t >;

            static const uint32_t DEFAULT_CMD_BUFFER_COUNT = 32;
            using CommandBufferArray = Utils::TCDynamicArray<CommandBufferPtr, DEFAULT_CMD_BUFFER_COUNT>;
            using DDICommandBufferArray = Utils::TCDynamicArray<DDICommandBuffer, DEFAULT_CMD_BUFFER_COUNT>;
            using UintArray = Utils::TCDynamicArray<uint32_t, DEFAULT_CMD_BUFFER_COUNT>;

            struct SCommandBufferBatch
            {
                CommandBufferArray vCmdBuffers;
                VkFence vkFence = VK_NULL_HANDLE;
                bool readyToExecute = false;
                void Reset()
                {
                    vCmdBuffers.Clear();
                    readyToExecute = false;
                }
            };
            using SubmitArray = Utils::TCDynamicArray<SCommandBufferBatch>;
            using SubmitList = std::list<SCommandBufferBatch>;
            using SemaphoreArray = Utils::TCDynamicArray<DDISemaphore, 8>;
            struct SExecuteData
            {
                // DDISemaphore            hDDISemaphoreBackBufferReady;
                SemaphoreArray vWaitSemaphores;
                CCommandBufferBatch* pBatch;
                uint32_t ddiImageIndex;

                uint32_t handle;
            };
            // using ExecuteDataQueue = Utils::TCList< SExecuteData >;
            using ExecuteDataQueue = std::deque<SExecuteData*>;
            using ExecuteDataPool = Utils::TSFreePool< SExecuteData >;

            struct SExecuteBatch
            {
                using ExecuteBatchArray = Utils::TCDynamicArray<SExecuteBatch*, 4>;

                CContextBase*   pContext = nullptr;
                DDISemaphore    hSignalGPUFence = DDI_NULL_HANDLE;
                DDIFence        hSignalCPUFence = DDI_NULL_HANDLE;
                SemaphoreArray  vDDIWaitGPUFences;
                Utils::TCDynamicArray<CCommandBuffer*, DEFAULT_CMD_BUFFER_COUNT> vpCommandBuffers;
                uint32_t        swapchainElementIndex = INVALID_POSITION;
                VKE_DEBUG_CODE( uint32_t executionCount = 0; )
                VKE_DEBUG_CODE( uint32_t acquireCount = 0; )
                Result          executionResult = Results::NOT_READY;
                EXECUTE_COMMAND_BUFFER_FLAGS executeFlags = 0;
                ExecuteBatchArray vDependencies;

                void AddDependency(SExecuteBatch** ppBatch)
                {
                    SExecuteBatch* pBatch = *ppBatch;
                    vDDIWaitGPUFences.PushBackUnique( pBatch->hSignalGPUFence );
                    vDependencies.PushBackUnique( pBatch );
                    pBatch->executeFlags |= ExecuteCommandBufferFlags::SIGNAL_GPU_FENCE;
                    VKE_ASSERT( pBatch->executionResult == Results::NOT_READY );
                }
            };
            using ExecuteBatchArray = Utils::TCDynamicArray< SExecuteBatch >;
            using ExecuteBatchQueue = std::deque<SExecuteBatch*>;

            public:

                CContextBase( CDeviceContext* pCtx, cstr_t pName );

                Result                      Create(const SContextBaseDesc& Desc);
                void                        Destroy();

                CommandBufferPtr            GetCommandBuffer() { return CommandBufferPtr{ _GetCurrentCommandBuffer() }; }

                CDeviceContext*             GetDeviceContext() const { return m_pDeviceCtx; }
                CTransferContext*           GetTransferContext() const;
                //template<EXECUTE_COMMAND_BUFFER_FLAGS Flags = ExecuteCommandBufferFlags::END>
                Result Execute( EXECUTE_COMMAND_BUFFER_FLAGS flags);

                //CCommandBuffer*             GetPreparationCommandBuffer();
                //Result                      BeginPreparation();
                //Result                      EndPreparation();
                //Result                      WaitForPreparation();
                //bool                        IsPreparationDone();

                DDISemaphore                GetSignaledSemaphore() const { return _GetLastExecutedBatch()->GetSignaledSemaphore(); }

                Result                      UpdateBuffer( CommandBufferPtr, const SUpdateMemoryInfo& Info, BufferHandle* phInOut );
                Result                      UpdateBuffer( CommandBufferPtr, const SUpdateMemoryInfo& Info, BufferPtr* ppInOut );

                Result                      UpdateTexture(const SUpdateMemoryInfo& Info, TextureHandle* phInOut);

                /*uint32_t                    LockStagingBuffer(const uint32_t maxSize);
                Result                      UpdateStagingBuffer( const SUpdateStagingBufferInfo& Info );
                Result                      UnlockStagingBuffer(CContextBase* pCtx, const SUnlockBufferInfo& Info);
                Result                      UploadMemoryToStagingBuffer(const SUpdateMemoryInfo& Info, SStagingBufferInfo* pOut);*/

                uint8_t                     GetBackBufferIndex() const { return m_backBufferIdx; }

                /*DescriptorSetHandle         CreateDescriptorSet( const SDescriptorSetDesc& Desc );
                const DDIDescriptorSet&     GetDescriptorSet( const DescriptorSetHandle& hSet );
                DescriptorSetLayoutHandle   GetDescriptorSetLayout( const DescriptorSetHandle& hSet );
                void                        UpdateDescriptorSet( BufferPtr pBuffer, DescriptorSetHandle* phInOut );
                void                        UpdateDescriptorSet( const RenderTargetHandle& hRT, DescriptorSetHandle* phInOut );
                void                        UpdateDescriptorSet( const SamplerHandle& hSampler, const RenderTargetHandle& hRT, DescriptorSetHandle* phInOut );
                void                        UpdateDescriptorSet( const SUpdateBindingsHelper& Info, DescriptorSetHandle* phInOut );

                void                        FreeDescriptorSet( const DescriptorSetHandle& hSet );

                DescriptorSetHandle         CreateResourceBindings( const SCreateBindingDesc& Desc );
                DescriptorSetHandle         CreateResourceBindings( const SUpdateBindingsHelper& Info );*/

                PipelinePtr                 BuildCurrentPipeline();

                vke_force_inline
                void                        SetTextureState( CommandBufferPtr pCmdbuffer, TEXTURE_STATE state, TextureHandle* phInOut ) { _SetTextureState( pCmdbuffer.Get(), state, phInOut ); }
                void SetTextureState( CommandBufferPtr pCmdbuffer, TEXTURE_STATE state,
                                      RenderTargetHandle* phInOut );

                void Lock() { m_CommandBufferSyncObj.Lock(); }
                void Unlock() { m_CommandBufferSyncObj.Unlock();}
                bool IsLocked() const { return m_CommandBufferSyncObj.IsLocked(); }

                void SyncExecute( CommandBufferPtr );
                void SignalGPUFence();

            protected:

                SExecuteBatch*          _AcquireExecuteBatch();
                SExecuteBatch*          _PushCurrentBatchToExecuteQueue();
                SExecuteBatch*          _PopExecuteBatch();
                Result                  _ExecuteBatch(SExecuteBatch*);
                Result                  _ExecuteAllBatches();
                SExecuteBatch* _ResetExecuteBatch( uint32_t idx );
                Result _CreateNewExecuteBatch();
                Result _ExecuteDependenciesForBatch(SExecuteBatch* pBatch);

                CCommandBuffer*         _CreateCommandBuffer();
                CCommandBuffer*         _GetCurrentCommandBuffer();
                Result                  _BeginCommandBuffer( CCommandBuffer** ppInOut );
                Result                  _EndCommandBuffer( CCommandBuffer** ppInOut );

                CCommandBufferBatch*    _GetLastExecutedBatch() const { return m_pLastExecutedBatch; }

                /*void                    _DestroyDescriptorSets( DescriptorSetHandle* phSets, const uint32_t count );
                void                    _FreeDescriptorSets( DescriptorSetHandle* phSets, uint32_t count );*/

                Result                  _EndCurrentCommandBuffer();

                SExecuteData*           _GetFreeExecuteData();
                void                    _AddDataToExecute( SExecuteData* pData ) { m_qExecuteData.push_back( pData ); }
                SExecuteData* _PopExecuteData();
                /// <summary>
                /// Checks which batched command buffers are executed
                /// and free them for later reuse
                /// </summary>
                void _FreeExecutedBatches();

                CDDI& _GetDDI() const { return m_DDI; }

                CCommandBufferManager& _GetCommandBufferManager() { return m_CmdBuffMgr; }
                void _FreeCommandBuffers( uint32_t count, CCommandBuffer** ppArray )
                {
                    m_CmdBuffMgr.FreeCommandBuffers< VKE_NOT_THREAD_SAFE >( count, ppArray );
                }

                void _SetTextureState( CCommandBuffer* pCmdBuff, TEXTURE_STATE state, TextureHandle* phInOut );

                

            protected:

                CDDI&                           m_DDI;
                CDeviceContext*                 m_pDeviceCtx;
                cstr_t                          m_pName = "";
                QueueRefPtr                     m_pQueue;
                Threads::SyncObject             m_ExecuteBatchSyncObj;
                ExecuteBatchArray               m_vExecuteBatches;
                SExecuteBatch*                  m_pCurrentExecuteBatch = nullptr;
                ExecuteBatchQueue               m_qExecuteBatches;
                uint32_t                        m_currExeBatchIdx = 0;
                CCommandBufferManager           m_CmdBuffMgr;
                Threads::SyncObject             m_CommandBufferSyncObj;
                //CommandBufferArray              m_vCommandBuffers;
                CCommandBufferBatch*            m_pLastExecutedBatch;
                //SPreparationData                m_PreparationData;
                /*SDescriptorPoolDesc             m_DescPoolDesc;
                DescPoolArray                   m_vDescPools;*/
                ExecuteDataQueue                m_qExecuteData;
                ExecuteDataPool                 m_ExecuteDataPool;
                EXECUTE_COMMAND_BUFFER_FLAGS    m_additionalEndFlags = ExecuteCommandBufferFlags::END;
                uint8_t                         m_backBufferIdx = 0;
                bool                            m_initComputeShader = false;
                bool                            m_initGraphicsShaders = false;
        };
    } // RenderSystem

    namespace RenderSystem
    {
        //template<EXECUTE_COMMAND_BUFFER_FLAGS Flags>
        //Result CContextBase::Execute()
        //{
        //    EXECUTE_COMMAND_BUFFER_FLAGS flags = ExecuteCommandBufferFlags::EXECUTE | Flags;
        //    //Result ret = this->m_pCurrentCommandBuffer->End( flags, nullptr );
        //    return m_pDeviceCtx->m_CmdBuffMgr.EndCommandBuffer( this, flags );
        //    /*m_pCurrentCommandBuffer = _CreateCommandBuffer();
        //    VKE_ASSERT2( m_pCurrentCommandBuffer != nullptr, "" );
        //    m_pCurrentCommandBuffer->Begin();
        //    return ret;*/
        //}
    } // RenderSystem
} // VKE