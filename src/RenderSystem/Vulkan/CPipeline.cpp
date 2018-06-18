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
            


            return VKE_OK;
        }

        void CPipeline::Destroy()
        {

        }
    }
}