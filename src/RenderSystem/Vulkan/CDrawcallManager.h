#pragma once

#include "RenderSystem/Vulkan/Common.h"
#include "Core/Memory/CFreeListPool.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDrawcall;

        struct SDrawcallManagerInfo
        {

        };

        class CDrawcallManager
        {
            using DrawcallPtr = CDrawcall*;
            using DrawcallVec = vke_vector< DrawcallPtr >;
            // Transform matrices
            // Materials
            // Vertex buffers
            // Index buffers
            // visibles. Whether object is visible or not
            // disableds. Whether object is disabled from rendering
            // Bounding boxes
            // Bounding spheres
            // AABBs

            public:

            Result Create(const SDrawcallManagerInfo& Info);
            void Destroy();

            protected:

                Memory::CFreeListPool m_pMemMgr;
        };
    } // RenderSystem
} // VKE