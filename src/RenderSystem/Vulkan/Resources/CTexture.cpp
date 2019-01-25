#include "RenderSystem/Resources/CTexture.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CCommandBuffer.h"
namespace VKE
{
    namespace RenderSystem
    {
        CTexture::CTexture(CTextureManager* pMgr) :
            m_pMgr{ pMgr }
        {}

        CTexture::~CTexture()
        {}

        void CTexture::Init(const STextureDesc& Desc)
        {
            m_Desc = Desc;
            m_vViews.Clear();
            this->m_hObject = CalcHash( Desc );
        }

        void CTexture::ChangeLayout(CommandBufferPtr pCommandBuffer, TEXTURE_LAYOUT /*newLayout*/)
        {
            assert( pCommandBuffer.IsValid() );

        }

        hash_t CTexture::CalcHash( const STextureDesc& Desc )
        {
            SHash Hash;
            Hash.Combine( Desc.format, Desc.memoryUsage, Desc.mipLevelCount, Desc.multisampling,
                Desc.Size.width, Desc.Size.height, Desc.type, Desc.usage );
            return Hash.value;
        }

        CTextureView::CTextureView()
        {}

        CTextureView::~CTextureView()
        {}

        void CTextureView::Init( const STextureViewDesc& Desc, TexturePtr pTexture )
        {
            m_Desc = Desc;
            m_pTexture = pTexture;
        }

        hash_t CTextureView::CalcHash( const STextureViewDesc& Desc )
        {
            SHash Hash;
            Hash.Combine( Desc.aspect, Desc.beginMipmapLevel, Desc.endMipmapLevel, Desc.format, Desc.hTexture.handle,
                Desc.type );
            return Hash.value;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER