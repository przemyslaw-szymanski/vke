#pragma once
#include "RenderSystem/IFrameGraph.h"

namespace VKE
{
    namespace RenderSystem
    {
        static cstr_t   FRAME_GRAPH_FORWARD_RENDERER_NAME = "VKE_FORWARD_RENDERER";

        class CForwardRenderer : public IFrameGraph
        {
            using DrawcallArray = Utils::TCDynamicArray< DrawcallPtr, 1 >;
            using BoolArray = Utils::TCDynamicArray< bool, 1 >;
            using BitsArray = Utils::TCDynamicArray< UObjectBits, 1 >;

            public:

                void                    Render( CGraphicsContext* pCtx ) override;

            protected:

                Result          _Create( const SFrameGraphDesc& ) override;
                void            _Destroy() override;

                void            _Sort();

            protected:

                SForwardRendererDesc    m_Desc;
        };
    }
}