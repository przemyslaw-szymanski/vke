#pragma once

#include "Core/Resources/CResource.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CGraphicsContext;
        class CResource : public Resources::CResource
        {
        public:
            CResource() :
                CResource(nullptr, nullptr, NULL_HANDLE) {}
            explicit CResource(CGraphicsContext* pCtx) :
                CResource(pCtx, nullptr) {}
            CResource(CGraphicsContext* pCtx, Resources::CManager* pMgr) :
                CResource(pCtx, pMgr, NULL_HANDLE) {}
            CResource(CGraphicsContext* pCtx, Resources::CManager* pMgr, handle_t hRes) :
                Resources::CResource(pMgr, hRes),
                m_pCtx(pCtx) {}

            virtual ~CResource() {}

        protected:

            CGraphicsContext*   m_pCtx = nullptr;
            handle_t    m_hResource = 0;
        };
    } // RenderSystem
} // VKE