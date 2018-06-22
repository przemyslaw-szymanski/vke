#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/CPipeline.h"
#include "Core/Memory/TCFreeListManager.h"
#include "Core/Managers/CResourceManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CPipelineManager
        {
            friend class CDeviceContext;
            friend class CPipeline;

            protected:

                using PipelineBuffer = Core::TSResourceBuffer< CPipeline*, CPipeline*, 2048 >;

            public:

                CPipelineManager(CDeviceContext* pCtx);
                ~CPipelineManager();

                Result Create(const SPipelineManagerDesc&);
                void Destroy();

                PipelinePtr CreatePipeline(const SPipelineDesc&);

            protected:

                hash_t      _CalcHash(const SPipelineDesc&);
                Result      _CreatePipeline(const SPipelineDesc& Desc, CPipeline::SVkCreateDesc* pOut);

            protected:

                CDeviceContext*         m_pCtx;
                PipelineBuffer          m_Buffer;
                Memory::CFreeListPool   m_PipelineFreeListPool;
        };
    } // RenderSystem
} // VKE