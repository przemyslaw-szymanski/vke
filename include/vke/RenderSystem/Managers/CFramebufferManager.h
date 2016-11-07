#pragma once

#include "Core/Resources/TCManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CRenderSystem;
        class CFramebuffer;
        class CFramebufferManager : public Resources::TCManager< CFramebuffer >
        {
            using Base = Resources::TCManager< CFramebuffer >;
            
            public:

                CFramebufferManager(CRenderSystem*);

            protected:

                Resources::CManager::ResourceRawPtr _AllocateMemory(const Resources::SCreateDesc* const pInfo) override;

            protected:

                CRenderSystem*  m_pRenderSystem;
        };
    } // RenderSystem
} // VKE