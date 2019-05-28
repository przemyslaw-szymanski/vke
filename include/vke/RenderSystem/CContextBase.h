#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/CQueue.h"
#include "RenderSystem/Vulkan/Managers/CSubmitManager.h"
#include "RenderSystem/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        struct SContextBaseDesc
        {
            QueueRefPtr         pQueue;
            handle_t            hCommandBufferPool;
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

            void AddBuffer( uint8_t index, PIPELINE_STAGES stages );
            void AddTexture( uint8_t index, PIPELINE_STAGES stages );
            void AddSampler( uint8_t index, PIPELINE_STAGES stages );
            void AddSamplerTexture( uint8_t index, PIPELINE_STAGES stages );

            SDescriptorSetLayoutDesc    LayoutDesc;
        };

        // Implementation in CDeviceContext.cpp
        class VKE_API CContextBase
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CCommandBuffer;
            friend class CBufferManager;
            friend class CTransferContext;

            struct SPreparationData 
            {
                CCommandBuffer* pCmdBuffer = nullptr;
                DDIFence        hDDIFence = DDI_NULL_HANDLE;
            };

            using DescPoolArray = Utils::TCDynamicArray< handle_t >;

            public:

                CContextBase( CDeviceContext* pCtx );

                Result                      Create(const SContextBaseDesc& Desc);
                void                        Destroy();

                CCommandBuffer*             GetCommandBuffer() { return _GetCurrentCommandBuffer(); }

                CDeviceContext*             GetDeviceContext() { return m_pDeviceCtx; }
                CTransferContext*           GetTransferContext();
                Result                      Flush( DDISemaphore* phDDIOut );

                CCommandBuffer*             GetPreparationCommandBuffer();
                Result                      BeginPreparation();
                Result                      EndPreparation();
                Result                      WaitForPreparation();
                bool                        IsPreparationDone();

                DDISemaphore                GetSignaledSemaphore() const { return _GetLastExecutedBatch()->GetSignaledSemaphore(); }

                Result                      UpdateBuffer( const SUpdateMemoryInfo& Info, BufferPtr* ppInOut );

                uint8_t                     GetBackBufferIndex() const { return m_backBufferIdx; }

                DescriptorSetHandle         CreateDescriptorSet( const SDescriptorSetDesc& Desc );
                const DDIDescriptorSet&     GetDescriptorSet( const DescriptorSetHandle& hSet );
                DescriptorSetLayoutHandle   GetDescriptorSetLayout( const DescriptorSetHandle& hSet );
                void                        UpdateDescriptorSet( BufferPtr pBuffer, DescriptorSetHandle* phInOut );
                void                        UpdateDescriptorSet( const RenderTargetHandle& hRT, DescriptorSetHandle* phInOut );
                void                        UpdateDescriptorSet( const SamplerHandle& hSampler, const RenderTargetHandle& hRT, DescriptorSetHandle* phInOut );
                void                        UpdateDescriptorSet( const SUpdateBindingsInfo& Info, DescriptorSetHandle* phInOut );

                void                        FreeDescriptorSet( const DescriptorSetHandle& hSet );

                DescriptorSetHandle         CreateResourceBindings( const SCreateBindingDesc& Desc );
                DescriptorSetHandle         CreateResourceBindings( const SUpdateBindingsInfo& Info );

                PipelinePtr                 BuildCurrentPipeline();

                void                        SetTextureState( const TextureHandle& hTex, const TEXTURE_STATE& state );
                void                        SetTextureState( const RenderTargetHandle& hRt, const TEXTURE_STATE& state );

            protected:

                CCommandBuffer*         _CreateCommandBuffer();
                CCommandBuffer*         _GetCurrentCommandBuffer();
                Result                  _BeginCommandBuffer( CCommandBuffer** ppInOut );
                Result                  _EndCommandBuffer( COMMAND_BUFFER_END_FLAGS flags, CCommandBuffer** ppInOut, DDISemaphore* phDDIOut );

                CCommandBufferBatch*    _GetLastExecutedBatch() const { return m_pLastExecutedBatch; }

                void                    _DestroyDescriptorSets( DescriptorSetHandle* phSets, const uint32_t count );
                void                    _FreeDescriptorSets( DescriptorSetHandle* phSets, uint32_t count );

                Result                  _FlushCurrentCommandBuffer( DDISemaphore* phDDIOut );
                Result                  _EndCurrentCommandBuffer(COMMAND_BUFFER_END_FLAGS flags, DDISemaphore* phDDIOut );

            protected:

                CDDI&                   m_DDI;
                CDeviceContext*         m_pDeviceCtx;
                QueueRefPtr             m_pQueue;
                handle_t                m_hCommandPool = NULL_HANDLE;
                CCommandBuffer*         m_pCurrentCommandBuffer = nullptr;
                CCommandBufferBatch*    m_pLastExecutedBatch;
                SPreparationData        m_PreparationData;
                SDescriptorPoolDesc     m_DescPoolDesc;
                DescPoolArray           m_vDescPools;
                uint8_t                 m_backBufferIdx = 0;
                bool                    m_initComputeShader = false;
                bool                    m_initGraphicsShaders = false;
        };
    } // RenderSystem
} // VKE