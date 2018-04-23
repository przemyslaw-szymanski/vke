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
            
            vke_force_inline
            void Free(const size_t&, void**, const uint32_t&) {}
            
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
                void Free(const size_t& size, void** pPtrOut, const uint32_t&)
                {
                    if (*pPtrOut && size > 0)
                        VKE_FREE(*pPtrOut);
                    *pPtrOut = nullptr;
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

            static void Free(const size_t& size, void** ppPtrOut, const uint32_t& p)
            {
                CHeapAllocator::GetInstance().Free(size, ppPtrOut, p);
            }
        };

    } // Memory
    static Memory::CHeapAllocator& HeapAllocator = Memory::CHeapAllocator::GetInstance();
} // VKE