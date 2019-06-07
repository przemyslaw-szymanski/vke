#include "CStagingBufferManager.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        Result CStagingBufferManager::Create( const SStagingBufferManagerDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            return ret;
        }

        void CStagingBufferManager::Destroy(CDeviceContext* pCtx)
        {
            for( uint32_t i = 0; i < m_vpBuffers.GetCount(); ++i )
            {
                //pCtx->DestroyBuffer( &m_vpBuffers[i] );
                m_vpBuffers[i] = nullptr;
            }
            m_vpBuffers.Clear();
            m_vMemViews.Clear();
        }

        Result CStagingBufferManager::GetBuffer( const SBufferRequirementInfo& Info, SBufferData** ppData )
        {
            Result ret = VKE_ENOTFOUND;
            SBindMemoryInfo BindInfo;
            auto& DeviceMemMgr = Info.pCtx->_GetDeviceMemoryManager();
            const uint32_t bufferSize = std::max( m_Desc.bufferSize, Info.Requirements.size );

            SAllocateMemoryInfo AllocInfo;
            CMemoryPoolView::SAllocateData AllocData;
            AllocInfo.alignment = Info.Requirements.alignment;
            AllocInfo.size = Info.Requirements.size;

            Threads::ScopedLock l( m_MemViewSyncObj );
            for( uint32_t i = 0; i < m_vMemViews.GetCount(); ++i )
            {
                CMemoryPoolView& View = m_vMemViews[ i ];
                uint64_t memory = View.Allocate( AllocInfo, &AllocData );
                if( memory != 0 )
                {
                    SBufferData Data;
                    Data.pBuffer = m_vpBuffers[i];
                    Data.offset = AllocData.offset;
                    Data.size = AllocInfo.size;
                    Data.handle = i;
                    const auto dataIdx = m_vUsedData.PushBack( Data );
                    *ppData = &m_vUsedData[dataIdx];
                    ret = VKE_OK;
                    break;
                }
            }
            if( ret == VKE_ENOTFOUND )
            {
                SCreateBufferDesc BufferDesc;
                BufferDesc.Create.async = false;
                BufferDesc.Create.stages = Resources::StageBits::FULL_LOAD;
                BufferDesc.Buffer.memoryUsage = MemoryUsages::STAGING;
                BufferDesc.Buffer.size = bufferSize;
                BufferDesc.Buffer.usage = BufferUsages::TRANSFER_SRC;
                BufferRefPtr pBuffer = Info.pCtx->CreateBuffer( BufferDesc );
                if( pBuffer.IsValid() )
                {
                    CMemoryPoolView::SInitInfo ViewInfo;
                    ViewInfo.allocationAlignment = 0;
                    ViewInfo.memory = ( pBuffer->m_hMemory );
                    ViewInfo.size = bufferSize;
                    ViewInfo.offset = 0;
                    CMemoryPoolView View;
                    View.Init( ViewInfo );
                    m_vMemViews.PushBack( View );
                    m_vpBuffers.PushBack( pBuffer );
                    return GetBuffer( Info, ppData );
                }
            }
            return ret;
        }

        void CStagingBufferManager::FreeBuffer( SBufferData** ppData )
        {
            auto pData = *ppData;
            auto& View = m_vMemViews[ pData->handle ];
            CMemoryPoolView::SAllocateData AllocData;
            AllocData.memory = pData->pBuffer->m_hMemory;
            AllocData.offset = pData->offset;
            AllocData.size = pData->size;
            View.Free( AllocData );
        }

        uint32_t CStagingBufferManager::_FindBuffer( const SBufferRequirementInfo& Info )
        {
            uint32_t idx = UNDEFINED_U32;
            
            return idx;
        }

        void CStagingBufferManager::FreeUnusedAllocations( CDeviceContext* pCtx )
        {
            uint32_t count = m_vUsedData.GetCount();
            uint32_t currEl = 0;
            for( uint32_t i = 0; i < count; ++i )
            {
                auto& Curr = m_vUsedData[ currEl ];
                if( Curr.pCommandBuffer.IsValid() && Curr.pCommandBuffer->IsExecuted() )
                {
                    auto pData = &Curr;
                    FreeBuffer( &pData );
                    m_vUsedData.RemoveFast( currEl );
                }
                else
                {
                    currEl++;
                }
            }
        }

    } // RenderSystem
} // VKE