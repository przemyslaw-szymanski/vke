#pragma once
#include "RenderSystem/Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CTexture.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
        class CAPIResourceManager;

        namespace Managers
        {
            class VKE_API CResourceManager
            {
                friend class CDeviceContext;
                friend class CAPIResourceManager;

                    using TextureBuffer = Utils::TSFreePool< TextureSmartPtr, CTexture* >;

                    Memory::IAllocator*     m_pTextureAllocator = &HeapAllocator;

                public:

                                        CResourceManager(CDeviceContext* pCtx);
                                        ~CResourceManager();

                    Result              Create(const SResourceManagerDesc& Desc);
                    void                Destroy();

                    TexturePtr          CreateTexture(const STextureDesc& Desc);
                    void                FreeTexture(TexturePtr* ppTextureInOut);
                    void                DestroyTexture(TexturePtr* ppTextureInOut);
                    void                DestroyTextures();

                    Result              CreateTextureView(const STextureViewDesc& Desc, TexturePtr* ppTexInOut);
                    Result              CreateTextureView(TexturePtr* ppTexInOut);

                protected:

                    

                protected:

                    CDeviceContext*         m_pCtx;
                    CAPIResourceManager*    m_pAPIResMgr;
                    CommandBufferPtr        m_pInitialCommandBuffer;
                    TextureBuffer           m_Textures;
            };
        } // Managers
    } // RenderSystem
} // VKE