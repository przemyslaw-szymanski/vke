#include "Core/Memory/CMemoryPoolManager.h"
#include "Core/Utils/CTimer.h"

#define VKE_ENABLE_MEM_POOL_WARNINGS 0

namespace VKE
{
    Result CMemoryPoolView::Init( const SInitInfo& Info )
    {
        Result ret = VKE_OK;
        m_InitInfo = Info;
        m_MainChunk.offset = Info.offset;
        m_MainChunk.size = Info.size;
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

    uint64_t CMemoryPoolView::_AllocateFromMainChunkFirst( const SAllocateMemoryInfo& Info, SAllocateData* pOut )
    {
        uint64_t ret = INVALID_ALLOCATION;
        uint32_t size = CalcAlignedSize( Info.size, Info.alignment );
        // If there is a space in main memory
        if( m_MainChunk.size >= size )
        {
            const uint32_t alignedOffset = size; //CalcAlignedSize( m_MainChunk.offset, Info.alignment );

            ret = m_InitInfo.memory + alignedOffset;

            pOut->memory = m_InitInfo.memory;
            pOut->offset = m_MainChunk.offset;
            pOut->size = size;

            m_MainChunk.size -= size;
            m_MainChunk.offset += alignedOffset;
        }
        else
        {
            uint32_t idx = _FindBestFitFree( size );
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

    uint64_t CMemoryPoolView::_AllocateFromFreeFirstWithBestFit( const SAllocateMemoryInfo& Info, SAllocateData* pOut )
    {
        uint64_t ret = INVALID_ALLOCATION;
        uint32_t size = CalcAlignedSize( Info.size, Info.alignment );
        uint32_t idx = _FindBestFitFree( size );
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
        // If there is a space in main memory
        if( m_MainChunk.size >= size )
        {
            const uint32_t alignedOffset = size; //CalcAlignedSize( m_MainChunk.offset, Info.alignment );

            ret = m_InitInfo.memory + alignedOffset;

            pOut->memory = m_InitInfo.memory;
            pOut->offset = m_MainChunk.offset;
            pOut->size = size;

            m_MainChunk.size -= size;
            m_MainChunk.offset += alignedOffset;
        }
        else
        {
            if( !m_vFreeChunks.IsEmpty() )
            {
                Defragment();
            }
            idx = _FindBestFitFree( size );
            if( idx == UNDEFINED_U32 )
            {
#if VKE_ENABLE_MEM_POOL_WARNINGS
                VKE_LOG_WARN( "No free memory left in CMemoryPoolView for requested size: " << size );
#endif
            }
            //VKE_ASSERT( idx != UNDEFINED_U32, "" );
        }
        return ret;
    }

    uint64_t CMemoryPoolView::_AllocateFromFreeFirstFirstAvailable( const SAllocateMemoryInfo& Info, SAllocateData* pOut )
    {
        uint64_t ret = INVALID_ALLOCATION;
        uint32_t size = CalcAlignedSize( Info.size, Info.alignment );
        uint32_t idx = _FindFirstFree( size );
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
        // If there is a space in main memory
        if( m_MainChunk.size >= size )
        {
            const uint32_t alignedOffset = size; //CalcAlignedSize( m_MainChunk.offset, Info.alignment );

            ret = m_InitInfo.memory + alignedOffset;

            pOut->memory = m_InitInfo.memory;
            pOut->offset = m_MainChunk.offset;
            pOut->size = size;

            m_MainChunk.size -= size;
            m_MainChunk.offset += alignedOffset;
        }
        else
        {
            VKE_LOG_ERR( "No free memory left in CMemoryPoolView for requested size: " << size );
        }
        return ret;
    }

    void CMemoryPoolView::Free( const SAllocateData& Data )
    {
        SChunk Chunk;
        Chunk.offset = Data.offset;
        Chunk.size = Data.size;

        m_vFreeChunkSizes.PushBack( Data.size );
        m_vFreeChunkOffsets.PushBack( Data.offset );
        m_vFreeChunks.PushBack( Chunk );
    }

    void CMemoryPoolView::Defragment()
    {
        // Sort offsets
        VKE_ASSERT( !m_vFreeChunks.IsEmpty(), "" );
        auto pFirst = &m_vFreeChunks.Front();
        auto pLast = &m_vFreeChunks.Back();
        std::sort( pFirst, pLast, [&](const SChunk& Left, const SChunk& Right)
        {
            return Left.offset < Right.offset;
        } );

        for( uint32_t i = 0; i < m_vFreeChunks.GetCount()-1; ++i )
        {
            auto& Left = m_vFreeChunks[i];
            auto& Right = m_vFreeChunks[i + 1];

            if( Left.offset + Left.size == Right.offset )
            {
                Left.size += Right.size; // merge right to left
                Right.size = 0; // mark right for remove
            }
        }

        // Remove marked chunks
        for( uint32_t i = 0; i < m_vFreeChunks.GetCount(); ++i )
        {
            auto& Curr = m_vFreeChunks[i];
            if( Curr.size == 0 )
            {
                m_vFreeChunks.RemoveFast( i );
            }
        }

        m_vFreeChunkSizes.Resize( m_vFreeChunks.GetCount() );
        m_vFreeChunkOffsets.Resize( m_vFreeChunks.GetCount() );

        for( uint32_t i = 0; i < m_vFreeChunks.GetCount(); ++i )
        {
            const auto& Curr = m_vFreeChunks[i];
            m_vFreeChunkOffsets[i] = Curr.offset;
            m_vFreeChunkSizes[i] = Curr.size;
        }
    }

    template<typename T, typename Callback>
    uint32_t FindMin( T* pArray, uint32_t count, const T& max, Callback&& Cb )
    {
        T min = max;
        uint32_t ret = UNDEFINED_U32;

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

    uint32_t CMemoryPoolView::_FindFirstFree( uint32_t size )
    {
        static const bool FindFirstFree = false;
        uint32_t ret = UNDEFINED_U32;
        for( uint32_t i = 0; i < m_vFreeChunkSizes.GetCount(); ++i )
        {
            if( m_vFreeChunkSizes[i] >= size )
            {
                ret = i;
                break;
            }
        }
        return ret;
    }

    uint32_t CMemoryPoolView::_FindBestFitFree( uint32_t size )
    {
        uint32_t ret = UNDEFINED_U32;
        if( !m_vFreeChunkSizes.IsEmpty() )
        {
            const auto pPtr = m_vFreeChunkSizes.GetData();
            uint32_t idx = FindMin( pPtr, m_vFreeChunkSizes.GetCount(), UINT32_MAX,
                [ & ]( const uint32_t& el, const uint32_t& min )
            {
                return el < min && el >= size;
            } );
            ret = idx;
        }
        return ret;
    }

} // VKE