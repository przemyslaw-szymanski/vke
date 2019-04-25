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

        Result CStagingBufferManager::GetBuffer( const SBufferRequirementInfo& Info, SBufferData* pData )
        {
            Result ret = VKE_ENOTFOUND;
            SBindMemoryInfo BindInfo;
            auto& DeviceMemMgr = Info.pCtx->_GetDeviceMemoryManager();
            const uint32_t bufferSize = std::max( m_Desc.bufferSize, Info.Requirements.size );

            SAllocateMemoryInfo AllocInfo;
            CMemoryPoolView::SAllocateData AllocData;
            AllocInfo.alignment = Info.Requirements.alignment;
            AllocInfo.size = Info.Requirements.size;

            for( uint32_t i = 0; i < m_vMemViews.GetCount(); ++i )
            {
                CMemoryPoolView& View = m_vMemViews[ i ];
                uint64_t memory = View.Allocate( AllocInfo, &AllocData );
                if( memory != 0 )
                {
                    pData->pBuffer = m_vpBuffers[ i ];
                    pData->offset = AllocData.offset;
                    pData->size = AllocInfo.size;
                    pData->handle = i;
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
                    ViewInfo.memory = reinterpret_cast< uint64_t >( pBuffer->m_BindInfo.hDDIMemory );
                    ViewInfo.size = bufferSize;
                    ViewInfo.offset = 0;
                    CMemoryPoolView View;
                    View.Init( ViewInfo );
                    m_vMemViews.PushBack( View );
                    m_vpBuffers.PushBack( pBuffer );
                    return GetBuffer( Info, pData );
                }
            }
            return ret;
        }

        void CStagingBufferManager::FreeBuffer( const SBufferData& Data )
        {
            auto& View = m_vMemViews[ Data.handle ];
            //View.Free()
            assert( 0 );
        }

        uint32_t CStagingBufferManager::_FindBuffer( const SBufferRequirementInfo& Info )
        {
            uint32_t idx = UNDEFINED_U32;
            
            return idx;
        }
    } // RenderSystem
} // VKE