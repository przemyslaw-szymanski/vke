#include "Core/Memory/CMemoryPoolManager.h"
#include "Core/Utils/CTimer.h"

namespace VKE
{
    namespace Memory
    {
        CMemoryPoolManager::CMemoryPoolManager()
        {

        }

        CMemoryPoolManager::~CMemoryPoolManager()
        {
            Destroy();
        }

        void CMemoryPoolManager::Destroy()
        {

        }

        Result CMemoryPoolManager::Create(const SMemoryPoolManagerDesc& Desc)
        {
            Result res = VKE_FAIL;
            if( m_vvMemoryBuffers.Resize( Desc.poolTypeCount ) )
            {
                for( uint32_t i = 0; i < Desc.poolTypeCount; ++i )
                {
                    m_vvMemoryBuffers[ i ].Resize( Desc.indexTypeCount );
                }

                if( m_vTmpData.Resize( Desc.poolTypeCount ) )
                {
                    m_mAllocatedData.reserve( Desc.defaultAllocationCount );
                    m_vAllocatedData.Reserve( Desc.defaultAllocationCount );
                    m_vCaches.Resize( Desc.poolTypeCount );
                    res = VKE_OK;
                }
            }
            return res;
        }

        uint32_t CalcAlignedSize(uint32_t size, uint32_t alignment)
        {
            uint32_t ret = size;
            uint32_t remainder = size % alignment;
            if( remainder > 0 )
            {
                ret = size + alignment - remainder;
            }

            return ret;
        }

        uint32_t CMemoryPoolManager::_FindFreeMemory(const SAllocateInfo& Info, SFindMemoryData* pOut)
        {
            SMemoryBufferData* pMemData;
            SCache& Cache = m_vCaches[ Info.type ];
            if( Cache.lastUsedIndex == Info.index )
            {
                pMemData = Cache.pLastUsedMemoryData;
            }
            else
            {
                pMemData = &m_vvMemoryBuffers[ Info.type ][ Info.index ];
                Cache.lastUsedIndex = Info.index;
                Cache.pLastUsedMemoryData = pMemData;
            }

            auto& vFreeChunks = pMemData->FreeMemory.vChunkSizes;
            
            uint32_t minIdx = UINT32_MAX;
            uint32_t minDiff = UINT32_MAX;
            uint32_t ret = 0;
            const uint32_t alignedSize = CalcAlignedSize( Info.size, Info.alignment );
            const uint32_t count = vFreeChunks.GetCount();

            for( uint32_t i = 0; i < count; ++i )
            {
                const uint32_t freeSpace = vFreeChunks[ i ];
                if( freeSpace >= alignedSize )
                {
                    const uint32_t diff = freeSpace - alignedSize;
                    if( diff < minDiff )
                    {
                        minDiff = diff;
                        minIdx = i;
                    }
                }
            }

            if( minIdx < UINT32_MAX )
            {
                pOut->pMemoryData = pMemData;
                pOut->freeChunkIdx = minIdx;
                ret = alignedSize;
            }

            return ret;
        }   

        uint64_t CMemoryPoolManager::Allocate(const SAllocateInfo& Info, SAllocatedData* pOut)
        {
            SFindMemoryData FindData;

            const uint32_t alignedSize = _FindFreeMemory( Info, &FindData );
            uint64_t ret = 0;

            if( alignedSize > 0 )
            {
                SMemoryChunk* pMemChunk = &FindData.pMemoryData->FreeMemory.vChunks[ FindData.freeChunkIdx ];
                uint32_t* pChunkSize = &FindData.pMemoryData->FreeMemory.vChunkSizes[ FindData.freeChunkIdx ];

                pOut->Memory.memory = pMemChunk->memory;
                pOut->Memory.offset = pMemChunk->offset;
                pOut->Memory.size = alignedSize;
                pOut->type = Info.type;
                pOut->index = Info.index;

                uint64_t memPtr = pMemChunk->memory + pMemChunk->offset;
                
                //m_mAllocatedData.insert( { memPtr, *pOut } );
                //m_mAllocatedData[ memPtr ] = *pOut;
                m_vAllocatedData.PushBack( *pOut );
                
                pMemChunk->offset += alignedSize;
                pMemChunk->size -= alignedSize;
                *pChunkSize = pMemChunk->size;
                ret = memPtr;
            }

            return ret;
        }

        Result CMemoryPoolManager::_AddFreeChunk(const SMemoryChunk& Chunk, SFreeMemoryData* pOut)
        {
            Result res = VKE_ENOMEMORY;
            if( pOut->vChunks.PushBack( Chunk ) != Utils::INVALID_POSITION )
            {
                if( pOut->vChunkSizes.PushBack( Chunk.size ) != Utils::INVALID_POSITION )
                {
                    res = VKE_OK;
                }
            }
            return res;
        }

        void CMemoryPoolManager::_MergeFreeChunks(SFreeMemoryData* pOut)
        {
            auto& vFreeChunks = pOut->vChunks;
            STemporaryData* pTmp = &m_vTmpData[ 0 ];
            // Find not locked tmp data
            for( uint32_t i = 0; i < m_vTmpData.GetCount(); ++i )
            {
                if( !m_vTmpData[ i ].SyncObj.IsLocked() )
                {
                    pTmp = &m_vTmpData[ i ];
                    break;
                }
            }
            Threads::ScopedLock l( pTmp->SyncObj );
            auto& vTmpChunks = pTmp->vChunks;

            const uint32_t count = vFreeChunks.GetCount();
            for( uint32_t i = 0; i < count; ++i )
            {
                const auto& FreeChunk = vFreeChunks[ i ];
                SMemoryChunk CurrentChunk = FreeChunk;
                for( uint32_t j = i + 1; j < count; ++j )
                {
                    const auto& NextChunk = vFreeChunks[ j ];
                    if( CurrentChunk.memory == NextChunk.memory )
                    {
                        if( CurrentChunk.offset + CurrentChunk.size == NextChunk.offset )
                        {
                            CurrentChunk.size += NextChunk.size;
                        }
                    }
                }
                vTmpChunks.PushBack( CurrentChunk );
            }
            vFreeChunks.Clear();
            vFreeChunks.Append( vTmpChunks );
        }

        Result CMemoryPoolManager::AllocatePool(const SPoolAllocateInfo& Info)
        {
            Result res = VKE_ENOMEMORY;
            auto& MemBuffers = m_vvMemoryBuffers[ Info.type ][ Info.index ];
            MemBuffers.index = Info.index;
            {
                SMemoryBuffer Buffer;
                Buffer.memory = Info.memory;
                Buffer.size = Info.size;
                if( MemBuffers.vBuffers.PushBack( Buffer ) != Utils::INVALID_POSITION )
                {
                    SMemoryChunk FreeChunk;
                    FreeChunk.memory = Info.memory;
                    FreeChunk.offset = 0;
                    FreeChunk.size = Info.size;
                    res = _AddFreeChunk( FreeChunk, &MemBuffers.FreeMemory );
                }
            }
            return res;
        }

        Result CMemoryPoolManager::Free(uint64_t memory)
        {
            Result res = VKE_ENOTFOUND;
            auto Itr = m_mAllocatedData.find(memory);
            if( Itr != m_mAllocatedData.end() )
            {
                SAllocatedData& Data = Itr->second;
                auto& Buffers = m_vvMemoryBuffers[ Data.type ][ Data.index ];
                res = _AddFreeChunk( Data.Memory, &Buffers.FreeMemory );
            }
            else
            {
                for( uint32_t i = 0; i < m_vAllocatedData.GetCount(); ++i )
                {
                    SAllocatedData& Data = m_vAllocatedData[ i ];
                    uint64_t memPtr = Data.Memory.memory + Data.Memory.offset;
                    if( memPtr == memory )
                    {
                        auto& Buffers = m_vvMemoryBuffers[ Data.type ][ Data.index ];
                        res = _AddFreeChunk( Data.Memory, &Buffers.FreeMemory );
                        break;
                    }
                }
            }
            return res;
        }

    } // Memory
} // VKE