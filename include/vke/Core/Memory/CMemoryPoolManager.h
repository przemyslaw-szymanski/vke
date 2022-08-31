#pragma once

#include "Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    class VKE_API CMemoryPoolView
    {
        protected:

            struct SChunk
            {
                uint32_t offset;
                uint32_t size;
            };
            using ChunkVec = Utils::TCDynamicArray< SChunk >;
            using UintVec = Utils::TCDynamicArray< uint32_t >;

        public:

            enum ALLOC_ALGORITHM
            {
                ALLOC_FROM_MAIN_CHUNK,
                ALLOC_FROM_FREE_WITH_BEST_FIT,
                ALLOC_FROM_FREE_WITH_FIRST_AVAILABLE
            };

            static const uint64_t   INVALID_ALLOCATION = UNDEFINED_U64;

            struct SAllocateData
            {
                uint64_t    memory;
                uint32_t    offset;
                uint32_t    size;
            };

            struct SInitInfo
            {
                uint64_t    memory;
                uint32_t    poolIdx;
                uint32_t    offset;
                uint32_t    size;
                uint16_t    allocationAlignment;
            };

        public:

            Result      Init(const SInitInfo& Info);

            template<ALLOC_ALGORITHM Algorithm = ALLOC_FROM_FREE_WITH_BEST_FIT>
            uint64_t    Allocate( const SAllocateMemoryInfo& Info, SAllocateData* pOut );
            void        Free( const SAllocateData& Data );
            void        Defragment();
            bool        CanDefragment() const { return !m_vFreeChunks.IsEmpty(); }

            const SInitInfo& GetDesc() const { return m_InitInfo; }

            void UpdateDebugInfo( const SAllocateMemoryInfo::SDebugInfo* pInfo )
            {
                ( void )pInfo;
#if VKE_MEMORY_DEBUG
                m_vAllocations.Back().Debug = *pInfo;
#endif
            }

            void LogDebug() const;

        protected:

            uint64_t    _AllocateFromMainChunkFirst( const SAllocateMemoryInfo& Info, SAllocateData* pOut );
            uint64_t    _AllocateFromFreeFirstWithBestFit( const SAllocateMemoryInfo& Info, SAllocateData* pOut );
            uint64_t    _AllocateFromFreeFirstFirstAvailable( const SAllocateMemoryInfo& Info, SAllocateData* pOut );
            uint32_t    _FindFirstFree( uint32_t size );
            uint32_t    _FindBestFitFree( uint32_t size );

        protected:

            SInitInfo   m_InitInfo;
            SChunk      m_MainChunk;
            ChunkVec    m_vFreeChunks;
            UintVec     m_vFreeChunkSizes;
            UintVec     m_vFreeChunkOffsets;
#if VKE_MEMORY_DEBUG
            Utils::TCDynamicArray<SAllocateMemoryInfo, 256> m_vAllocations;
#endif
    };

    template<CMemoryPoolView::ALLOC_ALGORITHM Algorithm>
    uint64_t CMemoryPoolView::Allocate( const SAllocateMemoryInfo& Info, SAllocateData* pOut )
    {
        uint64_t ret;
        switch( Algorithm )
        {
            case ALLOC_FROM_MAIN_CHUNK:
            {
                ret = _AllocateFromMainChunkFirst( Info, pOut );
            }
            break;
            case ALLOC_FROM_FREE_WITH_BEST_FIT:
            {
                ret = _AllocateFromFreeFirstWithBestFit( Info, pOut );
            }
            break;
            case ALLOC_FROM_FREE_WITH_FIRST_AVAILABLE:
            {
                ret = _AllocateFromFreeFirstFirstAvailable( Info, pOut );
            }
            break;
            default:
                ret = 0;
            break;
        };
#if VKE_MEMORY_DEBUG
        if(ret != UNDEFINED_U64)
        {
            //VKE_ASSERT2( !Info.Debug.Name.empty(), "Debug info must be set." );
            m_vAllocations.PushBack( Info );
        }
#endif
        return ret;
    }


} // VKE