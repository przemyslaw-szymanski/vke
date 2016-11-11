#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CDevice.h"
#include "CVkEngine.h"
#include "CSwapChain.h"
#include "RenderSystem/CDeviceContext.h"
#include "PrivateDescs.h"
#include "Vulkan.h"
#include "Core/Utils/CLogger.h"

#include "Core/Memory/Memory.h"

#include "Core/Platform/CWindow.h"

#include "RenderSystem/Vulkan/CGraphicsQueue.h"

#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct CGraphicsContext::SPrivate
        {
            SGraphicsContextPrivateDesc PrivateDesc;
            CSwapChain*				    pSwapChain = nullptr;                       
            bool					    needRenderFrame = false;
        };

        CGraphicsContext::CGraphicsContext(CDeviceContext* pCtx) :
            m_pDeviceCtx(pCtx)
            //, m_pDeviceCtxCtx(pDevice->_GetDeviceContext())
        {

        }

        CGraphicsContext::~CGraphicsContext()
        {
            Destroy();
        }

        void CGraphicsContext::Destroy()
        {
            Memory::DestroyObject(&HeapAllocator, &m_pPrivate->pSwapChain);
            Memory::DestroyObject(&HeapAllocator, &m_pPrivate);
        }

        Result CGraphicsContext::Create(const SGraphicsContextDesc& Desc)
        {
            auto pPrivate = reinterpret_cast<SGraphicsContextPrivateDesc*>(Desc.pPrivate);
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pPrivate));
            m_pPrivate->PrivateDesc = *pPrivate;
            
            const auto& SwapChains = Desc.SwapChains;
            
            if (SwapChains.count)
            {
                for (uint32_t i = 0; i < SwapChains.count; ++i)
                {
                    SSwapChainDesc Desc = SwapChains[i];
                    Desc.pPrivate = &m_pPrivate->PrivateDesc;
                    //VKE_RETURN_IF_FAILED(CreateSwapChain(Desc));
                }
            }
            // Create dummy queue
            CreateGraphicsQueue({});
            return VKE_OK;
        }

        Vulkan::ICD::Device& CGraphicsContext::_GetICD() const
        {
            return *m_pPrivate->PrivateDesc.pICD;
        }

        Result CGraphicsContext::CreateSwapChain(const SSwapChainDesc& Info)
        {
            CSwapChain* pSwapChain;
            if (VKE_FAILED(Memory::CreateObject(&HeapAllocator, &pSwapChain, this)))
            {
                VKE_LOG_ERR("No memory to create swap chain object");
                return VKE_ENOMEMORY;
            }
            Result err;
            if (VKE_FAILED((err = pSwapChain->Create(Info))))
            {
                Memory::DestroyObject(&HeapAllocator, &pSwapChain);
                return err;
            }

            m_pPrivate->pSwapChain = pSwapChain;
            auto pWnd = m_pDeviceCtx->GetRenderSystem()->GetEngine()->FindWindow(Info.hWnd);
            //pWnd->SetRenderingContext(this);
            pWnd->AddUpdateCallback([&](CWindow* pWnd)
            {
                (void)pWnd;
                if( this->m_pPrivate->needRenderFrame )
                {
                    if( this->_BeginFrame() )
                    {
                        this->_EndFrame();
                    }
                    this->m_pPrivate->needRenderFrame = false;
                }
            });
            return VKE_OK;
        }

        handle_t CGraphicsContext::CreateGraphicsQueue(const SGraphicsQueueInfo& Info)
        {

            return NULL_HANDLE;
        }

        void CGraphicsContext::RenderFrame()
        {
            m_pPrivate->needRenderFrame = true;
        }

        bool CGraphicsContext::_BeginFrame()
        {
            m_pPrivate->pSwapChain->BeginPresent();
            return true;
        }

        void CGraphicsContext::_EndFrame()
        {
            
            m_pPrivate->pSwapChain->EndPresent();
        }

        void CGraphicsContext::Resize(uint32_t width, uint32_t height)
        {
            (void)width;
            (void)height;
        }

    } // RenderSystem
} // VKE