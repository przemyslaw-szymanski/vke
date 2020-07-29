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
            //using TextureBuffer = Utils::TSFreePool< CTexture*, TextureHandle >;
            using TextureBuffer = Core::TSVectorResourceBuffer< CTexture*, CTexture*, 1 >;
            using TextureViewBuffer = Utils::TSFreePool< CTextureView* >;
            using RenderTargetBuffer = Utils::TSFreePool< CRenderTarget* >;
            using SamplerMap = Core::TSHashMap < hash_t, CSampler* >;

            using TextureViewNameMap = vke_hash_map< hash_t, TextureViewPtr >;

            using TexMemMgr = Memory::CFreeListPool;
            using TexViewMemMgr = Memory::CFreeListPool;
            using RenderTargetMemMgr = Memory::CFreeListPool;
            using SamplerMemMgr = Memory::CFreeListPool;

            public:

                CTextureManager( CDeviceContext* pCtx );
                ~CTextureManager();

                Result              Create( const STextureManagerDesc& Desc );
                void                Destroy();

                TextureHandle       CreateTexture( const STextureDesc& Desc );
                TextureHandle       LoadTexture(const Core::SLoadFileInfo& Info);
                void                DestroyTexture( TextureHandle* phTexture );
                //void                FreeTexture( TextureHandle* phTexture );
                TextureRefPtr       GetTexture( TextureHandle hTexture );

                TextureViewHandle   CreateTextureView( const STextureViewDesc& Desc );
                void                DestroyTextureView( TextureViewHandle* phTextureView );
                //void                FreeTextureView( TextureViewHandle* phTextureView );
                TextureViewRefPtr   GetTextureView( TextureViewHandle hTextureView );
                TextureViewRefPtr   GetTextureView( const TextureHandle& hTexture );
                TextureViewRefPtr   GetTextureView( const RenderTargetHandle& hRT );

                RenderTargetHandle  CreateRenderTarget( const SRenderTargetDesc& Desc );
                RenderTargetRefPtr  GetRenderTarget( const RenderTargetHandle& hRT );
                void                DestroyRenderTarget( RenderTargetHandle* phRT );

                SamplerHandle       CreateSampler( const SSamplerDesc& Desc );
                SamplerRefPtr       GetSampler( const SamplerHandle& hSampler );
                void                DestroySampler( SamplerHandle* phSampler );

                Result              UpdateTexture(  const SUpdateMemoryInfo& Info, TextureHandle* phInOut );

            protected:

                CTexture*           _CreateTextureTask( const STextureDesc& Desc );
                CTexture*           _LoadTextureTask(const Core::SLoadFileInfo& Info);
                void                _DestroyTexture( CTexture** ppInOut );
                Result              _UpdateTextureTask(const SUpdateMemoryInfo& Info, CTexture** ppInOut);

                CTextureView*       _CreateTextureViewTask( const STextureDesc& Desc );
                void                _DestroyTextureView( CTextureView** ppInOut );

                CTexture*           _FindFreeTextureForReuse( const STextureDesc& Desc );
                void                _FreeTexture( CTexture** ppInOut );
                void                _AddTexture( CTexture* pTexture );

                CTextureView*       _FindFreeTextureViewForReuse( const STextureViewDesc& Desc );
                void                _FreeTextureView( CTextureView** ppInOut );
                void                _AddTextureView( CTextureView* pTexture );

                void                _DestroyRenderTarget( CRenderTarget** ppInOut );
                void                _DestroySampler( CSampler** ppInOut );

            protected:

                CDeviceContext*         m_pCtx;
                TextureBuffer           m_Textures;
                TextureViewBuffer       m_TextureViews;
                SamplerMap              m_Samplers;
                RenderTargetBuffer      m_RenderTargets;
                //FreeTextureType          m_FreeTextures;
                Threads::SyncObject     m_SyncObj;
                TexMemMgr               m_TexMemMgr;
                TexViewMemMgr           m_TexViewMemMgr;
                RenderTargetMemMgr      m_RenderTargetMemMgr;
                SamplerMemMgr           m_SamplerMemMgr;
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER