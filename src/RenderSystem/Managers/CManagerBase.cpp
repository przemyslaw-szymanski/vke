#include "RenderSystem/Managers/CManagerBase.h"
#include "RenderSystem/CRenderSystem.h"

namespace VKE
{
    namespace RenderSystem
    {
        CManagerBase::CManagerBase(CRenderSystem* pRS) :
            m_pRenderSystem(pRS)
        {}

        CManagerBase::~CManagerBase()
        {}

        Resources::CManager::ResourceRawPtr CManagerBase::_AllocateMemory(
            const Resources::SCreateDesc* const /*pCreateInfo*/)
        {
            return 0;
        }

    } // RenderSystem
} // VKE