#include "RenderSystem/CPipeline.h"

namespace VKE
{
    namespace RenderSystem
    {
        Result CPipelineLayout::Init(const SPipelineLayoutDesc& Desc)
        {
            Result res = VKE_OK;

            return res;
        }

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
            //m_type = Desc.Shaders..IsValid() ? PipelineTypes::COMPUTE : PipelineTypes::GRAPHICS;
            m_type = Desc.Shaders.apShaders[ ShaderTypes::COMPUTE ].IsNull() ? PipelineTypes::GRAPHICS : PipelineTypes::COMPUTE;
            //m_Desc = Desc;
            return VKE_OK;
        }

        void CPipeline::Destroy()
        {

        }
    }
}