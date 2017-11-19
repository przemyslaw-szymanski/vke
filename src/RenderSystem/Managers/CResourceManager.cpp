#include "RenderSystem/Managers/CResourceManager.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        namespace Managers
        {
            CResourceManager::CResourceManager(CDeviceContext* pCtx) :
                m_pCtx{ pCtx }
            {}

            CResourceManager::~CResourceManager()
            {}

            void CResourceManager::Destroy()
            {

            }

            Result CResourceManager::Create(const SResourceManagerDesc& Desc)
            {
                Result res = VKE_FAIL;
                
                return res;
            }

            TexturePtr CResourceManager::CreateTexture(const STextureDesc& Desc)
            {
                TexturePtr pTexture;
                TextureHandle hTex = m_pCtx->CreateTexture( Desc );
                if( hTex != NULL_HANDLE )
                {
                    STextureInitDesc InitDesc;
                    InitDesc.Desc       = Desc;
                    InitDesc.hTexture   = hTex;
                    InitDesc.pContext   = m_pCtx;
                    InitDesc.hNative    = m_pCtx->GetResourceManager().GetTexture( hTex );
                    CTexture* pTex = nullptr;
                    if( !m_Textures.vFreeElements.PopBack( &pTex ) )
                    {
                        if( VKE_FAILED( Memory::CreateObject( m_pTextureAllocator, &pTex ) ) )
                        {
                            VKE_LOG_ERR( "Unable to create memory for CTexture object." );
                            m_pCtx->GetResourceManager().DestroyTexture( hTex );
                        }
                    }
                    if( pTex )
                    {
                        {
                            pTex->Init( InitDesc );
                            pTexture = TexturePtr( pTex );
                            if( VKE_SUCCEEDED( CreateTextureView( &pTexture ) ) )
                            {

                            }
                            else
                            {
                                DestroyTexture( &pTexture );
                            }
                        }
                    }
                }
                return pTexture;
            }

            void CResourceManager::FreeTexture(TexturePtr* ppTex)
            {
                assert( ppTex && ppTex->IsValid() );
                CTexture* pTex = ppTex->Release();
                auto& vViews = pTex->GetDesc().vTextureViews;
                for( uint32_t i = 0; i < vViews.GetCount(); ++i )
                {
                    m_pCtx->GetResourceManager().DestroyTextureView( vViews[ i ].GetDesc().hView );
                }
                vViews.Clear();
                m_pCtx->GetResourceManager().DestroyTexture( pTex->GetDesc().hTexture );
                m_Textures.vFreeElements.PushBack( pTex );
            }

            void CResourceManager::DestroyTexture(TexturePtr* ppTex)
            {
                assert( ppTex && ppTex->IsValid() );
                uint32_t idx = m_Textures.vPool.Find( *ppTex );
                assert( idx != Utils::INVALID_POSITION );
                CTexture* pTex = ppTex->Release();
                auto& vViews = pTex->GetDesc().vTextureViews;
                for( uint32_t i = 0; i < vViews.GetCount(); ++i )
                {
                    m_pCtx->GetResourceManager().DestroyTextureView( vViews[ i ].GetDesc().hView );
                }
                vViews.Clear();
                m_pCtx->GetResourceManager().DestroyTexture( pTex->GetDesc().hTexture );
                m_Textures.vPool.RemoveFast( idx );
                Memory::DestroyObject( m_pTextureAllocator, &pTex );
            }

            void CResourceManager::DestroyTextures()
            {

            }

            Result CResourceManager::CreateTextureView(const STextureViewDesc& Desc, TexturePtr* ppTexInOut)
            {
                TextureViewHandle hView = m_pCtx->CreateTextureView( Desc );
                TextureViewPtr pView;
                Result res = VKE_FAIL;
                if( hView != NULL_HANDLE )
                {
                    STextureViewInitDesc InitDesc;
                    InitDesc.Desc           = Desc;
                    InitDesc.hView          = hView;
                    InitDesc.hNative        = m_pCtx->GetResourceManager().GetTextureView( hView );
                    InitDesc.pTexture       = *ppTexInOut;
                    CTextureView TexView;
                    TexView.Init( InitDesc );
                    STextureInitDesc& TexDesc   = (*ppTexInOut)->GetDesc();
                    uint32_t idx                = TexDesc.vTextureViews.PushBack( TexView );
                    if( idx != Utils::INVALID_POSITION )
                    {
                        res = VKE_OK;
                    }
                }
                return res;
            }

            TEXTURE_ASPECT FormatToAspect(TEXTURE_FORMAT format)
            {
                TEXTURE_ASPECT aspect;
                switch( format )
                {
                    case TextureFormats::D16_UNORM:
                    case TextureFormats::D32_SFLOAT:
                        aspect = TextureAspects::DEPTH;
                    break;
                    case TextureFormats::D32_SFLOAT_S8_UINT:
                    case TextureFormats::D16_UNORM_S8_UINT:
                    case TextureFormats::D24_UNORM_S8_UINT:
                    case TextureFormats::X8_D24_UNORM_PACK32:
                        aspect = TextureAspects::DEPTH_STENCIL;
                    break;
                    default:
                        aspect = TextureAspects::COLOR;
                    break;
                }
                return aspect;
            }

            Result CResourceManager::CreateTextureView(TexturePtr* ppTexInOut)
            {
                const STextureInitDesc& TexDesc = (*ppTexInOut)->GetDesc();
                STextureViewDesc Desc;
                Desc.hTexture           = TexDesc.hTexture;
                Desc.beginMipmapLevel   = 0;
                Desc.endMipmapLevel     = TexDesc.Desc.mipLevelCount;
                Desc.format             = TexDesc.Desc.format;
                Desc.type               = static_cast< TEXTURE_VIEW_TYPE >( TexDesc.Desc.type );
                Desc.aspect             = FormatToAspect( TexDesc.Desc.format );
                return CreateTextureView( Desc, ppTexInOut );
            }
        }
    }
}