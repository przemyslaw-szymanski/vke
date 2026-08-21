#pragma once

#include "Core/VKECommon.h"
#include "Core/Platform/CPlatform.h"
#include "Core/Utils/TCBitset.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/CLogger.h"
#include "Core/Math/Math.h"

namespace VKE
{
    namespace Utils
    {
        template< typename ChunkSizeT >
        class TCBitPool
        {
            using ChunkType                                 = Utils::TCBitset< ChunkSizeT >;
            using ChunkArray                                = Utils::TCDynamicArray< ChunkType, 1 >;
            using UintArray                                 = Utils::TCDynamicArray< ExtentU32, 1 >;
            static constexpr uint32_t   BitsPerChunk        = sizeof( ChunkSizeT ) * 8;
            static constexpr uint32_t   LastBitInChunkIndex = BitsPerChunk - 1;
            static constexpr ChunkSizeT AllBitsSet          = std::numeric_limits< ChunkSizeT >::max();

            struct SBitHandle
            {
                uint32_t chunkIndex : 26;
                uint32_t bitIndex : 6; // max 64 bits due to uint64_t

                bool IsLastBit() const
                {
                    return bitIndex == LastBitInChunkIndex;
                }
            };

        public:
            Result Create( uint32_t slotCount )
            {
                // Round up
                auto numChunks = slotCount / BitsPerChunk + 1;
                if( !m_vBitPool.Resize( numChunks, static_cast < ChunkSizeT >( 0 ) ) )
                {
                    return VKE_ENOMEMORY;
                }
                if( !m_vFreeBitRanges.Reserve( slotCount ) )
                {
                    return VKE_ENOMEMORY;
                }
                m_vFreeBitRanges.PushBack( { 0, slotCount } );
                m_numBits = slotCount;
                return VKE_OK;
            }

            SBitHandle GetBitHandle( uint32_t slotIndex )
            {
                SBitHandle Ret;
                Ret.chunkIndex = slotIndex / BitsPerChunk;
                Ret.bitIndex   = slotIndex % BitsPerChunk;
                return Ret;
            }

            SBitHandle GetNextBit( SBitHandle Handle )
            {
                /// TODO: make it faster. This if check is probably redundant
                if( Handle.bitIndex + 1u >= BitsPerChunk )
                {
                    Handle.bitIndex = 0;
                    Handle.chunkIndex++;
                    VKE_ASSERT( Handle.chunkIndex < m_vBitPool.GetCount() );
                    return Handle;
                }
                Handle.bitIndex++;
                return Handle;
            }

            bool IsBitAllocated( SBitHandle Handle )
            {
                return m_vBitPool[ Handle.chunkIndex ].IsBitSet( Handle.bitIndex );
            }

            bool IsBitAllocated( uint32_t bitIndex )
            {
                return IsBitAllocated( GetBitHandle( bitIndex ) );
            }

            uint32_t GetAbsoluteBitIndex( SBitHandle Handle )
            {
                return Handle.chunkIndex * BitsPerChunk + Handle.bitIndex;
            }

            template< bool SetBitT >
            vke_force_inline void SetBit( SBitHandle Handle )
            {
                m_vBitPool[ Handle.chunkIndex ].template SetBit< SetBitT >( Handle.bitIndex );
            }

            template< bool SetBitT >
            vke_force_inline void SetBits( SBitHandle BitHandle, uint32_t numBits )
            {
                const auto numBitsLeftInFirstChunk = Math::Min( BitsPerChunk - BitHandle.bitIndex, numBits );
                const auto numWholeChunks          = ( BitsPerChunk - BitHandle.bitIndex ) / BitsPerChunk;
                uint32_t   currSlot                = 0;
                ChunkType* pChunk                  = &m_vBitPool[ BitHandle.chunkIndex ];
                // Reset bits in current chunk
                for( uint8_t b = BitHandle.bitIndex; currSlot < numBitsLeftInFirstChunk; ++b, ++currSlot )
                {
                    pChunk->template SetBit< SetBitT >( b );
                }
                // Move to the next bit chunk
                BitHandle.chunkIndex++;
                BitHandle.bitIndex = 0;
                // Reset whole chunks
                for( uint32_t c = BitHandle.chunkIndex; c < numWholeChunks; ++c )
                {
                    pChunk    = &m_vBitPool[ c ];
                    *pChunk   = static_cast<ChunkSizeT>( 0 );
                    currSlot += BitsPerChunk;
                    BitHandle.chunkIndex++;
                }
                // Reset slots in last chunk
                auto numRemainingSlots = numBits - currSlot;
                VKE_ASSERT( numRemainingSlots <= BitsPerChunk );
                pChunk = &m_vBitPool[ BitHandle.chunkIndex ];
                for( uint8_t b = 0; b < numRemainingSlots; ++b )
                {
                    pChunk->template SetBit< SetBitT >( b );
                }
            }

            uint32_t AllocateSlots( uint32_t numSlots )
            {
                uint32_t ret = UNDEFINED_U32;
                // Find enouth space to allocate
                for( uint32_t i = 0; i < m_vFreeBitRanges.GetCount(); ++i )
                {
                    // Get absolute bit index
                    auto& FreeRange = m_vFreeBitRanges[ i ];
                    if( FreeRange.end > 0 )
                    {
                        // Get chunk index
                        SBitHandle FirstBit     = GetBitHandle( FreeRange.begin );
                        auto&      Chunk        = m_vBitPool[ FirstBit.chunkIndex ];
                        bool       bitAllocated = Chunk.IsBitSet( FirstBit.bitIndex );
                        if( bitAllocated )
                        {
                            m_vFreeBitRanges.RemoveFast( i );
                            continue;
                        }
                        auto     TmpBit       = FirstBit;
                        uint32_t numFreeSlots = !bitAllocated;

                        while( !bitAllocated && numFreeSlots < numSlots )
                        {
                            TmpBit        = GetNextBit( TmpBit );
                            bitAllocated  = IsBitAllocated( TmpBit );
                            numFreeSlots += !bitAllocated;
                        }
                        if( numFreeSlots >= numSlots )
                        {
                            // Shrink instead of removing free slot
                            if( FreeRange.end > numFreeSlots )
                            {
                                FreeRange.begin += numFreeSlots; // move first free slot
                                FreeRange.end   -= numFreeSlots; // reduce number of free slots
                            }
                            else
                            {
                                m_vFreeBitRanges[ i ].end = 0;
                            }
                            ret = GetAbsoluteBitIndex( FirstBit );
                            SetBits< true >( FirstBit, numSlots );
                            break;
                        }
                    }
                }
                return ret;
            }

            ExtentU32* GetFreeRange( uint32_t slotIndex )
            {
                for( uint32_t i = 0; i < m_vFreeBitRanges.GetCount(); ++i )
                {
                    auto& Range = m_vFreeBitRanges[ i ];
                    if( Range.begin == slotIndex || Range.begin + Range.end - 1 == slotIndex )
                    {
                        return &m_vFreeBitRanges[ i ];
                    }
                }
                return nullptr;
            }

            void FreeSlots( uint32_t firstSlotIndex, uint32_t slotCount )
            {
                // Merge with next free space
                const uint32_t nextSlotIndex = firstSlotIndex + slotCount;
                const uint32_t prevSlotIndex = firstSlotIndex > 0 ? firstSlotIndex - 1 : firstSlotIndex;
                if( nextSlotIndex < m_numBits && !IsBitAllocated( nextSlotIndex ) )
                {
                    ExtentU32* pRange = GetFreeRange( nextSlotIndex );
                    VKE_ASSERT( pRange );
                    pRange->begin  = firstSlotIndex;
                    pRange->end   += slotCount;
                }
                else if( prevSlotIndex != firstSlotIndex && !IsBitAllocated( prevSlotIndex ) )
                {
                    ExtentU32* pRange = GetFreeRange( prevSlotIndex );
                    VKE_ASSERT( pRange );
                    pRange->end += slotCount;
                }
                else
                {
                    const ExtentU32 NewRange = { firstSlotIndex, slotCount };
                    // Find first free slot to use
                    bool addNew = true;
                    for( uint32_t i = 0; i < m_vFreeBitRanges.GetCount(); ++i )
                    {
                        if( m_vFreeBitRanges[ i ].end == 0 )
                        {
                            m_vFreeBitRanges[ i ] = NewRange;
                            addNew                = false;
                            break;
                        }
                    }
                    if( addNew )
                    {
                        const bool newFirst = true;
                        if( newFirst )
                        {
                            m_vFreeBitRanges.PushBack( m_vFreeBitRanges.Back() );
                            // swap with prev last
                            m_vFreeBitRanges[ m_vFreeBitRanges.GetCount() - 2 ] = NewRange;
                        }
                        else
                        {
                            m_vFreeBitRanges.PushBack( NewRange );
                        }
                    }
                }
                {
                    SetBits< false >( GetBitHandle( firstSlotIndex ), slotCount );
                }
            }

            void Print( std::string&& s )
            {
                VKE_LOGGER_BEGIN( "[INFO]" );
                VKE_LOGGER << s << "\t";
                for( uint32_t c = 0; c < m_vBitPool.GetCount(); ++c )
                {
                    const ChunkType& Chunk = m_vBitPool[ c ];
                    VKE_LOGGER << "|";
                    for( uint8_t b = 0; b < BitsPerChunk; ++b )
                    {
                        VKE_LOGGER << Chunk.IsBitSet( b );
                    }
                }
                VKE_LOGGER_END;
            }

        protected:
            uint32_t m_numBits = 0;

            ChunkArray m_vBitPool;
            UintArray  m_vFreeBitRanges;
        };
    } // namespace Utils

} // namespace VKE
