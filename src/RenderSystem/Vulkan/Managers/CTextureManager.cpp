#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Managers/CTextureManager.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
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

        }

        Result CTextureManager::Create( const STextureManagerDesc& Desc )
        {
            Result ret = VKE_OK;

            return ret;
        }

        TextureHandle CTextureManager::CreateTexture( const STextureDesc& Desc )
        {
            hash_t hash = CTexture::CalcHash( Desc );
            CTexture* pTex = nullptr;
            TextureHandle hTex = TextureHandle{ static_cast<handle_t>(hash) };

            if( m_Textures.TryToReuse( hTex.handle, &pTex ) )
            {

            }
            else if( m_Textures.GetFree( &pTex ) )
            {
                
            }
            else
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_TexMemMgr, &pTex, this ) ) )
                {
                    m_Textures.Add( hTex.handle, TextureRefPtr( pTex ) );
                }
            }
            if( pTex != nullptr )
            {
                pTex->Init( Desc );
                {
                    pTex->m_hObject = hTex.handle;
                    if( pTex->GetDDIObject() == DDI_NULL_HANDLE )
                    {
                        pTex->m_hDDIObject = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
                    }
                }
            }
            if( pTex == nullptr || pTex->GetDDIObject() == DDI_NULL_HANDLE )
            {
                FreeTexture( &hTex );
            }
            return hTex;
        }

        TextureViewHandle CTextureManager::CreateTextureView( const STextureViewDesc& Desc )
        {
            hash_t hash = CTextureView::CalcHash( Desc );
            CTextureView* pView = nullptr;
            TextureViewHandle hView = TextureViewHandle{ static_cast<handle_t>(hash) };

            if( !m_TextureViews.TryToReuse( hView.handle, &pView ) )
            {
                if( !m_TextureViews.GetFree( &pView ) )
                {
                    if( VKE_SUCCEEDED( Memory::CreateObject( &m_TexViewMemMgr, &pView ) ) )
                    {
                        m_TextureViews.Add( hView.handle, TextureViewRefPtr( pView ) );
                    }
                }
            }
            if( pView != nullptr )
            {
                TexturePtr pTex = GetTexture( Desc.hTexture );
                pView->Init( Desc, pTex );
                {
                    pView->m_hObject = hView.handle;
                    if( pView->m_hDDIObject == DDI_NULL_HANDLE )
                    {
                        pView->m_hDDIObject = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
                    }
                }
            }
            if( pView == nullptr || pView->GetDDIObject() == DDI_NULL_HANDLE )
            {
                FreeTextureView( &hView );
            }

            return TextureViewHandle{ hView };
        }

        void CTextureManager::FreeTexture( TextureHandle* phTexture )
        {
            TextureHandle& hTex = *phTexture;
            TextureRefPtr pTex;
            if( m_Textures.Find( hTex.handle, &pTex ) )
            {
                m_pCtx->_GetDDI().DestroyObject( &pTex->m_hDDIObject, nullptr );
                m_Textures.AddFree( hTex.handle, pTex.Get() );                
            }
            hTex = NULL_HANDLE;
        }

        void CTextureManager::DestroyTexture( TextureHandle* phTexture )
        {
            TextureHandle& hTex = *phTexture;
            TextureRefPtr pTex;
            if( m_Textures.Remove( hTex.handle, &pTex ) )
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

        void CTextureManager::FreeTextureView( TextureViewHandle* phView )
        {
            TextureViewHandle& hView = *phView;
            TextureViewRefPtr pView;
            if( m_TextureViews.Find( hView.handle, &pView ) )
            {
                m_pCtx->_GetDDI().DestroyObject( &pView->m_hDDIObject, nullptr );
                m_TextureViews.AddFree( hView.handle, pView.Get() );
            }
            hView = NULL_HANDLE;
        }

        void CTextureManager::DestroyTextureView( TextureViewHandle* phView )
        {
            TextureViewHandle& hView = *phView;
            TextureViewRefPtr pView;
            if( m_TextureViews.Remove( hView.handle, &pView ) )
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

        TextureRefPtr CTextureManager::GetTexture( TextureHandle hTexture )
        {
            TextureRefPtr pTex;
            m_Textures.Find( hTexture.handle, &pTex );
            return pTex;
        }

        TextureViewRefPtr CTextureManager::GetTextureView( TextureViewHandle hTexture )
        {
            TextureViewRefPtr pView;
            m_TextureViews.Find( hTexture.handle, &pView );
            return pView;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER