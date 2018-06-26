#include "RenderSystem/CPipeline.h"

namespace VKE
{
    namespace RenderSystem
    {
        CPipeline::CPipeline(CPipelineManager* pMgr) :
            m_pMgr( pMgr )
        {
        }

        CPipeline::~CPipeline()
        {
            Destroy();
        }

        Result CPipeline::Init(const SPipelineDesc& Desc)
        {
            m_type = Desc.Shaders.pComputeShader.IsValid() ? PipelineTypes::COMPUTE : PipelineTypes::GRAPHICS;
            return VKE_OK;
        }

        void CPipeline::Destroy()
        {

        }
    }
}