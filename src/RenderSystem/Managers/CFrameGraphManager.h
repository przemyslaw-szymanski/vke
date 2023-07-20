#pragma once

#include "RenderSystem/CFrameGraph.h"
#include "Core/Memory/CFreeList.h"

namespace VKE::RenderSystem
{
    class IFrameGraph;

    

    struct SFrameGraphManagerDesc
    {

    };

    class CFrameGraphManager
    {
        friend class CRenderSystem;
        friend class CFrameGraph;
        using NodeMemoryManager = Memory::CFreeList;
        using FrameGraphMap = vke_hash_map<ResourceName, CFrameGraph>;
      public:

          CFrameGraph* CreateFrameGraph( const SFrameGraphDesc& );
          void DestroyFrameGraph( CFrameGraph** );
          void DestroyFrameGraph( cstr_t pName );

      protected:
        Result _Create(const SFrameGraphManagerDesc&);
        void _Destroy();

      protected:

          SFrameGraphManagerDesc m_Desc;
          NodeMemoryManager m_NodeMemMgr;
          FrameGraphMap m_mFrameGraphs;
    };
} // VKE