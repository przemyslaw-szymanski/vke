#pragma once

#include "RenderSystem/Vulkan/Common.h"
#include "Core/Resources/CResource.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDevice;

        class VKE_API CCommandBuffer : public Resources::CResource
        {
            friend class CCommandBufferManager;

            enum class State
            {
                UNKNOWN,
                CREATED,
                BEGIN,
                END,
                SUBMITED,
            };

            public:

                struct SCreateDesc : public Resources::SCreateDesc
                {
              
                };

            public:

                CCommandBuffer();
                virtual ~CCommandBuffer();

                Result  Create(CDevice* pDevice, CCommandBufferManager* pMgr, const handle_t& handle,
                               CCommandBuffer* pPrimary);
                void    Destroy();

                void    Begin();
                void    End();
                void    Submit();
                void    Barrier();
                void    ClearColor();
                void    Draw();

                vke_force_inline
                State   GetState() const { return m_state; }

            protected:

                CDevice*                m_pDevice = nullptr;
                CCommandBufferManager*  m_pManager = nullptr;
                CCommandBuffer*         m_pPrimary = nullptr;
                CCommandBuffer*         m_pSecondary = nullptr;
                handle_t                m_handle = 0;
                State                   m_state = State::UNKNOWN;
        };
    } // RendeSystem
} // VKE
