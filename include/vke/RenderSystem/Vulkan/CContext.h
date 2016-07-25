#pragma once

#include "RenderSystem/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CSWapChain;
        struct SInternal;

        class CContext
        {
            friend class CRenderSystem;
            friend class CDevice;
            public:

            CContext();
            ~CContext();

            Result Create(const SContextInfo& Info);
            void Destroy();

            void RenderFrame(const handle_t& hSwapChain);

            protected:

            Result          _CreateDevices();
            Result          _CreateDevice(const SDeviceInfo& Info);
            
            void*           _GetInstance() const;

            const
            void*   _GetInstanceFunctions() const;
            const
            void*     _GetGlobalFunctions() const;

            protected:

                SContextInfo    m_Info;

                SInternal*      m_pInternal = nullptr;
        };
    } // RenderSystem
} // vke