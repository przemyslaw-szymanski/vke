#pragma once

#include "Core/VKECommon.h"

namespace VKE
{
    struct SSceneInfo
    {

    };

    namespace RenderSystem
    {
        class CDrawcall;
    }

    namespace Scene
    {
        class CScene
        {
            struct SInternal;

            public:

            Result Create(const SSceneInfo&);
            void Destroy();

            void AddObject(const RenderSystem::CDrawcall*);

            protected:

                SInternal*  m_pInternal = nullptr;
        };
    }
} // VKE
