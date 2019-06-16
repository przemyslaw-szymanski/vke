#pragma once

#include "RenderSystem/Common.h"
#include "Core/Math/Math.h"
#include "Core/Utils/TCBitset.h"

namespace VKE
{
    namespace RenderSystem
    {
        using LayerHandle = uint8_t;
        using FrameGraphNodeHandle = uint16_t;

        class CFrameGraph;
        class CFrameGraphNode;
        class CRenderLayer;

        

        struct SRenderLayerDesc
        {
            uint16_t    level;
        };

        

        class VKE_API CFrameGraph
        {
            
        };
    } // RenderSystem
} // VKE