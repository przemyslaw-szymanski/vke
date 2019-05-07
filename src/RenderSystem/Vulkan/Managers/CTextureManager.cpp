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

            VKE_ASSERT( 0, "implement" );
            
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

            return hTex;
        }

        TextureViewHandle CTextureManager::CreateTextureView( const STextureViewDesc& Desc )
        {
            //hash_t hash = CTextureView::CalcHash( Desc );
            CTextureView* pView = nullptr;
            TextureViewHandle hView;

            if( VKE_SUCCEEDED( Memory::CreateObject( &m_TexViewMemMgr, &pView ) ) )
            {
                hView.handle = m_TextureViews.Add( TextureViewRefPtr( pView ) );
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

            return TextureViewHandle{ hView };
        }

        //void CTextureManager::FreeTexture( TextureHandle* phTexture )
        //{
        //    TextureHandle& hTex = *phTexture;
        //    TextureRefPtr pTex;
        //    if( m_Textures.Find( hTex.handle, &pTex ) )
        //    {
        //        m_pCtx->_GetDDI().DestroyObject( &pTex->m_hDDIObject, nullptr );
        //        m_Textures.AddFree( hTex.handle, pTex.Get() );                
        //    }
        //    hTex = NULL_HANDLE;
        //}

        void CTextureManager::DestroyTexture( TextureHandle* phTexture )
        {
            TextureHandle& hTex = *phTexture;
            TextureRefPtr pTex;
            m_Textures.Free( hTex.handle );
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

        /*void CTextureManager::FreeTextureView( TextureViewHandle* phView )
        {
            TextureViewHandle& hView = *phView;
            TextureViewRefPtr pView;
            if( m_TextureViews.Find( hView.handle, &pView ) )
            {
                m_pCtx->_GetDDI().DestroyObject( &pView->m_hDDIObject, nullptr );
                m_TextureViews.AddFree( hView.handle, pView.Get() );
            }
            hView = NULL_HANDLE;
        }*/

        void CTextureManager::DestroyTextureView( TextureViewHandle* phView )
        {
            TextureViewHandle& hView = *phView;
            TextureViewRefPtr pView;
            m_TextureViews.Free( hView.handle );
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
            return m_Textures[hTexture.handle];
        }

        TextureViewRefPtr CTextureManager::GetTextureView( TextureViewHandle hView )
        {
            return m_TextureViews[hView.handle];
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER