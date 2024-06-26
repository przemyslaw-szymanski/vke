#pragma once
#include "Core/VKECommon.h"
#if VKE_COMPILER_VISUAL_STUDIO
#pragma warning( push )
#pragma warning( disable : 4100 )
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
namespace VKE
{
    namespace Memory
    {

        class CPageAllocator
        {
          public:
            using MemRange = TSExtent<memptr_t>;
            struct SFreeBlock
            {
                SFreeBlock* pNext;
            };
            typedef uint8_t DataType;
            typedef DataType* MemoryPool;
            CPageAllocator()
            {
            }
            CPageAllocator( CPageAllocator&& Other );
            CPageAllocator( uint32_t elementCount, size_t elementSize )
                : m_elementCount( elementCount )
                , m_elementSize( elementSize )
            {
            }
            ~CPageAllocator()
            {
                Destroy();
            }
            void operator=( const CPageAllocator& Other ) = delete;
            Result Create( uint32_t elementCount, size_t elementSize );
            void Destroy();
            const MemRange& GetMemoryRange() const
            {
                return m_MemoryRange;
            }
            void* Allocate()
            {
                return Alloc();
            }
            void* Allocate( size_t sSize )
            {
                return Alloc();
            }
            void* Allocate( void* pData )
            {
                return Alloc();
            }
            void* Allocate( void* pData, size_t dataSize )
            {
                return Alloc();
            }
            void* Allocate( uint32_t _uiElementCount, size_t elementSize )
            {
                return Alloc();
            }
            void* Allocate( size_t uiSize, uint32_t uiBlock, cstr_t pFileName, uint32_t line )
            {
                return Alloc();
            }
            void Deallocate( void* pPtr )
            {
                Free( pPtr );
            }
            void Deallocate( void** pPtr )
            {
                Free( *pPtr );
            }
            void Deallocate( void* pPtr, size_t size )
            {
                Free( pPtr );
            }
            void Deallocate( void* pPtr, uint32_t uiElementCount, size_t elementSize )
            {
                Free( pPtr );
            }
            memptr_t Alloc()
            {
                // ~0.000004 sec in debug
                if( m_pFirstFreeBlock != nullptr )
                {
                    memptr_t pPtr = reinterpret_cast<memptr_t>( m_pFirstFreeBlock );
                    m_pFirstFreeBlock = m_pFirstFreeBlock->pNext;
                    --m_freeBlockCount;
                    return pPtr;
                }
                return nullptr;
            }
            vke_inline Result Free( void* pPtr )
            {
                if( pPtr >= m_MemoryRange.begin && pPtr < m_MemoryRange.end )
                {
                    FreeNoCheck( pPtr );
                    return VKE_OK;
                }
                return VKE_ENOTFOUND;
            }
            vke_inline void FreeNoCheck( void* pPtr )
            {
                SFreeBlock* pFreeBlock = reinterpret_cast<SFreeBlock*>( pPtr );
                pFreeBlock->pNext = m_pFirstFreeBlock;
                m_pFirstFreeBlock = pFreeBlock;
                ++m_freeBlockCount;
            }

            protected:

                uint32_t _CreateChunk();

          protected:
            MemoryPool m_pMemoryPool = nullptr;
            SFreeBlock* m_pFirstFreeBlock = nullptr;
            MemRange m_MemoryRange;
            size_t m_memorySize = 0;
            size_t m_elementSize = 0;
            uint32_t m_elementCount = 0;
            uint32_t m_freeBlockCount = 0;
            bool m_isMemoryPoolCreated = false;
        };
    } // namespace Memory
} // namespace VKE
#if VKE_COMPILER_VISUAL_STUDIO
#pragma warning( pop )
#else
#pragma GCC diagnostic pop
#endif
