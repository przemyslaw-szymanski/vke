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
                m_CurrBackBufferPtrs.pRenderingPipeline = m_BackBuffers.aRenderingPipelines[ m_currBackBufferIndex ][ hPipeline.handle ];
            }

            uint32_t CBackBufferManager::AddCustomData(void** apData)
            {
                uint32_t idx;
                auto& Data = m_BackBuffers.aCustomData;

                for( uint32_t i = 0; i < m_backBufferCount; ++i )
                {
                    idx = Data[ i ].PushBack( apData[ i ] );
                }
                return idx;
            }

            void CBackBufferManager::UpdateCustomData(uint32_t idx, void** ppData)
            {
                auto& Data = m_BackBuffers.aCustomData;
                for( uint32_t i = 0; i < m_backBufferCount; ++i )
                {
                    Data[ i ][ idx ] = ppData[ i ];
                }
            }

            void* CBackBufferManager::GetCustomData(uint32_t idx) const
            {
                return m_BackBuffers.aCustomData[ m_currBackBufferIndex ][ idx ];
            }

        } // Managers
    } // RenderSystem
} // VKE