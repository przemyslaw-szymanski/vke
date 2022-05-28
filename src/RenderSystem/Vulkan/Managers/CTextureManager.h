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

            template<class T> using TaskPool = Utils::TSFreePool<T, uint32_t, 1024>;
            // using CreateShaderTaskPool = TaskPool< ShaderManagerTasks::SCreateShaderTask >;
            using LoadTextureTask = Threads::TSDataTypedTask<Core::SLoadFileInfo>;
            using LoadTextureTaskPool = TaskPool<LoadTextureTask>;

            //using TextureBuffer = Core::TSResourceBuffer< TextureRefPtr, CTexture* >;
            //using TextureViewBuffer = Core::TSResourceBuffer< TextureViewRefPtr, CTextureView* >;
            //using TextureBuffer = Utils::TSFreePool< CTexture*, TextureHandle >;
            using TextureBuffer = Core::TSVectorResourceBuffer< CTexture*, CTexture*, 1 >;
            using TextureViewBuffer = Utils::TSFreePool< CTextureView* >;
            using RenderTargetBuffer = Utils::TSFreePool< CRenderTarget* >;
            using RenderTargetNameMap = vke_hash_map< cstr_t, RenderTargetHandle >;
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
                TextureHandle       CreateTexture(const Core::ImageHandle& hImg);
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
                RenderTargetRefPtr  GetRenderTarget( cstr_t pName );
                void                DestroyRenderTarget( RenderTargetHandle* phRT );

                SamplerHandle       CreateSampler( const SSamplerDesc& Desc );
                SamplerRefPtr       GetSampler( const SamplerHandle& hSampler );
                void                DestroySampler( SamplerHandle* phSampler );

                Result              UpdateTexture(  const SUpdateMemoryInfo& Info, TextureHandle* phInOut );

            protected:

                CTexture*           _CreateTextureTask( const STextureDesc& Desc );
                CTexture*           _CreateTexture(const Core::ImageHandle& hImg);
                CTexture*           _LoadTextureTask(const Core::SLoadFileInfo& Info);
                CTexture*           _CreateTextureFromImage(const Core::ImageHandle& hImg);
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
                RenderTargetNameMap     m_mRenderTargetNames;
                //FreeTextureType          m_FreeTextures;
                Threads::SyncObject     m_SyncObj;
                LoadTextureTaskPool     m_LoadTaskPool;
                TexMemMgr               m_TexMemMgr;
                TexViewMemMgr           m_TexViewMemMgr;
                RenderTargetMemMgr      m_RenderTargetMemMgr;
                SamplerMemMgr           m_SamplerMemMgr;
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER