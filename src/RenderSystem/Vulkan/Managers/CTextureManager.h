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

            //using TextureBuffer = Core::TSResourceBuffer< TextureRefPtr, CTexture* >;
            //using TextureViewBuffer = Core::TSResourceBuffer< TextureViewRefPtr, CTextureView* >;
            using TextureBuffer = Utils::TSFreePool< TextureRefPtr >;
            using TextureViewBuffer = Utils::TSFreePool< TextureViewRefPtr >;

            using TextureViewNameMap = vke_hash_map< hash_t, TextureViewPtr >;

            using TexMemMgr = Memory::CFreeListPool;
            using TexViewMemMgr = Memory::CFreeListPool;

            public:

                CTextureManager( CDeviceContext* pCtx );
                ~CTextureManager();

                Result              Create( const STextureManagerDesc& Desc );
                void                Destroy();

                TextureHandle       CreateTexture( const STextureDesc& Desc );
                void                DestroyTexture( TextureHandle* phTexture );
                //void                FreeTexture( TextureHandle* phTexture );
                TextureRefPtr       GetTexture( TextureHandle hTexture );

                TextureViewHandle   CreateTextureView( const STextureViewDesc& Desc );
                void                DestroyTextureView( TextureViewHandle* phTextureView );
                //void                FreeTextureView( TextureViewHandle* phTextureView );
                TextureViewRefPtr   GetTextureView( TextureViewHandle hTextureView );

            protected:

                CTexture*           _CreateTextureTask( const STextureDesc& Desc );
                void                _DestroyTexture( CTexture** ppInOut );

                CTextureView*       _CreateTextureViewTask( const STextureDesc& Desc );
                void                _DestroyTextureView( CTextureView** ppInOut );

                CTexture*           _FindFreeTextureForReuse( const STextureDesc& Desc );
                void                _FreeTexture( CTexture** ppInOut );
                void                _AddTexture( CTexture* pTexture );

                CTextureView*       _FindFreeTextureViewForReuse( const STextureViewDesc& Desc );
                void                _FreeTextureView( CTextureView** ppInOut );
                void                _AddTextureView( CTextureView* pTexture );

            protected:

                CDeviceContext*         m_pCtx;
                TextureBuffer           m_Textures;
                TextureViewBuffer       m_TextureViews;
                //FreeTextureType          m_FreeTextures;
                Threads::SyncObject     m_SyncObj;
                TexMemMgr               m_TexMemMgr;
                TexViewMemMgr           m_TexViewMemMgr;
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER