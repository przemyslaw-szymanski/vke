#include "RenderSystem/Managers/CFramebufferManager.h"
#include "Core/Memory/Memory.h"
#include "RenderSystem/Resources/CFramebuffer.h"
#include "RenderSystem/CRenderSystem.h"

namespace VKE
{
    namespace RenderSystem
    {
        CFramebufferManager::CFramebufferManager(CRenderSystem* pRS) :
            m_pRenderSystem(pRS)
        {}

        Resources::CManager::ResourceRawPtr CFramebufferManager::_AllocateMemory(
            const Resources::SCreateDesc* const pInfo)
        {
            ResourceRawPtr pPtr = nullptr;
            CGraphicsContext* pCtx = m_pRenderSystem->GetCurrentContext(ContextScopes::FRAMEBUFFER);
            if (VKE_SUCCEEDED(Memory::CreateObject(&HeapAllocator, &pPtr, pCtx, this)))
            {
                if (pInfo->pTypeDesc)
                {
                    SFramebufferDesc* pDesc = reinterpret_cast<SFramebufferDesc*>(pInfo->pTypeDesc);
                    {
                        pPtr->m_hResource = m_pRenderSystem->CreateFramebuffer(*pDesc);
                    }
                }
            }
            return reinterpret_cast< Resources::CManager::ResourceRawPtr >(pPtr);
        }
    } // RenderSystem
} // VKE

