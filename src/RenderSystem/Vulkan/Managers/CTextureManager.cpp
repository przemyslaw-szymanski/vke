#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Managers/CTextureManager.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"

namespace VKE
{
    namespace RenderSystem
    {

        TEXTURE_ASPECT ConvertTextureUsageToAspect(const TEXTURE_USAGE& usage)
        {
            TEXTURE_ASPECT ret = TextureAspects::UNKNOWN;
            if( usage & TextureUsages::COLOR_RENDER_TARGET ||
                usage & TextureUsages::FILE_IO )
            {
                ret = TextureAspects::COLOR;
            }
            else if( usage & TextureUsages::DEPTH_STENCIL_RENDER_TARGET )
            {
                ret = TextureAspects::DEPTH_STENCIL;
            }
            return ret;
        }

        CTextureManager::CTextureManager( CDeviceContext* pCtx ) :
            m_pCtx{ pCtx }
        {

        }

        CTextureManager::~CTextureManager()
        {
            Destroy();
        }

        void CTextureManager::Destroy()
        {
            for( auto& Pair : m_Samplers.Container )
            {
                auto& pCurr = Pair.second;
                if( pCurr )
                {
                    m_pCtx->DDI().DestroyObject( &pCurr->m_hDDIObject, nullptr );
                }
            }

            for( uint32_t i = 1; i < m_TextureViews.vPool.GetCount(); ++i )
            {
                auto& pCurr = m_TextureViews[i];
                if( pCurr )
                {
                    m_pCtx->DDI().DestroyObject( &pCurr->m_hDDIObject, nullptr );
                }
            }
            for( uint32_t i = 1; i < m_Textures.vPool.GetCount(); ++i )
            {
                auto& pCurr = m_Textures[i];
                if( pCurr )
                {
                    m_pCtx->DDI().DestroyObject( &pCurr->m_hDDIObject, nullptr );
                }
            }
            
            m_TextureViews.Clear();
            m_Textures.Clear();
            m_RenderTargets.Clear();
            m_Samplers.Clear();

            m_SamplerMemMgr.Destroy();
            m_RenderTargetMemMgr.Destroy();
            m_TexViewMemMgr.Destroy();
            m_TexMemMgr.Destroy();
        }

        Result CTextureManager::Create( const STextureManagerDesc& Desc )
        {
            Result ret = VKE_OK;
            
            m_Textures.Add( {} );
            m_TextureViews.Add( {} );
            m_RenderTargets.Add( {} );
            m_Samplers[0] = {};

            uint32_t count = Config::RenderSystem::Texture::MAX_COUNT;
            ret = m_TexMemMgr.Create( count, sizeof( CTexture ), 1 );
            if( VKE_SUCCEEDED( ret ) )
            {
                count = Config::RenderSystem::TextureView::MAX_COUNT;
                ret = m_TexViewMemMgr.Create( count, sizeof( CTextureView ), 1 );
                if( VKE_SUCCEEDED( ret ) )
                {
                    count = Config::RenderSystem::RenderTarget::MAX_COUNT;
                    ret = m_RenderTargetMemMgr.Create( count, sizeof( CRenderTarget ), 1 );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        count = Config::RenderSystem::Sampler::MAX_COUNT;
                        ret = m_SamplerMemMgr.Create( count, sizeof( CSampler ), 1 );
                    }
                }
            }
            return ret;
        }

        TextureHandle CTextureManager::CreateTexture( const STextureDesc& Desc )
        {
            hash_t hash = CTexture::CalcHash( Desc );
            CTexture* pTex = nullptr;
            TextureHandle hTex = TextureHandle{ static_cast<handle_t>(hash) };
            uint32_t handle;

            if( VKE_SUCCEEDED( Memory::CreateObject( &m_TexMemMgr, &pTex, this ) ) )
            {
                handle = m_Textures.Add( ( pTex ) );
                if( handle == UNDEFINED_U32 )
                {
                    goto ERR;
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to create memory for Texture." );
                goto ERR;
            }
            
            if( pTex != nullptr )
            {
                pTex->Init( Desc );
                {
                    if( pTex->GetDDIObject() == DDI_NULL_HANDLE )
                    {
                        m_pCtx->DDI().UpdateDesc( &pTex->m_Desc );

                        pTex->m_hDDIObject = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
                    }
                    if( pTex->m_hDDIObject != DDI_NULL_HANDLE )
                    {
                        // Create memory for buffer
                        SAllocateDesc AllocDesc;
                        AllocDesc.Memory.hDDITexture = pTex->GetDDIObject();
                        AllocDesc.Memory.memoryUsages = Desc.memoryUsage | MemoryUsages::TEXTURE;
                        AllocDesc.Memory.size = 0;
                        AllocDesc.poolSize = VKE_MEGABYTES( 10 );
                        pTex->m_hMemory = m_pCtx->_GetDeviceMemoryManager().AllocateTexture( AllocDesc );
                        if( pTex->m_hMemory )
                        {
                            hTex.handle = handle;
                            pTex->m_hObject = hTex.handle;
                        }
                        else
                        {
                            goto ERR;
                        }
                    }
                    else
                    {
                        goto ERR;
                    }
                }
            }

            return hTex;
        ERR:
            pTex->_Destroy();
            Memory::DestroyObject( &m_TexMemMgr, &pTex );
            return hTex;
        }

        TextureViewHandle CTextureManager::CreateTextureView( const STextureViewDesc& Desc )
        {
            //hash_t hash = CTextureView::CalcHash( Desc );
            CTextureView* pView = nullptr;
            TextureViewHandle hRet;
            uint32_t handle;

            if( VKE_SUCCEEDED( Memory::CreateObject( &m_TexViewMemMgr, &pView ) ) )
            {
                handle = m_TextureViews.Add( ( pView ) );
            }
            else
            {
                VKE_LOG_ERR( "Unable to create memory for TextureView." );
                goto ERR;
            }
            if( pView != nullptr )
            {
                TexturePtr pTex = GetTexture( Desc.hTexture );
                pView->Init( Desc, pTex );
                {
                    if( pView->m_hDDIObject == DDI_NULL_HANDLE )
                    {
                        pView->m_hDDIObject = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
                    }
                    if( pView->m_hDDIObject != DDI_NULL_HANDLE )
                    {
                        hRet.handle = handle;
                        pView->m_hObject = hRet.handle;
                    }
                    else
                    {
                        goto ERR;
                    }
                }
            }

            return hRet;

        ERR:
            pView->_Destroy();
            Memory::DestroyObject( &m_TexViewMemMgr, &pView );
            return hRet;
        }


        void CTextureManager::DestroyTexture( TextureHandle* phTexture )
        {
            TextureHandle& hTex = *phTexture;
            TextureRefPtr pTex;
            m_Textures.Free( static_cast< uint32_t >( hTex.handle ) );
            {
                CTexture* pTmp = pTex.Release();
                _DestroyTexture( &pTmp );
            }
            hTex = NULL_HANDLE;
        }

        void CTextureManager::_DestroyTexture( CTexture** ppInOut )
        {
            CTexture* pTex = *ppInOut;
            m_pCtx->_GetDDI().DestroyObject( &pTex->m_hDDIObject, nullptr );
            Memory::DestroyObject( &m_TexMemMgr, &pTex );
            *ppInOut = nullptr;
        }


        void CTextureManager::DestroyTextureView( TextureViewHandle* phView )
        {
            TextureViewHandle& hView = *phView;
            TextureViewRefPtr pView;
            m_TextureViews.Free( static_cast< uint32_t >( hView.handle ) );
            {
                CTextureView* pTmp = pView.Release();
                _DestroyTextureView( &pTmp );
                
            }
            hView = NULL_HANDLE;
        }

        void CTextureManager::_DestroyTextureView( CTextureView** ppInOut )
        {
            CTextureView* pView = *ppInOut;
            m_pCtx->_GetDDI().DestroyObject( &pView->m_hDDIObject, nullptr );
            Memory::DestroyObject( &m_TexViewMemMgr, &pView );
            *ppInOut = nullptr;
        }

        RenderTargetHandle CTextureManager::CreateRenderTarget( const SRenderTargetDesc& Desc )
        {
            RenderTargetHandle hRet = NULL_HANDLE;
            STextureDesc TexDesc;
            STextureViewDesc ViewDesc;
            uint32_t handle;

            CRenderTarget*  pRT = nullptr;
            if( VKE_SUCCEEDED( Memory::CreateObject( &m_RenderTargetMemMgr, &pRT ) ) )
            {
                handle = m_RenderTargets.Add( { pRT } );
            }
            else
            {
                VKE_LOG_ERR( "Unable to create memory for RenderTarget." );
                goto ERR;
            }
            if( pRT != nullptr )
            {
                pRT->Init( Desc );
                {
                    TexDesc.format = Desc.format;
                    TexDesc.memoryUsage = Desc.memoryUsage;
                    TexDesc.mipLevelCount = Desc.mipLevelCount;
                    TexDesc.multisampling = Desc.multisampling;
                    TexDesc.Size = Desc.Size;
                    TexDesc.type = Desc.type;
                    TexDesc.usage = Desc.usage;
                    auto hTex = CreateTexture( TexDesc );
                    if( hTex != NULL_HANDLE )
                    {
                        ViewDesc.format = Desc.format;
                        ViewDesc.hTexture = hTex;
                        ViewDesc.SubresourceRange.aspect = ConvertTextureUsageToAspect( Desc.usage );
                        ViewDesc.SubresourceRange.beginArrayLayer = 0;
                        ViewDesc.SubresourceRange.beginMipmapLevel = 0;
                        ViewDesc.SubresourceRange.layerCount = 1;
                        ViewDesc.SubresourceRange.mipmapLevelCount = Desc.mipLevelCount;
                        auto hView = CreateTextureView( ViewDesc );
                        if( hView != NULL_HANDLE )
                        {
                            hRet.handle = handle;

                            pRT->m_hObject = hRet.handle;
                            pRT->m_hTexture = hTex;
                            pRT->m_Desc.hTextureView = hView;
                            pRT->m_Size = Desc.Size;
                        }
                    }
                    else
                    {
                        goto ERR;
                    }
                }
            }
            return hRet;
        ERR:
            _DestroyRenderTarget( &pRT );
            return hRet;
        }

        void CTextureManager::_DestroyRenderTarget( CRenderTarget** ppInOut )
        {
            CRenderTarget* pRT = *ppInOut;

            pRT->_Destroy();
            DestroyTextureView( &pRT->m_Desc.hTextureView );
            DestroyTexture( &pRT->m_hTexture );

            Memory::DestroyObject( &m_RenderTargetMemMgr, ppInOut );
            *ppInOut = nullptr;
        }

        TextureRefPtr CTextureManager::GetTexture( TextureHandle hTexture )
        {
            return TextureRefPtr{ m_Textures[ static_cast<uint32_t>( hTexture.handle )] };
        }

        TextureViewRefPtr CTextureManager::GetTextureView( TextureViewHandle hView )
        {
            return TextureViewRefPtr{ m_TextureViews[ static_cast<uint32_t>( hView.handle ) ] };
        }

        TextureViewRefPtr CTextureManager::GetTextureView( const TextureHandle& hTexture )
        {
            return GetTexture( hTexture )->GetView();
        }

        TextureViewRefPtr CTextureManager::GetTextureView( const RenderTargetHandle& hRT )
        {
            RenderTargetPtr pRT = GetRenderTarget( hRT );
            return GetTextureView( pRT->GetTextureView() );
        }

        RenderTargetRefPtr CTextureManager::GetRenderTarget( const RenderTargetHandle& hRT )
        {
            return RenderTargetRefPtr{ m_RenderTargets[ static_cast<uint32_t>( hRT.handle ) ] };
        }

        void CTextureManager::DestroyRenderTarget( RenderTargetHandle* phRT )
        {
            CRenderTarget* pRT = GetRenderTarget( *phRT ).Release();
            _DestroyRenderTarget( &pRT );
            *phRT = NULL_HANDLE;
        }

        SamplerHandle CTextureManager::CreateSampler( const SSamplerDesc& Desc )
        {
            SamplerHandle hRet = NULL_HANDLE;
            hash_t hash = CSampler::CalcHash( Desc );
            CSampler* pSampler = nullptr;
            SamplerMap::Iterator Itr;
            
            if( !m_Samplers.Find( hash, &pSampler, &Itr ) )
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_SamplerMemMgr, &pSampler, this ) ) )
                {
                    m_Samplers.Insert( Itr, hash, pSampler );

                }
                else
                {
                    VKE_LOG_ERR("Unable to create memory for CSampler object.");
                    goto ERR;
                }
            }
            if( pSampler )
            {
                hRet.handle = hash;
                if( pSampler->GetDDIObject() == DDI_NULL_HANDLE )
                {
                    pSampler->Init( Desc );
                    pSampler->m_hDDIObject = m_pCtx->DDI().CreateObject( pSampler->m_Desc, nullptr );
                    if( pSampler->m_hDDIObject != DDI_NULL_HANDLE )
                    {
                        pSampler->m_hObject = hRet.handle;
                    }
                    else
                    {
                        hRet = NULL_HANDLE;
                        goto ERR;
                    }
                }
                else
                {
                    hRet = NULL_HANDLE;
                    goto ERR;
                }
            }
            return hRet;
        ERR:
            _DestroySampler( &pSampler );
            return hRet;
        }

        SamplerRefPtr CTextureManager::GetSampler( const SamplerHandle& hSampler )
        {
            SamplerRefPtr pRet;
            CSampler* pSampler;
            m_Samplers.Find( hSampler.handle, &pSampler );
            {
                pRet = SamplerRefPtr{ pSampler };
            }
            return pRet;
        }

        void CTextureManager::DestroySampler( SamplerHandle* phSampler )
        {
            CSampler* pSampler;
            auto Itr = m_Samplers.Find( (*phSampler).handle, &pSampler );
            _DestroySampler( &pSampler );
            m_Samplers.Remove( Itr );
            *phSampler = NULL_HANDLE;
        }

        void CTextureManager::_DestroySampler( CSampler** ppInOut )
        {
            VKE_ASSERT( ppInOut != nullptr && *ppInOut != nullptr, "" );
            CSampler* pSampler = *ppInOut;
            m_pCtx->DDI().DestroyObject( &pSampler->m_hDDIObject, nullptr );
            pSampler->_Destroy();
            Memory::DestroyObject( &m_SamplerMemMgr, &pSampler );
            *ppInOut = nullptr;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER