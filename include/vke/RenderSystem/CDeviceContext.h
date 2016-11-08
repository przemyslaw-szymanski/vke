#pragma once

#include "Common.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CRenderSystem;
        class CGraphicsContext;
        class CComputeContext;
        class CDataTransferContext;

        class VKE_API CDeviceContext
        {
            struct SInternalData;
            friend class CRenderSystem;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CDataTransferContext;

        public:
            using GraphicsContextArray = Utils::TCDynamicArray< CGraphicsContext* >;
            using ComputeContextArray = Utils::TCDynamicArray< CComputeContext* >;
            using DataTransferContextArray = Utils::TCDynamicArray< CDataTransferContext* >;

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

            protected:

                Result _CreateContexts();

            protected:

                SInternalData*              m_pPrivate = nullptr;
                CRenderSystem*              m_pRenderSystem = nullptr;
                GraphicsContextArray        m_vGraphicsContexts;
                //ComputeContextArray         m_vComputeContexts;
                //DataTransferContextArray    m_vDataTransferContexts;
        };
    } // RenderSystem
} // VKE