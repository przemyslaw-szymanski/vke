#pragma once

#include "Core/Resources/TCManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CRenderSystem;
        class CManagerBase : public Resources::CManager
        {
        public:

        public:

            CManagerBase(CRenderSystem*);
            virtual ~CManagerBase();

        protected:

            virtual ResourceRawPtr _AllocateMemory(const Resources::SCreateDesc* const);

        protected:

            CRenderSystem*  m_pRenderSystem;
        };
    } // RenderSystem
} // VKE