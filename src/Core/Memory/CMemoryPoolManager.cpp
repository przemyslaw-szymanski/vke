#include "Core/Memory/CMemoryPoolManager.h"

namespace VKE
{
    namespace Memory
    {
        uint32_t CalcAlignedSize( uint32_t size, uint32_t alignment )
        {
            return size + ( size % alignment );
        }

        Result CMemoryPoolManager::_FindFreeMemory(const SAllocateInfo& Info, SMemoryChunk** ppOut)
        {
            auto& Buffers = m_vMemoryBuffers[ Info.type ][ Info.index ];
            auto& vFreeChunks = Buffers.vFreeChunks;
            uint32_t minIdx = UINT32_MAX;
            uint32_t minDiff = UINT32_MAX;
            Result res = VKE_ENOTFOUND;
            const uint32_t alignedSize = CalcAlignedSize( Info.size, Info.alignment );

            const uint32_t count = vFreeChunks.GetCount();
            for( uint32_t i = 0; i < count; ++i )
            {
                auto& Chunk = vFreeChunks[ i ];
                if( Chunk.size <= alignedSize )
                {
                    const uint32_t diff = alignedSize - Info.size;
                    if( diff < minDiff )
                    {
                        minDiff = diff;
                        minIdx = i;
                        *ppOut = &Chunk;
                        res = VKE_OK;
                    }
                }
            }
            return res;
        }   

        Result CMemoryPoolManager::Allocate(const SAllocateInfo& Info, SAllocateData* pOut)
        {
            SMemoryChunk* pMemChunk = nullptr;
            Result res = _FindFreeMemory( Info, &pMemChunk );
            if( VKE_SUCCEEDED( res ) )
            {
                const uint32_t alignedSize = CalcAlignedSize( Info.size, Info.alignment );
                pOut->memory = pMemChunk->memory;
                pOut->offset = pMemChunk->offset;
                pOut->type = Info.type;
                pOut->index = Info.index;
                pOut->size = alignedSize;

                pMemChunk->offset += alignedSize;
                pMemChunk->size -= alignedSize;
            }
            else
            {
                res = VKE_ENOMEMORY;
            }
            return res;
        }

        Result CMemoryPoolManager::AllocatePool(const SPoolAllocateInfo& Info)
        {
            Result res = VKE_ENOMEMORY;
            auto& MemBuffers = m_vMemoryBuffers[ Info.type ][ Info.index ];
            MemBuffers.index = Info.index;
            {
                SMemoryBuffer Buffer;
                Buffer.memory = Info.memory;
                Buffer.size = Info.size;
                if( MemBuffers.vBuffers.PushBack( Buffer ) != Utils::INVALID_POSITION )
                {
                    SMemoryChunk FreeChunk;
                    FreeChunk.index = Info.index;
                    FreeChunk.memory = Info.memory;
                    FreeChunk.offset = 0;
                    FreeChunk.size = Info.size;
                    if( MemBuffers.vFreeChunks.PushBack( FreeChunk ) != Utils::INVALID_POSITION )
                    {
                        res = VKE_OK;
                    }
                }
            }
            return res;
        }

        Result CMemoryPoolManager::Free(const SAllocateData& Data)
        {
            auto& Buffers = m_vMemoryBuffers[ Data.type ][ Data.index ];
            SMemoryChunk Chunk;
            Chunk.index = Data.index;
            Chunk.memory = Data.memory;
            Chunk.offset = Data.offset;
            Chunk.size = Data.size;
            Buffers.vFreeChunks.PushBack( Chunk );
        }

    } // Memory
} // VKE