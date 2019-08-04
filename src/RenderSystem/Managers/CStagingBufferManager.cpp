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

        void CStagingBufferManager::Destroy(CDeviceContext*)
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
            //auto& DeviceMemMgr = Info.pCtx->_GetDeviceMemoryManager();
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
                BufferDesc.Create.stages = Core::ResourceStages::FULL_LOAD;
                BufferDesc.Buffer.memoryUsage = MemoryUsages::STAGING;
                BufferDesc.Buffer.size = bufferSize;
                BufferDesc.Buffer.usage = BufferUsages::TRANSFER_SRC;
                BufferHandle hBuffer = Info.pCtx->CreateBuffer( BufferDesc );
                if( hBuffer != INVALID_HANDLE )
                {
                    auto pBuffer = Info.pCtx->GetBuffer( hBuffer );
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

        Result CStagingBufferManager::GetBuffer( const SBufferRequirementInfo& Info, handle_t* phInOut,
                                                 CStagingBufferManager::SBufferInfo* pOut )
        {
            VKE_ASSERT( phInOut != nullptr, "" );
            Result ret = VKE_OK;
            UStagingBufferHandle hStagingBuffer, hInOut;
            hInOut.handle = *phInOut;
            hStagingBuffer.handle = *phInOut;

            const auto alignedSize = Memory::CalcAlignedSize( Info.Requirements.size, (uint32_t)Info.Requirements.alignment );
            bool needNewBatch = hInOut.handle == UNDEFINED_U64;
            bool needNewChunk = true;
            // If a buffer chunk is already present
            if( !needNewBatch )
            {
                // Check if there is a free space left
                SBufferChunk& Chunk = m_vBufferChunks[ hInOut.index ];
                needNewChunk = Chunk.offset + alignedSize > hInOut.size;
            }
            if( needNewChunk )
            {
                hStagingBuffer = _FindFreeChunk( alignedSize );
                if( hStagingBuffer.handle == UNDEFINED_U64 )
                {
                    const auto bufferIdx = _CreateBuffer( Info );
                    if( bufferIdx != UNDEFINED_U32 )
                    {
                        hStagingBuffer = _FindFreeChunk( alignedSize );
                        VKE_ASSERT( hStagingBuffer.handle != UNDEFINED_U64, "" );
                    }
                    else
                    {
                        ret = VKE_FAIL;
                    }
                }
                
                if( needNewBatch )
                {
                    hStagingBuffer.batchIndex = m_vChunkBatches.PushBack( {} );
                    VKE_ASSERT( m_vChunkBatches.GetCount() < MAX_BATCH_COUNT, "" );
                }
                else
                {
                    hStagingBuffer.batchIndex = hInOut.batchIndex;
                }
                {
                    VKE_ASSERT( hStagingBuffer.batchIndex < m_vChunkBatches.GetCount(), "" );
                    m_vChunkBatches[ hStagingBuffer.batchIndex ].PushBack( hStagingBuffer );
                }
                *phInOut = hStagingBuffer.handle;
            }
            
            VKE_ASSERT( ret == VKE_OK, "" );
            const auto pBuffer = m_vpBuffers[ hStagingBuffer.bufferIndex ];
            auto& Chunk = m_vBufferChunks[ hStagingBuffer.index ];
            pOut->hMemory = pBuffer->m_hMemory;
            pOut->hDDIBuffer = pBuffer->GetDDIObject();
            pOut->offset = pBuffer->CalcOffset( Chunk.bufferRegion, 0 ) + Chunk.offset;
            pOut->size = alignedSize;
            Chunk.offset += alignedSize;

            return ret;
        }

        void CStagingBufferManager::GetBufferInfo( const handle_t& hStagingBuffer, SBufferInfo* pOut )
        {
            UStagingBufferHandle Handle;
            Handle.handle = hStagingBuffer;
            const auto pBuffer = m_vpBuffers[ Handle.bufferIndex ];
            const auto& Chunk = m_vBufferChunks[ Handle.index ];
            pOut->hMemory = pBuffer->m_hMemory;
            pOut->hDDIBuffer = pBuffer->GetDDIObject();
            pOut->offset = pBuffer->CalcOffset( Chunk.bufferRegion, 0 ) + Chunk.offset;
        }

        void CStagingBufferManager::FreeBuffer( const handle_t& hStagingBuffer )
        {
            if( hStagingBuffer != UNDEFINED_U64 )
            {
                UStagingBufferHandle Handle;
                Handle.handle = hStagingBuffer;
                auto& vBatch = m_vChunkBatches[ Handle.batchIndex ];
                {
                    Threads::SyncObject l( m_FreeChunkSyncObj );
                    for( uint32_t i = 0; i < vBatch.GetCount(); ++i )
                    {
                        UStagingBufferHandle hChunk = vBatch[ i ];
                        VKE_ASSERT( hChunk.batchIndex == Handle.batchIndex, "" );
                        hChunk.batchIndex = UNDEFINED_U8;
                        m_vhFreeChunks.PushBack( hChunk );
                        m_vBufferChunks[ hChunk.index ].offset = 0;
                    }
                }
                {
                    Threads::SyncObject l( m_BatchSyncObj );
                    vBatch.Clear();
                }
            }
        }

        uint8_t CStagingBufferManager::_CreateBuffer( const SBufferRequirementInfo& Info )
        {
            uint8_t ret = UNDEFINED_U8;
            const auto size = Memory::CalcAlignedSize( Info.Requirements.size, (uint32_t)Info.Requirements.alignment );
            const uint32_t chunkSize = std::max( size, Config::RenderSystem::Buffer::STAGING_BUFFER_CHUNK_SIZE );
            const uint32_t bufferSize = std::max( chunkSize * 4, Config::RenderSystem::Buffer::STAGING_BUFFER_SIZE );
            const uint32_t regionCount = bufferSize / chunkSize;

            VKE_ASSERT( chunkSize >= VKE_MEGABYTES( 1 ) && chunkSize < VKE_MEGABYTES( 255 ), "" );
            VKE_ASSERT( m_vpBuffers.GetCount() < MAX_BUFFER_COUNT, "" );

            if( m_vpBuffers.GetCount() + 1 < MAX_BUFFER_COUNT )
            {
                SCreateBufferDesc BufferDesc;
                BufferDesc.Create.async = false;
                BufferDesc.Create.stages = Core::ResourceStages::FULL_LOAD;
                BufferDesc.Buffer.memoryUsage = MemoryUsages::STAGING;
                BufferDesc.Buffer.size = 0; // Config::RenderSystem::Buffer::STAGING_BUFFER_SIZE;
                BufferDesc.Buffer.usage = BufferUsages::TRANSFER_SRC;
                BufferDesc.Buffer.vRegions =
                {
                    SBufferRegion( regionCount, chunkSize )
                };
                BufferHandle hBuffer = Info.pCtx->CreateBuffer( BufferDesc );
                auto pBuffer = Info.pCtx->GetBuffer( hBuffer );
                auto idx = m_vpBuffers.PushBack( pBuffer );
                VKE_ASSERT( idx < MAX_BUFFER_COUNT, "" );

                Threads::ScopedLock l( m_FreeChunkSyncObj );
                for( uint32_t i = 0; i < regionCount; ++i )
                {
                    SBufferChunk Chunk;
                    
                    UStagingBufferHandle hStagingBuffer;
                    hStagingBuffer.index = m_vBufferChunks.PushBack( Chunk );
                    hStagingBuffer.bufferIndex = idx;
                    hStagingBuffer.size = chunkSize;
                    hStagingBuffer.batchIndex = UNDEFINED_U8;
                    m_vhFreeChunks.PushBack( hStagingBuffer );
                    VKE_ASSERT( m_vBufferChunks.GetCount() < MAX_CHUNK_COUNT, "" );
                }
                ret = (uint8_t)idx;
            }
            return ret;
        }

        UStagingBufferHandle CStagingBufferManager::_FindFreeChunk( const uint32_t size )
        {
            UStagingBufferHandle hRet;
            // Find minimum requirements
            uint32_t minSize = UNDEFINED_U32;

            for( uint32_t i = 0; i < m_vhFreeChunks.GetCount(); ++i )
            {
                const uint32_t currSize = m_vhFreeChunks[ i ].size;
                if( currSize >= size && minSize > currSize )
                {
                    hRet = m_vhFreeChunks[ i ];
                    minSize = currSize;
                }
            }
            return hRet;
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

        void CStagingBufferManager::FreeUnusedAllocations( CDeviceContext* )
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

            DefragmentMemory();
        }

        void CStagingBufferManager::DefragmentMemory()
        {
            for( uint32_t i = 0; i < m_vMemViews.GetCount(); ++i )
            {
                if( m_vMemViews[ i ].CanDefragment() )
                {
                    m_vMemViews[ i ].Defragment();
                }
            }
        }

    } // RenderSystem
} // VKE