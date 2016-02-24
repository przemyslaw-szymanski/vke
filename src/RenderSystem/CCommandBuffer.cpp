#include "RenderSystem/CCommandBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        CCommandBuffer::CCommandBuffer(CDevice* pDevice, Resource::CManager* pMgr) :
            CResource(pMgr)
            , m_pDevice(pDevice)
        {

        }

        CCommandBuffer::~CCommandBuffer()
        {

        }

        void CCommandBuffer::Begin()
        {
   
        }

        void CCommandBuffer::End()
        {

        }
    } // rendersystem
} // vke