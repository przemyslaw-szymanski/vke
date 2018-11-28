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
            this->m_hObjHandle = CalcHash( Desc );
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

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER