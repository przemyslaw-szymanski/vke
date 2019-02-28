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
                m_backBufferCount = static_cast< uint32_t >( m_Desc.backBufferCount );
                return res;
            }

            uint32_t CBackBufferManager::AcquireNextBuffer()
            {
                const uint16_t currIdx = m_currBackBufferIndex;
                m_currBackBufferIndex = ( currIdx + 1 ) % m_backBufferCount;
                return m_currBackBufferIndex;
            }

            uint32_t CBackBufferManager::AddCustomData(uint8_t* aData, uint32_t elementSize)
            {
                uint32_t idx = m_customDataIdx++;
                //auto& Data = m_BackBuffers.aCustomData;
                uint8_t* pCurrPtr = aData;
                for( uint32_t i = 0; i < m_backBufferCount; ++i )
                {
                    //idx = Data[ i ].PushBack( pCurrPtr );
                    m_BackBuffers[ i ].CustomData.insert( { idx, pCurrPtr } );
                    pCurrPtr += elementSize;
                }
                return idx;
            }

            void CBackBufferManager::UpdateCustomData(uint32_t backBufferIdx, uint32_t dataIdx, void* pData)
            {
                m_BackBuffers[ backBufferIdx ].CustomData[ dataIdx ] = pData;
            }

            void CBackBufferManager::UpdateCustomData(uint32_t dataIdx, uint8_t* aData, uint32_t elementSize)
            {
                uint8_t* pCurrPtr = aData;
                for( uint32_t i = 0; i < m_backBufferCount; ++i )
                {
                    m_BackBuffers[ i ].CustomData[ dataIdx ] = pCurrPtr;
                    pCurrPtr += elementSize;
                }
            }

            void* CBackBufferManager::GetCustomData(uint32_t idx)
            {
                return m_BackBuffers[ m_currBackBufferIndex ].CustomData[ idx ];
            }

        } // Managers
    } // RenderSystem
} // VKE