#pragma once
#include "RenderSystem/Common.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CTextureView.h"
#include "Core/VKEConfig.h"
#include "RenderSystem/CDDI.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
        class CTextureView;
        class CSampler;
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

        class VKE_API CSampler final : public Resources::CResource
        {
            friend class CTexture;
            friend class CTextureManager;
            friend class CDeviceContext;
            friend class CContextBase;

            VKE_ADD_DDI_OBJECT( DDISampler );

            public:

                CSampler( CTextureManager* pMgr );
                ~CSampler();

                void    Init( const SSamplerDesc& Desc );

                static hash_t CalcHash( const SSamplerDesc& Desc );

            protected:

                void    _Destroy();

            protected:

                SSamplerDesc    m_Desc;
        };

        class VKE_API CTextureView final : public Resources::CResource
        {
            friend class CTexture;
            friend class CTextureManager;
            friend class CResourceManager;
            friend class CDeviceContext;

            VKE_ADD_DDI_OBJECT( DDITextureView );

            public:

                CTextureView();
                ~CTextureView();

                void                    Init( const STextureViewDesc& Desc, TexturePtr pTexture );

                static hash_t           CalcHash( const STextureViewDesc& Desc );

                const STextureViewDesc& GetDesc() const { return m_Desc; }

            protected:

                void                    _Destroy() {}

            protected:

                STextureViewDesc    m_Desc;
            };

        using TextureViewRefPtr = Utils::TCObjectSmartPtr< CTextureView >;
        using TextureViewPtr = Utils::TCWeakPtr< CTextureView >;

        class VKE_API CTexture final : public Resources::CResource
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            friend class CRenderSystem;
            friend class CResourceManager;
            friend class CTextureView;
            friend class CTextureManager;

            using ViewArray = Utils::TCDynamicArray< TextureViewHandle, Config::RenderSystem::Texture::MAX_VIEW_COUNT >;

            VKE_ADD_DDI_OBJECT( DDITexture );

            public:

                            CTexture(CTextureManager* pMgr);
                            ~CTexture();

                void                    Init(const STextureDesc& Desc);

                const STextureDesc&     GetDesc() const { return m_Desc; }

                static hash_t           CalcHash( const STextureDesc& Desc );

                void                    SetState( const TEXTURE_STATE& state, STextureBarrierInfo* pOut );

                TEXTURE_STATE           GetState() const { return m_state; }

                TextureViewRefPtr       GetView();
                SamplerRefPtr           GetSampler();

                DDITextureView          GetDDIView();
                DDISampler              GetDDISampler();

                static TEXTURE_ASPECT       ConvertFormatToAspect( const TEXTURE_FORMAT& format );
                static MEMORY_ACCESS_TYPE   ConvertStateToSrcMemoryAccess( const TEXTURE_STATE& currentState, const TEXTURE_STATE& newState );
                static MEMORY_ACCESS_TYPE   ConvertStateToDstMemoryAccess( const TEXTURE_STATE& currentState, const TEXTURE_STATE& newState );


            protected:

                void                    _Destroy() {}
                //void                    _AddView( TextureViewHandle hView ) { m_vViews.PushBack( hView ); }
                //ViewArray&              _GetViews() { return m_vViews; }

            protected:

                STextureDesc            m_Desc;
                TextureViewHandle       m_hView;
                SamplerHandle           m_hSampler;
                CTextureManager*        m_pMgr;
                handle_t                m_hMemory = NULL_HANDLE;
                TEXTURE_STATE           m_state = TextureStates::UNDEFINED;
        };

        using TextureRefPtr = Utils::TCObjectSmartPtr< CTexture >;
        using TexturePtr = Utils::TCWeakPtr< CTexture >;

        class VKE_API CRenderTarget final : public Core::CObject
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            friend class CRenderingPipeline;
            friend class CSwapChain;
            friend class CTextureManager;

            using Desc = SRenderPassDesc::SRenderTargetDesc;

            public:

                CRenderTarget(  );
                ~CRenderTarget();

                void Init( const SRenderTargetDesc& Desc );

                const Desc& GetDesc() const { return m_Desc; }
                const TextureSize& GetSize() const { return m_Size; }

                const TextureHandle&        GetTexture() const { return m_hTexture; }
                const TextureViewHandle&    GetTextureView() const { return m_Desc.hTextureView; }

            protected:

                void    _Destroy();

            protected:

                Desc                m_Desc;
                TextureSize         m_Size;
                TextureHandle       m_hTexture;
        };

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER