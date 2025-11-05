#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/TCSingleton.h"

namespace VKE
{
    namespace Memory
    {
        class CFreeList;

        class CFreeListPool
        {
            using FreeListVec = std::vector< CFreeList* >;
            using MemRange    = TSExtent< memptr_t >;
            using MemRangeVec = std::vector< MemRange >;

            // Max of 24 bits
            static const uint32_t MAX_ALLOC_COUNT = 0xFFFFFF;
            // Max of 8 bits
            static const uint32_t MAX_POOL_COUNT = 0xFF;

        public:
            struct SHandle
            {
                uint32_t poolIndex : 8;
                uint32_t allocIndex : 24;

                vke_force_inline operator bool() const
                {
                    return poolIndex != MAX_POOL_COUNT && allocIndex != MAX_ALLOC_COUNT;
                }
            };

            static const SHandle INVALID_HANDLE;

        public:
            CFreeListPool();
            virtual ~CFreeListPool();

            Result Create( uint32_t freeListElementCount, size_t freeListElemenetSize, uint32_t freeListCount );
            Result AddNewLists( uint32_t count );
            void   Destroy();

            memptr_t Allocate( const uint32_t = 0 /*used for Memory::Create*/ );
            memptr_t Allocate( SHandle* pHandle );

            template< class T, typename... ArgsT >
            T* AllocateObject( SHandle* pHandle, ArgsT&&... );

            Result Free( const uint32_t, void** ppPtr );
            Result Free( SHandle handle );

            memptr_t Get( SHandle handle );

        protected:
            FreeListVec m_vpFreeLists;
            MemRangeVec m_vFreeListMemRanges;
            CFreeList*  m_pCurrList         = nullptr;
            uint32_t    m_currListId        = 0;
            uint32_t    m_freeListElemCount = 0;
            size_t      m_freeListElemSize  = 0;
        };

        template< class T, typename... ArgsT >
        T* CFreeListPool::AllocateObject( SHandle* pHandle, ArgsT&&... args )
        {
            memptr_t pMem = Allocate( pHandle );
            return ::new( pMem ) T( std::forward< ArgsT >( args )... );
        }
    } // namespace Memory
} // namespace VKE
