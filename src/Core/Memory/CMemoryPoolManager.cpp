#include "Core/Memory/CMemoryPoolManager.h"
#include "Core/Utils/CTimer.h"

namespace VKE
{
    //namespace Memory
    //{
    //    CMemoryPoolManager::CMemoryPoolManager()
    //    {

    //    }

    //    CMemoryPoolManager::~CMemoryPoolManager()
    //    {
    //        Destroy();
    //    }

    //    void CMemoryPoolManager::Destroy()
    //    {

    //    }

    //    Result CMemoryPoolManager::Create(const SMemoryPoolManagerDesc& Desc)
    //    {
    //        Result res = VKE_FAIL;
    //        if( m_vvMemoryBuffers.Resize( Desc.poolTypeCount ) )
    //        {
    //            for( uint32_t i = 0; i < Desc.poolTypeCount; ++i )
    //            {
    //                m_vvMemoryBuffers[ i ].Resize( Desc.indexTypeCount );
    //            }

    //            if( m_vTmpData.Resize( Desc.poolTypeCount ) )
    //            {
    //                m_mAllocatedData.reserve( Desc.defaultAllocationCount );
    //                m_vAllocatedData.Reserve( Desc.defaultAllocationCount );
    //                m_vCaches.Resize( Desc.poolTypeCount );
    //                res = VKE_OK;
    //            }
    //        }
    //        return res;
    //    }

    //    uint32_t CalcAlignedSize(uint32_t size, uint32_t alignment)
    //    {
    //        uint32_t ret = size;
    //        uint32_t remainder = size % alignment;
    //        if( remainder > 0 )
    //        {
    //            ret = size + alignment - remainder;
    //        }

    //        return ret;
    //    }

    //    uint32_t CMemoryPoolManager::_FindFreeMemory(const SFindInfo& Info)
    //    {
    //        SFreeMemoryData& FreeData = Info.pPool->FreeMemory;
    //        UintVec& vChunkSizes = FreeData.vChunkSizes;
    //        uint32_t ret = 0;

    //        if( Info.findMethod == FIRST_FREE )
    //        {
    //            for( uint32_t i = 0; i < vChunkSizes.GetCount(); ++i )
    //            {
    //                if( vChunkSizes[ i ] >= Info.size )
    //                {
    //                    ret = i;
    //                    break;
    //                }
    //            }
    //        }
    //        else if( Info.findMethod == BEST_FIT )
    //        {
    //            uint32_t minSize = UINT32_MAX;
    //            uint32_t minIdx = 0;

    //            for( uint32_t i = 0; i < vChunkSizes.GetCount(); ++i )
    //            {
    //                const uint32_t currSize = vChunkSizes[ i ];
    //                if( currSize >= Info.size && minSize > currSize )
    //                {
    //                    minSize = currSize;
    //                    minIdx = i;
    //                }
    //            }
    //            ret = minIdx;
    //        }

    //        return ret;
    //    }   

    //    uint64_t CMemoryPoolManager::Allocate(const SAllocateInfo& Info, SAllocatedData* pOut)
    //    {
    //        auto pPool = &m_mMemoryPools[ Info.hPool ];
    //        SFindInfo FindInfo;
    //        FindInfo.findMethod = BEST_FIT;
    //        FindInfo.pPool = pPool;
    //        FindInfo.size = CalcAlignedSize( Info.size, pPool->allocationAlignment );
    //        uint64_t ret = 0;
    //        const uint32_t chunkIdx = _FindFreeMemory( FindInfo );
    //        if( chunkIdx >= 0 )
    //        {
    //            auto& vChunks = pPool->FreeMemory.vChunks;
    //            auto& vChunkSizes = pPool->FreeMemory.vChunkSizes;

    //            SMemoryChunk& Chunk = vChunks[ chunkIdx ];
    //            const uint32_t offset = Chunk.offset;
    //            Chunk.size -= Info.size;
    //            Chunk.offset += Info.size;
    //            vChunkSizes[ chunkIdx ] = Chunk.size;
    //            ret = pPool->memory + offset;
    //        }

    //        return ret;
    //    }

    //    Result CMemoryPoolManager::_AddFreeChunk(const SMemoryChunk& Chunk, SFreeMemoryData* pOut)
    //    {
    //        Result res = VKE_ENOMEMORY;
    //        if( pOut->vChunks.PushBack( Chunk ) != Utils::INVALID_POSITION )
    //        {
    //            if( pOut->vChunkSizes.PushBack( Chunk.size ) != Utils::INVALID_POSITION )
    //            {
    //                res = VKE_OK;
    //            }
    //        }
    //        return res;
    //    }

    //    void CMemoryPoolManager::_MergeFreeChunks(SFreeMemoryData* pOut)
    //    {
    //        auto& vFreeChunks = pOut->vChunks;
    //        
    //    }

    //    Result CMemoryPoolManager::CreatePool(const SPoolAllocateInfo& Info)
    //    {
    //        Result ret = VKE_ENOMEMORY;
    //        
    //        return ret;
    //    }

    //    Result CMemoryPoolManager::Free(uint64_t memory)
    //    {
    //        Result res = VKE_ENOTFOUND;
    //        auto Itr = m_mAllocatedData.find(memory);
    //        if( Itr != m_mAllocatedData.end() )
    //        {
    //            SAllocatedData& Data = Itr->second;
    //            auto& Buffers = m_vvMemoryBuffers[ Data.type ][ Data.index ];
    //            res = _AddFreeChunk( Data.Memory, &Buffers.FreeMemory );
    //        }
    //        else
    //        {
    //            for( uint32_t i = 0; i < m_vAllocatedData.GetCount(); ++i )
    //            {
    //                SAllocatedData& Data = m_vAllocatedData[ i ];
    //                uint64_t memPtr = Data.Memory.memory + Data.Memory.offset;
    //                if( memPtr == memory )
    //                {
    //                    auto& Buffers = m_vvMemoryBuffers[ Data.type ][ Data.index ];
    //                    res = _AddFreeChunk( Data.Memory, &Buffers.FreeMemory );
    //                    break;
    //                }
    //            }
    //        }
    //        return res;
    //    }

    //} // Memory

    Result CMemoryPoolView::Init( const SInitInfo& Info )
    {
        Result ret = VKE_OK;
        m_InitInfo = Info;
        m_MainChunk.offset = Info.offset;
        m_MainChunk.size = Info.size;
        m_vFreeChunks.PushBack( {} );
        m_vFreeChunkOffsets.PushBack( 0 );
        m_vFreeChunkSizes.PushBack( 0 );
        return ret;
    }

    uint32_t CalcAlignedSize( uint32_t size, uint32_t alignment )
    {
        uint32_t ret = size;
        uint32_t remainder = size % alignment;
        if( remainder > 0 )
        {
            ret = size + alignment - remainder;
        }

        return ret;
    }

    uint64_t CMemoryPoolView::Allocate( const SAllocateMemoryInfo& Info, SAllocateData* pOut )
    {
        uint64_t ret = INVALID_ALLOCATION;
        uint32_t size = CalcAlignedSize( Info.size, Info.alignment );
        // If there is a space in main memory
        if( m_MainChunk.size >= size )
        {
            const uint32_t alignedOffset = CalcAlignedSize( m_MainChunk.offset, Info.alignment );

            ret = m_InitInfo.memory + alignedOffset;
            
            pOut->memory = m_InitInfo.memory;
            pOut->offset = alignedOffset;
            pOut->size = size;

            m_MainChunk.size -= size;
            m_MainChunk.offset += alignedOffset;
        }
        else
        {
            uint32_t idx = _FindFree( size );
            if( idx != UNDEFINED_U32 )
            {
                uint32_t offset = m_vFreeChunkOffsets[idx];
                pOut->memory = m_InitInfo.memory;
                pOut->offset = offset;
                pOut->size = size;
                ret = pOut->memory + offset;

                m_vFreeChunkOffsets.RemoveFast( idx );
                m_vFreeChunks.RemoveFast( idx );
                m_vFreeChunkSizes.RemoveFast( idx );
            }
            else
            {
                VKE_LOG_ERR( "No free memory left in CMemoryPoolView for requested size: " << size );
            }
        }
        return ret;
    }

    void CMemoryPoolView::Free( const SAllocateData& Data )
    {
        m_vFreeChunkSizes.PushBack( Data.size );
        m_vFreeChunkOffsets.PushBack( Data.offset );
        SChunk Chunk;
        Chunk.offset = Data.offset;
        Chunk.size = Data.size;
        m_vFreeChunks.PushBack( Chunk );
    }

    template<typename T, typename Callback>
    uint32_t FindMin( T* pArray, uint32_t count, const T& max, Callback&& Cb )
    {
        T min = max;
        uint32_t ret = 0;

        for( uint32_t i = 0; i < count; ++i )
        {
            if( Cb( pArray[ i ], min ) )
            {
                min = pArray[ i ];
                ret = i;
            }
        }
        return ret;
    }

    uint32_t CMemoryPoolView::_FindFree( uint32_t size )
    {
        static const bool FindFirstFree = false;
        uint32_t ret = UNDEFINED_U32;

        if( FindFirstFree )
        {
            for( uint32_t i = 0; i < m_vFreeChunkSizes.GetCount(); ++i )
            {
                if( m_vFreeChunkSizes[i] >= size )
                {
                    ret = i;
                    break;
                }
            }
        }
        else
        {
            uint32_t idx = FindMin( &m_vFreeChunkSizes[0], m_vFreeChunkSizes.GetCount(), UINT32_MAX,
                [ & ](const uint32_t& el, const uint32_t& min)
            {
                return el < min && el < size;
            } );

        }

        return ret;
    }

} // VKE