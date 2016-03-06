#pragma once

#include "RenderSystem/Common.h"
#include "Resource/CResource.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDevice;

        class VKE_API CCommandBuffer : public Resource::CResource
        {
            enum class State
            {
                CREATED,
                BEGIN,
                END,
            };

            public:

                struct SCreateInfo : public Resource::SCreateInfo
                {
              
                };

            public:

                CCommandBuffer(CDevice* pDevice, Resource::CManager*);
                virtual ~CCommandBuffer();

                Result  Create(const handle_t& handle, CCommandBuffer* pPrimary);
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

                CDevice*        m_pDevice;
                CCommandBuffer* m_pPrimary = nullptr;
                CCommandBuffer* m_pSecondary = nullptr;
                handle_t        m_handle = NULL_HANDLE;
                State           m_state;
        };
    } // RendeSystem
} // VKE
