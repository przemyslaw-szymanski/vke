#pragma once
#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CTextureView.h"
#include "Core/VKEConfig.h"
#include "RenderSystem/CDeviceDriverInterface.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        /*struct STextureInitDesc
        {
            using ImageVec = Utils::TCDynamicArray< VkImage, 4 >;
            using TextureViewVec = Utils::TCDynamicArray< CTextureView, Config::RenderSystem::Texture::MAX_VIEW_COUNT >;
            STextureDesc    Desc;
            CDeviceContext* pContext = nullptr;
            VkImage         hNative = VK_NULL_HANDLE;
            TextureViewVec  vTextureViews;
            TEXTURE_LAYOUT  layout = TextureLayouts::UNDEFINED;
            TextureHandle   hTexture = NULL_HANDLE;
        };*/

        class VKE_API CTexture : public Resources::CResource
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            friend class CRenderSystem;
            friend class CResourceManager;
            friend class CTextureView;
            friend class CTextureManager;

            using ViewArray = Utils::TCDynamicArray< TextureViewHandle, Config::RenderSystem::Texture::MAX_VIEW_COUNT >;

            public:

                            CTexture(CTextureManager* pMgr);
                            ~CTexture();

                void                    Init(const STextureDesc& Desc);

                const STextureDesc&     GetDesc() const { return m_Desc; }

                static hash_t           CalcHash( const STextureDesc& Desc );

                void                    ChangeLayout(CommandBufferPtr pCommandBuffer, TEXTURE_LAYOUT newLayout);

                const DDIImage&         GetDDIObject() const { return m_hDDIObject; }

            protected:

                void                    _AddView( TextureViewHandle hView ) { m_vViews.PushBack( hView ); }
                ViewArray&              _GetViews() { return m_vViews; }

            protected:

                STextureDesc        m_Desc;
                ViewArray           m_vViews;
                DDIImage            m_hDDIObject = DDINullHandle;
                CTextureManager*    m_pMgr;
        };

        using TextureRefPtr = Utils::TCObjectSmartPtr< CTexture >;
        using TexturePtr = Utils::TCWeakPtr< CTexture >;

        class VKE_API CTextureView : public Resources::CResource
        {
            friend class CTexture;
            friend class CTextureManager;
            friend class CResourceManager;
            friend class CDeviceContext;

            public:

                CTextureView();
                ~CTextureView();

                void                Init( const STextureViewDesc& Desc );

                const STextureViewDesc& GetDesc() const { return m_Desc; }

                const DDIImageView& GetDDIObject() const { return m_hDDIObject; }

            protected:

                STextureViewDesc    m_Desc;
                DDIImageView        m_hDDIObject = DDINullHandle;
        };

        using TextureViewRefPtr = Utils::TCObjectSmartPtr< CTextureView >;
        using TextureViewPtr = Utils::TCWeakPtr< CTextureView >;

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER