#pragma once

#include "RenderSystem/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDevice;

        struct SPipelineInternal;

        struct PipelineFlags
        {
            enum FLAG
            {

            };
        };

        class CPipeline
        {
            public:

            CPipeline(CDevice* pDevice);
            virtual ~CPipeline();

            void Create();
            size_t CalcPipelineObjectSize() const;
            size_t CalcPipelineInternalDataSize() const;

            protected:

                CDevice*            m_pDevice;
                SPipelineInternal*  m_pInternal;
        };
    } // RenderSystem
} // VKE
