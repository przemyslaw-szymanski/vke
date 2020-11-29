#include "Core/Resources/CImage.h"
#include "Core/Managers/CImageManager.h"

namespace VKE
{
    namespace Core
    {
        CImage::CImage(CImageManager* pMgr) :
            m_pMgr{ pMgr }
        {

        }

        CImage::~CImage()
        {

        }

        Result CImage::_Init(const SImageDesc&  Desc)
        {
            Result ret = VKE_OK;
            m_Desc = Desc;

            return ret;
        }

        void CImage::_Destroy()
        {
#if VKE_USE_DEVIL
            ilDeleteImage((ILuint)m_hNative);
#elif VKE_USE_DIRECTXTEX
            m_DXImage.Release();
#endif
        }

        void CImage::Release()
        {
            m_pMgr->_FreeImage( this );
        }

        const uint8_t* CImage::GetData() const
        {
#if VKE_USE_DIRECTXTEX
            return m_DXImage.GetPixels();
#endif
        }

        uint32_t CImage::GetDataSize() const
        {
#if VKE_USE_DIRECTXTEX
            return (uint32_t)m_DXImage.GetPixelsSize();
#endif
        }

        void CImage::GetTextureDesc(RenderSystem::STextureDesc* pOut) const
        {
            m_pMgr->_GetTextureDesc(this, pOut);
        }
    }
}