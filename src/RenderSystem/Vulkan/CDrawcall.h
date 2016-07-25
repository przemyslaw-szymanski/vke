#pragma once

#include "RenderSystem/Vulkan/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDrawcall
        {
            public:

            struct SDrawData
            {
                // Transform matrix
                // Vertex Buffer
                // Index Buffer
            };

            struct SCalcData
            {
                bool*   pVisible;
                bool*   pEnabled;
            };

            protected:

                SDrawData   m_Draw;
                SCalcData   m_Calc;
        };
    } // rendersystem
} // vke