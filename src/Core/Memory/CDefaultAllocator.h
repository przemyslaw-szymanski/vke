#pragma once
#include "Core/Memory/Memory.h"

namespace VKE
{
    namespace Memory
    {
        class CHeapAllocator final
        {
            public:

                static CHeapAllocator& GetInstance()
                {
                    static CHeapAllocator Allocator;
                    return Allocator;
                }

                vke_force_inline
                void* Allocate(const size_t& size)
                {
                    return VKE_MALLOC(size);
                }

                vke_force_inline
                void Free(const size_t& size, void** pPtrOut)
                {
                    if (*pPtrOut && size > 0)
                        VKE_FREE(*pPtrOut);
                    *pPtrOut = nullptr;
                }
        };
    } // Memory
    static Memory::CHeapAllocator& HeapAllocator = Memory::CHeapAllocator::GetInstance();
} // VKE