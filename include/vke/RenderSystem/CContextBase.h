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

            void AddBinding( const SResourceBinding& Binding, const BufferPtr& pBuffer );
            void AddBinding( const SResourceBinding& Binding, const TexturePtr& pTexture );

            SDescriptorSetLayoutDesc    LayoutDesc;
        };

        // Implementation in CDeviceContext.cpp
        class VKE_API CContextBase : public virtual CCommandBufferContext
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CCommandBuffer;
            friend class CBufferManager;

            struct SPreparationData 
            {
                CCommandBuffer* pCmdBuffer = nullptr;
                DDIFence        hDDIFence = DDI_NULL_HANDLE;
            };

            using DescPoolArray = Utils::TCDynamicArray< handle_t >;

            public:

                CContextBase( CDeviceContext* pCtx );

                Result                  Create(const SContextBaseDesc& Desc);
                void                    Destroy();

                CDeviceContext*         GetDeviceContext() { return m_pDeviceCtx; }

                CCommandBuffer*         GetPreparationCommandBuffer();
                Result                  BeginPreparation();
                Result                  EndPreparation();
                Result                  WaitForPreparation();
                bool                    IsPreparationDone();

                DDISemaphore            GetSignaledSemaphore() const { return _GetLastExecutedBatch()->GetSignaledSemaphore(); }

                Result                  UpdateBuffer( const SUpdateMemoryInfo& Info, BufferPtr* ppInOut );

                uint8_t                 GetBackBufferIndex() const { return m_backBufferIdx; }

                DescriptorSetHandle     CreateDescriptorSet( const SDescriptorSetDesc& Desc );
                const SDescriptorSet*   GetDescriptorSet( const DescriptorSetHandle& hSet );
                void                    UpdateDescriptorSet( BufferPtr pBuffer, DescriptorSetHandle* phInOut );

                DescriptorSetHandle     CreateResourceBindings( const SCreateBindingDesc& Desc );

            protected:

                CCommandBuffer*         _CreateCommandBuffer();
                Result                  _BeginCommandBuffer( CCommandBuffer** ppInOut );
                Result                  _EndCommandBuffer( CCommandBuffer** ppInOut, COMMAND_BUFFER_END_FLAGS flags );
                void                    _FlushCommandBuffer( CCommandBuffer** ppInOut );
                Result                  _ExecuteCommandBuffer( CCommandBuffer** ppInOut );

                CCommandBufferBatch*    _GetLastExecutedBatch() const { return m_pLastExecutedBatch; }

            protected:

                CDDI&                   m_DDI;
                CDeviceContext*         m_pDeviceCtx;
                QueueRefPtr             m_pQueue;
                handle_t                m_hCommandPool = NULL_HANDLE;
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