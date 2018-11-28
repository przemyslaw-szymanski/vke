#pragma once
#if VKE_VULKAN_RENDERER
#include "Core/Managers/CResourceManager.h"
#include "Core/Memory/CFreeListPool.h"
#include "Core/Threads/ITask.h"
#include "RenderSystem/Resources/CTexture.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CTextureManager;

        struct STextureManagerDesc
        {

        };

        class CTextureManager
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CTransferContext;
            friend class CTexture;

            struct SFreeTextureHandle
            {
                hash_t      descHash;
                uint32_t    idx;
            };

            using TextureBuffer = Utils::TCDynamicArray< TextureRefPtr, Config::RenderSystem::Texture::MAX_COUNT >;
            using FreeTextureBuffer = Utils::TCDynamicArray< SFreeTextureHandle, Config::RenderSystem::Texture::MAX_COUNT >;
            using TexturePool = Core::TSMultimapResourceBuffer< TextureRefPtr, CTexture* >;

            using TextureViewBuffer = Utils::TCDynamicArray< TextureViewRefPtr, Config::RenderSystem::TextureView::MAX_COUNT >;
            using FreeTextureViewBuffer = Utils::TCDynamicArray< TextureViewRefPtr, Config::RenderSystem::TextureView::MAX_COUNT >;
            using TextureViewPool = Core::TSMultimapResourceBuffer< TextureViewRefPtr, CTextureView* >;

            using TexMemMgr = Memory::CFreeListPool;
            using TexViewMemMgr = Memory::CFreeListPool;

            public:

                CTextureManager( CDeviceContext* pCtx );
                ~CTextureManager();

                Result              Create( const STextureManagerDesc& Desc );
                void                Destroy();

                TextureHandle       CreateTexture( const STextureDesc& Desc );
                void                DestroyTexture( TexturePtr* pInOut );
                void                FreeTexture( TexturePtr* pInOut );

                TextureRefPtr       GetTexture( TextureHandle hTexture );

            protected:

                CTexture*           _CreateTextureTask( const STextureDesc& Desc );
                void                _DestroyTexture( CTexture** ppInOut );

                CTexture*           _FindFreeTextureForReuse( const STextureDesc& Desc );
                void                _FreeTexture( CTexture** ppInOut );
                void                _AddTexture( CTexture* pTexture );

            protected:

                CDeviceContext*         m_pCtx;
                TexturePool              m_Textures;
                //FreeTextureType          m_FreeTextures;
                Threads::SyncObject     m_SyncObj;
                TexMemMgr               m_TexMemMgr;
                TexViewMemMgr           m_TexViewMemMgr;
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER