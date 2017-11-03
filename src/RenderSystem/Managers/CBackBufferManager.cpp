#include "RenderSystem/Managers/CBackBufferManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        namespace Managers
        {
            CBackBufferManager::CBackBufferManager( CGraphicsContext* pCtx ) :
                m_pCtx( pCtx )
            {

            }

            CBackBufferManager::~CBackBufferManager()
            {
                _Destroy();
            }

            void CBackBufferManager::_Destroy()
            {

            }

            void CBackBufferManager::Destroy()
            {

            }

            Result CBackBufferManager::Create(const SBackBufferManagerDesc& Desc)
            {
                Result res = VKE_OK;
                m_Desc = Desc;
                m_backBufferCount = m_Desc.backBufferCount;
                m_BackBuffers.RenderingPipelines.vData.Resize( m_Desc.backBufferCount );
                return res;
            }

            uint32_t CBackBufferManager::AcquireNextBuffer()
            {
                uint16_t currIdx = m_currBackBufferIndex;
                m_currBackBufferIndex = currIdx % m_backBufferCount;
                return m_currBackBufferIndex;
            }

            void CBackBufferManager::SetRenderingPipeline(const RenderingPipelineHandle& hPipeline)
            {
                m_CurrBackBufferHandles.hRenderingPipeline = hPipeline;
                m_CurrBackBufferPtrs.pRenderingPipeline = m_BackBuffers.RenderingPipelines.vData[ m_currBackBufferIndex ][ hPipeline.handle ];
            }

        } // Managers
    } // RenderSystem
} // VKE