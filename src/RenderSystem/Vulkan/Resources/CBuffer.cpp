#include "RenderSystem/Vulkan/Resources/CBuffer.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Managers/CBufferManager.h"
#include "RenderSystem/CDeviceContext.h"
#include "Core/Memory/Memory.h"

namespace VKE
{
    namespace RenderSystem
    {
        CBuffer::CBuffer( CBufferManager* pMgr ) :
            m_pMgr( pMgr )
        {

        }

        CBuffer::~CBuffer()
        {
            //Destroy();
        }

        void CBuffer::_Destroy()
        {
            this->m_hObject = NULL_HANDLE;
        }

        Result CBuffer::Init( const SBufferDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            // Note m_Desc.size will be changed if Desc.backBuffering is set
            // or buffer is used as uniform buffer
            uint32_t currOffset = 0;
            uint32_t totalSize = 0;
            const auto alignment = m_pMgr->m_pCtx->GetDeviceInfo().Limits.Alignment.minUniformBufferOffset;
            for( uint32_t i = 0; i < Desc.vRegions.GetCount(); ++i )
            {
                const auto& Curr = Desc.vRegions[i];
                SRegion Region;
                Region.elemSize = Memory::CalcAlignedSize( Curr.elementSize, (uint16_t)alignment );
                Region.size = Region.elemSize * Curr.elementCount;
                Region.offset = currOffset;
                currOffset += Region.size;
                totalSize += Region.size;
                m_vRegions.PushBack( Region );
            }
            if( m_Desc.size == 0 )
            {
                m_Desc.size = totalSize;
            }
            VKE_ASSERT( m_Desc.size >= currOffset, "Total buffer size must be greater or equal than sum of all region sizes." );
            return ret;
        }

        uint32_t GetNextIndexInRingBuffer(const uint32_t currIdx, const uint32_t count)
        {
            uint32_t ret = (currIdx + 1) % count;
            return ret;
        }

        hash_t CBuffer::CalcHash( const SBufferDesc& Desc )
        {
            SHash Hash;
            Hash.Combine( Desc.size, Desc.usage );
            return Hash.value;
        }

        uint32_t CBuffer::CalcOffset( const uint16_t& region, const uint16_t& elemIdx )
        {
            uint32_t ret = 0;
            const auto& Curr = m_vRegions[region];
            const uint32_t localOffset = Curr.elemSize * elemIdx;
            ret = Curr.offset + localOffset;

            VKE_ASSERT( localOffset + Curr.elemSize <= Curr.size, "elemIdx out of bounds in the region." );
            VKE_ASSERT( ret + Curr.elemSize <= m_Desc.size, "elemIdx out of bounds." );
            return ret;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER