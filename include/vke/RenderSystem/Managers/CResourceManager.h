#pragma once
#include "RenderSystem/Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CTexture.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
        namespace Managers
        {
            class VKE_API CResourceManager
            {
                friend class CDeviceContext;

                    using TextureBuffer = Utils::TSFreePool< TextureOwnPtr, TexturePtr >;

                    Memory::IAllocator*     m_pTextureAllocator = &HeapAllocator;

                public:

                                        CResourceManager(CDeviceContext* pCtx);
                                        ~CResourceManager();

                    Result              Create(const SResourceManagerDesc& Desc);
                    void                Destroy();

                    TexturePtr          CreateTexture(const STextureDesc& Desc);
                    void                FreeTexture(TexturePtr* ppTextureInOut);
                    void                DestroyTexture(TexturePtr* ppTextureInOut);

                    Result              CreateTextureView(const STextureViewDesc& Desc, TexturePtr* ppTexInOut);
                    Result              CreateTextureView(TexturePtr* ppTexInOut);

                protected:

                    CDeviceContext*     m_pCtx;
                    CommandBufferPtr    m_pInitialCommandBuffer;
                    TextureBuffer       m_Textures;
            };
        } // Managers
    } // RenderSystem
} // VKE