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
            
            if( m_Textures.TryToReuse( static_cast<handle_t>( hash ), &pTex ) )
            {

            }
            else if( m_Textures.GetFree( &pTex ) )
            {
                
            }
            else
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_TexMemMgr, &pTex, this ) ) )
                {
                    pTex->Init( Desc );
                    {
                        m_Textures.Add( pTex->GetHandle(), TextureRefPtr( pTex ) );
                    }
                }
            }
            if( pTex->GetDDIObject() == DDI_NULL_HANDLE )
            {
                pTex->m_hDDIObject = m_pCtx->_GetDDI().CreateObject( Desc );
            }
            return TextureHandle{ pTex->GetHandle() };
        }

        void CTextureManager::FreeTexture( TexturePtr* pInOut )
        {

        }

        void CTextureManager::DestroyTexture( TexturePtr* pInOut )
        {


        }

        TextureRefPtr CTextureManager::GetTexture( TextureHandle hTexture )
        {
            TextureRefPtr pTex;
            m_Textures.Find( hTexture.handle, &pTex );
            return pTex;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER