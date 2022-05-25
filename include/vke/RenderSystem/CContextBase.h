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
            using VkCommandBufferArray = Utils::TCDynamicArray<VkCommandBuffer, DEFAULT_CMD_BUFFER_COUNT>;
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

            public:

                CContextBase( CDeviceContext* pCtx, cstr_t pName );

                Result                      Create(const SContextBaseDesc& Desc);
                void                        Destroy();

                CommandBufferPtr            GetCommandBuffer() { return CommandBufferPtr{ _GetCurrentCommandBuffer() }; }

                CDeviceContext*             GetDeviceContext() const { return m_pDeviceCtx; }
                CTransferContext*           GetTransferContext() const;
                //template<EXECUTE_COMMAND_BUFFER_FLAGS Flags = ExecuteCommandBufferFlags::END>
                Result Execute( EXECUTE_COMMAND_BUFFER_FLAGS flags);

                CCommandBuffer*             GetPreparationCommandBuffer();
                Result                      BeginPreparation();
                Result                      EndPreparation();
                Result                      WaitForPreparation();
                bool                        IsPreparationDone();

                DDISemaphore                GetSignaledSemaphore() const { return _GetLastExecutedBatch()->GetSignaledSemaphore(); }

                Result                      UpdateBuffer( const SUpdateMemoryInfo& Info, BufferHandle* phInOut );
                Result                      UpdateBuffer( const SUpdateMemoryInfo& Info, BufferPtr* ppInOut );

                Result                      UpdateTexture(const SUpdateMemoryInfo& Info, TextureHandle* phInOut);

                uint32_t                    LockStagingBuffer(const uint32_t maxSize);
                Result                      UpdateStagingBuffer( const SUpdateStagingBufferInfo& Info );
                Result                      UnlockStagingBuffer(CContextBase* pCtx, const SUnlockBufferInfo& Info);
                Result                      UploadMemoryToStagingBuffer(const SUpdateMemoryInfo& Info, SStagingBufferInfo* pOut);

                uint8_t                     GetBackBufferIndCOMMAND_BUFFER_END_FLAGSex() const { return m_backBufferIdx; }

                DescriptorSetHandle         CreateDescriptorSet( const SDescriptorSetDesc& Desc );
                const DDIDescriptorSet&     GetDescriptorSet( const DescriptorSetHandle& hSet );
                DescriptorSetLayoutHandle   GetDescriptorSetLayout( const DescriptorSetHandle& hSet );
                void                        UpdateDescriptorSet( BufferPtr pBuffer, DescriptorSetHandle* phInOut );
                void                        UpdateDescriptorSet( const RenderTargetHandle& hRT, DescriptorSetHandle* phInOut );
                void                        UpdateDescriptorSet( const SamplerHandle& hSampler, const RenderTargetHandle& hRT, DescriptorSetHandle* phInOut );
                void                        UpdateDescriptorSet( const SUpdateBindingsHelper& Info, DescriptorSetHandle* phInOut );

                void                        FreeDescriptorSet( const DescriptorSetHandle& hSet );

                DescriptorSetHandle         CreateResourceBindings( const SCreateBindingDesc& Desc );
                DescriptorSetHandle         CreateResourceBindings( const SUpdateBindingsHelper& Info );

                PipelinePtr                 BuildCurrentPipeline();

                vke_force_inline
                void                        SetTextureState( CommandBufferPtr pCmdbuffer, TEXTURE_STATE state, TextureHandle* phInOut ) { _SetTextureState( pCmdbuffer.Get(), state, phInOut ); }
                void SetTextureState( CommandBufferPtr pCmdbuffer, TEXTURE_STATE state,
                                      RenderTargetHandle* phInOut );

            protected:

                CCommandBuffer*         _CreateCommandBuffer();
                CCommandBuffer*         _GetCurrentCommandBuffer();
                Result                  _BeginCommandBuffer( CCommandBuffer** ppInOut );
                Result                  _EndCommandBuffer( EXECUTE_COMMAND_BUFFER_FLAGS flags, CCommandBuffer** ppInOut, DDISemaphore* phDDIOut );

                CCommandBufferBatch*    _GetLastExecutedBatch() const { return m_pLastExecutedBatch; }

                void                    _DestroyDescriptorSets( DescriptorSetHandle* phSets, const uint32_t count );
                void                    _FreeDescriptorSets( DescriptorSetHandle* phSets, uint32_t count );

                Result                  _EndCurrentCommandBuffer( EXECUTE_COMMAND_BUFFER_FLAGS flags, DDISemaphore* phDDIOut );

                SExecuteData*           _GetFreeExecuteData();
                void                    _AddDataToExecute( SExecuteData* pData ) { m_qExecuteData.push_back( pData ); }
                SExecuteData* _PopExecuteData();

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
                //handle_t                        m_hCommandPool = INVALID_HANDLE;
                //CCommandBuffer*                 m_pCurrentCommandBuffer = nullptr;
                CCommandBufferManager           m_CmdBuffMgr;
                CommandBufferArray              m_vCommandBuffers;
                CCommandBufferBatch*            m_pLastExecutedBatch;
                SPreparationData                m_PreparationData;
                SDescriptorPoolDesc             m_DescPoolDesc;
                DescPoolArray                   m_vDescPools;
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
        //    VKE_ASSERT( m_pCurrentCommandBuffer != nullptr, "" );
        //    m_pCurrentCommandBuffer->Begin();
        //    return ret;*/
        //}
    } // RenderSystem
} // VKE