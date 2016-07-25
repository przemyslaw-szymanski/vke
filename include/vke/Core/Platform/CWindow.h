#pragma once

#include "Core/Platform/Common.h"
#include "Core/Utils/TCSmartPtr.h"

namespace VKE
{
    struct SWindowInternal;

    namespace RenderSystem
    {
        class CContext;
    }

    class CRenderSystem;

    class VKE_API CWindow
    {
        public:

            CWindow();
            ~CWindow();

            Result Create(const SWindowInfo& Info);
            void Destroy();

            void SetSwapChainHandle(const handle_t& hSwapChain) { m_hSwapChain = hSwapChain; }
            vke_force_inline
            const handle_t& GetSwapChainHandle() const { return m_hSwapChain; }

            void Update();

            const SWindowInfo& GetInfo() const
            {
                return m_Info; 
            }

            bool NeedQuit() const;
            void NeedQuit(bool need);

            void Show(bool bShow);

            void SetContext(RenderSystem::CContext* pCtx);
            void SetRenderSystem(CRenderSystem* pRS);

        protected:

            SWindowInfo         m_Info;
            SWindowInternal*    m_pInternal = nullptr;
            handle_t            m_hSwapChain = NULL_HANDLE;
            bool                m_needQuit = false;
    };

    using WindowPtr = Utils::TCWeakPtr< CWindow >;
    using WindowOwnPtr = Utils::TCUniquePtr< CWindow >;

} // vke
