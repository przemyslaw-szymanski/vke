#pragma once

#include "CResource.h"

namespace VKE
{
    namespace RenderSystem
    {

        class CFramebuffer //: public CResource
        {
            friend class CFramebufferManager;
        public:
            CFramebuffer();
            explicit CFramebuffer(CGraphicsContext*);
            CFramebuffer(CGraphicsContext*, Core::CManager*);
            CFramebuffer(CGraphicsContext*, Core::CManager*, handle_t);
            ~CFramebuffer(){}

        protected:
        };
    } // RenderSystem
} // VKE