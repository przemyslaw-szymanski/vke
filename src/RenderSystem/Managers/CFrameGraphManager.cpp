#include "CFrameGraphManager.h"

namespace VKE::RenderSystem
{
    Result CFrameGraphManager::_Create(const SFrameGraphManagerDesc& Desc)
    {
        Result ret = VKE_OK;
        m_Desc = Desc;
        ret = m_NodeMemMgr.Create( 128, sizeof( CFrameGraph ) );
        if( VKE_SUCCEEDED(ret) )
        {
            
        }
        return ret;
    }

    void CFrameGraphManager::_Destroy()
    {
        for (auto& Pair : m_mFrameGraphs)
        {
            Pair.second._Destroy();
        }
        m_mFrameGraphs.clear();
        m_NodeMemMgr.Destroy();
    }

    CFrameGraph* CFrameGraphManager::CreateFrameGraph(const SFrameGraphDesc& Desc)
    {
        CFrameGraph* pRet = nullptr;
        CFrameGraph& FrameGraph = m_mFrameGraphs[ Desc.Name ];
        if( VKE_SUCCEEDED( FrameGraph._Create( Desc ) ) )
        {
            pRet = &FrameGraph;
            m_pCurrentFrameGraph = pRet;
        }
        return pRet;
    }

} // VKE::RenderSystem