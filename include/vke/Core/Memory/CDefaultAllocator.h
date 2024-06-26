#pragma once
#include "Core/Memory/Memory.h"

namespace VKE
{
    namespace Memory
    {
        struct IAllocator
        {
            vke_force_inline
            void* Allocate(const size_t&) { return nullptr; }

            vke_force_inline
            void* Allocate(const size_t&, const uint32_t&) { return nullptr; }

            vke_force_inline
            void Free(const size_t&, void**) {}

            /// Free array of elements
            vke_force_inline
            void Free(const size_t&, const uint32_t&, void**) {}

        };

        class CHeapAllocator final : public IAllocator
        {
            public:

                static CHeapAllocator& GetInstance()
                {
                    static CHeapAllocator Allocator;
                    return Allocator;
                }

                static CHeapAllocator& Create()
                {
                    return GetInstance();
                }

                vke_force_inline
                void* Allocate(const size_t& size)
                {
                    return VKE_MALLOC(size);
                }

                vke_force_inline
                void* Allocate(const size_t& elemSize, const uint32_t& count)
                {
                    const auto size = elemSize * count;
                    return VKE_MALLOC(size);
                }

                vke_force_inline
                void Free(const size_t& size, void** pPtrOut)
                {
                    if (*pPtrOut && size > 0)
                        VKE_FREE(*pPtrOut);
                    *pPtrOut = nullptr;
                }

                vke_force_inline
                void Free(const size_t& elementSize, const uint32_t& count, void** pPtrOut)
                {
                    if( *pPtrOut && count > 0 )
                    {
                        VKE_FREE( *pPtrOut );
                        *pPtrOut = nullptr;
                    }
                }
        };

        template<typename _T_>
        struct THeapAllocator
        {
            static void* Allocate(const size_t& size)
            {
                return CHeapAllocator::GetInstance().Allocate(size);
            }

            static void* Allocate(const size_t& size, const uint32_t& count)
            {
                return CHeapAllocator::GetInstance().Allocate(size, count);
            }

            static void Free(const size_t& size, void** ppPtrOut)
            {
                CHeapAllocator::GetInstance().Free(size, ppPtrOut);
            }

            static void Free( const size_t& elementSize, const uint32_t& count, void** ppPtrOut )
            {
                CHeapAllocator::GetInstance().Free( elementSize, count, ppPtrOut );
            }
        };

    } // Memory
    inline Memory::CHeapAllocator& HeapAllocator = Memory::CHeapAllocator::GetInstance();
} // VKE