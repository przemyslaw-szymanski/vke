#pragma once

#include "Core/Resources/TCManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CRenderSystem;
        class CFramebuffer;
        class CFramebufferManager //: public Core::TCManager< CFramebuffer >
        {
            //using Base = Core::TCManager< CFramebuffer >;
            
            public:

                CFramebufferManager(CRenderSystem*);

            protected:

                //Core::CManager::ResourceRawPtr _AllocateMemory(const Core::SCreateDesc* const pInfo) override;

            protected:

                CRenderSystem*  m_pRenderSystem;
        };
    } // RenderSystem
} // VKE