#pragma once

#include "CResource.h"

namespace VKE
{
    namespace RenderSystem
    {

        class CFramebuffer : public CResource
        {
            friend class CFramebufferManager;
        public:
            CFramebuffer();
            explicit CFramebuffer(CGraphicsContext*);
            CFramebuffer(CGraphicsContext*, Resources::CManager*);
            CFramebuffer(CGraphicsContext*, Resources::CManager*, handle_t);
            ~CFramebuffer(){}

        protected:
        };
    } // RenderSystem
} // VKE