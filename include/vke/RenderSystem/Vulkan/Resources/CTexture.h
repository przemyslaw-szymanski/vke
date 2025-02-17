#pragma once
#include "RenderSystem/Common.h"
#include "Core/Resources/Common.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CTextureView.h"
#include "Core/VKEConfig.h"
#include "RenderSystem/CDDI.h"
#include "Core/Resources/CImage.h"

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
            TextureHandle   hTexture = INVALID_HANDLE;
        };*/

        class VKE_API CSampler
        {
            friend class CTexture;
            friend class CTextureManager;
            friend class CDeviceContext;
            friend class CContextBase;

            VKE_ADD_DDI_OBJECT( DDISampler );
            VKE_DECL_BASE_OBJECT( SamplerHandle );

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

        class VKE_API CTextureView
        {
            friend class CTexture;
            friend class CTextureManager;
            friend class CResourceManager;
            friend class CDeviceContext;

            VKE_ADD_DDI_OBJECT( DDITextureView );
            VKE_DECL_BASE_OBJECT( TextureViewHandle );

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

        class VKE_API CTexture
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
                static hash_t           CalcHash( cstr_t pName );

                bool                    SetState( TEXTURE_STATE state, STextureBarrierInfo* pOut );

                bool                    SetState( TEXTURE_STATE state, uint16_t mipmapLevel, STextureBarrierInfo* pOut );

                TEXTURE_STATE           GetState() const { return m_state; }

                TextureViewRefPtr       GetView();
                SamplerRefPtr           GetSampler();

                DDISampler              GetDDISampler();

                bool IsColor() const { return m_isColor; }
                bool IsDepth() const { return m_isDepth; }
                bool IsStencil() const { return m_isStencil; }
                bool IsReady();

                static TEXTURE_ASPECT       ConvertFormatToAspect( const TEXTURE_FORMAT format );
                static MEMORY_ACCESS_TYPE   ConvertStateToSrcMemoryAccess( const TEXTURE_STATE currentState,
                    const TEXTURE_STATE newState );
                static MEMORY_ACCESS_TYPE   ConvertStateToDstMemoryAccess( const TEXTURE_STATE currentState,
                    const TEXTURE_STATE newState );

                ImageRefPtr GetImage() const { return m_pImage; }

                TEXTURE_ASPECT GetAspect() const { return m_aspect; }

                /// <summary>
                /// Notifies TextureManager that this resource is ready to use.
                /// Must be called manually for resources uploaded resources.
                /// </summary>
                void NotifyReady();

                /// <summary>
                /// Sets command buffer that executes commands for this texture.
                /// Possible commands are: data upload (copy), mipmap generation (blit)
                /// </summary>
                /// <param name="pCmdBuffer"></param>
                void SetCommandBuffer(CommandBufferPtr pCmdBuffer) { m_pCmdBuffer = pCmdBuffer; }
                CommandBufferPtr GetCommandBuffer() const { return m_pCmdBuffer; }

            protected:

                void                    _Destroy() {}
                //void                    _AddView( TextureViewHandle hView ) { m_vViews.PushBack( hView ); }
                //ViewArray&              _GetViews() { return m_vViews; }

            protected:

                STextureDesc            m_Desc;

                VKE_ADD_DDI_OBJECT(DDITexture);
                VKE_DECL_BASE_OBJECT(TextureHandle);
                VKE_DECL_BASE_RESOURCE();

            protected:

                CommandBufferPtr        m_pCmdBuffer;
                TextureViewHandle       m_hView = INVALID_HANDLE;
                SamplerHandle           m_hSampler = INVALID_HANDLE;
                CTextureManager*        m_pMgr;
                ImageRefPtr             m_pImage;
                handle_t                m_hMemory = INVALID_HANDLE;
                //DDIFence                m_hFence = DDI_NULL_HANDLE;
                TEXTURE_STATE           m_state = TextureStates::UNDEFINED;
                TEXTURE_ASPECT          m_aspect = TextureAspects::UNKNOWN;
                bool m_isColor : 1;
                bool m_isDepth : 1;
                bool m_isStencil : 1;
                bool m_isReady : 1;
                bool m_canGenerateMipmaps : 1;
                bool pad : 3;
                bool m_isDataUploaded = false;
        };

        using TextureRefPtr = Utils::TCObjectSmartPtr< CTexture >;
        using TexturePtr = Utils::TCWeakPtr< CTexture >;

        class VKE_API CRenderTarget
        {
            friend class CGraphicsContext;
            friend class CDeviceContext;
            friend class CRenderingPipeline;
            friend class CSwapChain;
            friend class CTextureManager;

            using Desc = SRenderPassDesc::SRenderTargetDesc;

            VKE_DECL_BASE_OBJECT( RenderTargetHandle );

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
#endif // VKE_VULKAN_RENDER_SYSTEM