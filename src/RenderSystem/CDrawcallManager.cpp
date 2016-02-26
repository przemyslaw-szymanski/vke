#include "RenderSystem/CDrawcallManager.h"
#include "RenderSystem/CDrawcall.h"

namespace VKE
{
    namespace RenderSystem
    {
        Result CDrawcallManager::Create(const SDrawcallManagerInfo& Info)
        {
            auto res = m_pMemMgr.Create(Constants::RenderSystem::DEFAULT_DRAWCALL_COUNT, sizeof(CDrawcall), 1);
            VKE_RETURN_IF_FAILED(res);
            return VKE_OK;
        }

        void CDrawcallManager::Destroy()
        {

        }
    }
}