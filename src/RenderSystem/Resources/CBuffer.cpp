#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/Managers/CBufferManager.h"
#include "RenderSystem/CDeviceContext.h"
#include "Core/Memory/Memory.h"

namespace VKE
{
    namespace RenderSystem
    {
        CBuffer::CBuffer( CBufferManager* pMgr ) : m_pMgr( pMgr )
        {
        }

        CBuffer::~CBuffer()
        {
            // Destroy();
        }

        void CBuffer::_Destroy()
        {
            this->m_hObject = INVALID_HANDLE;
        }

        Result CBuffer::Init( const SBufferDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc.indexType = Desc.indexType;
            m_Desc.memoryUsage = Desc.memoryUsage;
            m_Desc.usage       = Desc.usage;
            m_Desc.SetDebugName( Desc.GetDebugName() );
            // Note m_Desc.size will be changed if Desc.backBuffering is set
            // or buffer is used as uniform buffer
            uint32_t    currOffset = 0;
            uint32_t    totalSize  = 0;
            const auto& Limits     = m_pMgr->m_pCtx->GetDeviceInfo().Limits;
            uint32_t    alignment  = 1;
            if( ( m_Desc.usage & BufferUsages::BUFFER ) == BufferUsages::BUFFER )
            {
                alignment = Limits.Alignment.storageBufferOffset;
                if( ( m_Desc.usage & BufferUsages::TEXEL_BUFFER ) == BufferUsages::TEXEL_BUFFER )
                {
                    alignment = Limits.Alignment.texelBufferOffset;
                }
            }
            else if( ( m_Desc.usage & BufferUsages::CONSTANT_BUFFER ) == BufferUsages::CONSTANT_BUFFER )
            {
                alignment = Limits.Alignment.constantBufferOffset;
                if( ( m_Desc.usage & BufferUsages::TEXEL_BUFFER ) == BufferUsages::TEXEL_BUFFER )
                {
                    alignment = Limits.Alignment.texelBufferOffset;
                }
                // Try to figure out best memory usage
                if( m_Desc.memoryUsage == MemoryUsages::UNDEFINED )
                {
                    m_Desc.memoryUsage = MemoryUsages::BUFFER | MemoryUsages::GPU_ACCESS;
                }
            }
            else if( ( m_Desc.usage & BufferUsages::VERTEX_BUFFER ) == BufferUsages::VERTEX_BUFFER ||
                     ( m_Desc.usage & BufferUsages::INDEX_BUFFER ) == BufferUsages::INDEX_BUFFER ||
                     ( m_Desc.usage & BufferUsages::TRANSFER_DST ) == BufferUsages::TRANSFER_DST )
            {
                // Try to figure out best memory usage
                if( m_Desc.memoryUsage == MemoryUsages::UNDEFINED )
                {
                    m_Desc.memoryUsage = MemoryUsages::BUFFER | MemoryUsages::GPU_ACCESS;
                }
            }

            m_alignment = (uint16_t)alignment;

            for( uint32_t i = 0; i < Desc.vRegions.GetCount(); ++i )
            {
                const auto& Curr = Desc.vRegions[ i ];
                
                SBufferRegion     Region;
                Region.elementSize  = Memory::CalcAlignedSize( Curr.elementSize, alignment );
                Region.elementCount  = Curr.elementCount;
                Region.offset        = currOffset;
                
                auto size      = Region.elementSize * Region.elementCount;
                Region.offset    = Memory::CalcAlignedSize( currOffset, alignment );
                currOffset      += size;
                totalSize       += size;
                
                VKE_ASSERT2( totalSize % alignment == 0, "" );
                VKE_ASSERT2( currOffset % alignment == 0, "" );
                VKE_ASSERT2( Region.elementSize % alignment == 0, "" );
                VKE_ASSERT2( size % alignment == 0, "" );
                m_Desc.vRegions.PushBack( Region );
            }
            
            VKE_ASSERT( !Desc.vRegions.IsEmpty() );
            m_size = totalSize;
            return ret;
        }

        uint32_t GetNextIndexInRingBuffer( const uint32_t currIdx, const uint32_t count )
        {
            uint32_t ret = ( currIdx + 1 ) % count;
            return ret;
        }

        hash_t CBuffer::CalcHash( const SBufferDesc& Desc )
        {
            Utils::SHash Hash;
            for(uint32_t i = 0; i < Desc.vRegions.GetCount(); ++i )
            {
                const auto& Curr = Desc.vRegions[ i ];
                Hash.Combine( Curr.offset, Curr.elementSize, Curr.elementCount );
            }
            return Hash.value;
        }

        uint32_t CBuffer::CalcAbsoluteOffset( const uint16_t& region, const uint32_t& elemIdx ) const
        {
            uint32_t       ret         = 0;
            const auto&    Curr        = m_Desc.vRegions[ region ];
            const uint32_t localOffset = Curr.elementSize * elemIdx;
            ret                        = Curr.offset + localOffset;

            VKE_ASSERT2( localOffset + Curr.elementSize <= Curr.elementSize * Curr.elementCount, "elemIdx out of bounds in the region." );
            VKE_ASSERT2( ret + Curr.elementSize <= GetSize(), "elemIdx out of bounds." );
            VKE_ASSERT2( ret % m_alignment == 0, "" );
            return ret;
        }

        uint32_t CBuffer::CalcRelativeOffset( const uint16_t& region, const uint32_t& elemIdx ) const
        {
            uint32_t    ret  = 0;
            const auto& Curr = m_Desc.vRegions[ region ];
            ret              = Curr.elementSize * elemIdx;
            VKE_ASSERT2( ret <= Curr.elementSize * Curr.elementCount, "elemIdx out of bounds in the region." );
            VKE_ASSERT2( ret + Curr.elementSize <= Curr.elementSize * Curr.elementCount, "elemIdx out of bounds." );
            VKE_ASSERT2( ret % m_alignment == 0, "" );
            return ret;
        }

        void* CBuffer::Map( uint32_t offset, uint32_t size )
        {
            VKE_ASSERT( offset < m_size );
            VKE_ASSERT( offset % m_alignment == 0 );
            VKE_ASSERT( size <= m_size - offset );
            
            SUpdateMemoryInfo Info;
            Info.hBuffer = GetDDIObject();
            Info.dataSize = size;
            Info.dstDataOffset = offset;
            Info.hMemory       = m_hMemory;
            return m_pMgr->LockMemory( Info );
        }

        void* CBuffer::MapRegion( uint16_t regionIndex, uint16_t elementIndex )
        {
            auto size   = GetRegionSize( regionIndex ) - ( elementIndex * m_Desc.vRegions[regionIndex].elementSize );
            auto offset = CalcAbsoluteOffset( regionIndex, elementIndex );
            //return m_pMgr->LockMemory( offset, size, &m_hMemory );
            return Map( offset, size );
        }

        void CBuffer::Unmap()
        {
            SUpdateMemoryInfo Info;
            Info.hBuffer       = GetDDIObject();
            Info.hMemory       = m_hMemory;
            m_pMgr->UnlockMemory( Info );
        }

    } // namespace RenderSystem
} // namespace VKE
