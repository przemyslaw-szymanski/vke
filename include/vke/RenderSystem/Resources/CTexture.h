#pragma once
#include "RenderSystem/Common.h"
#include "Core/CObject.h"
#include "Core/Utils/TCSmartPtr.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        class CTexture final : public Core::CObject
        {
            friend class CDeviceContext;
            public:

                CTexture(CDeviceContext* pCtx);
                ~CTexture();

                void    Init(TextureHandle hNative);
                void    ChangeLayout(CommandBufferPtr pPtr);

            protected:

                CDeviceContext*     m_pCtx;
                TextureHandle       m_hNative;
        };
        using TexturePtr = Utils::TCWeakPtr< CTexture >;
        using TextureSmartPtr = Utils::TCWeakPtr< CTexture >;
    }
} // VKE