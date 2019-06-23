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

        Core::CManager::ResourceRawPtr CFramebufferManager::_AllocateMemory(
            const Core::SCreateDesc* const /*pInfo*/)
        {
            ResourceRawPtr pPtr = nullptr;
            CGraphicsContext* pCtx = m_pRenderSystem->GetCurrentContext(ContextScopes::FRAMEBUFFER);
            if (VKE_SUCCEEDED(Memory::CreateObject(&HeapAllocator, &pPtr, pCtx, this)))
            {
                
            }
            return reinterpret_cast< Core::CManager::ResourceRawPtr >(pPtr);
        }
    } // RenderSystem
} // VKE

