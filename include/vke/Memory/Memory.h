#pragma once

#include "VKECommon.h"
#include "Memory/CDefaultAllocator.h"

namespace VKE
{
    namespace Memory
    {
        template<typename _T_, typename _A_>
        vke_force_inline void* AllocMemory(_A_* pAllocator)
        {
            void* pMem = pAllocator->Allocate(sizeof(_T_));
            return pMem;
        }

        template<typename _T_, typename... _ARGS_>
        vke_force_inline _T_* CreateObject(void* pMemory, _ARGS_&&... args)
        {
            assert(pMemory);
            return new(pMemory)_T_(args...);
        }

        template<typename _T_, typename _A_, typename... _ARGS_>
        vke_force_inline Result CreateObject(_A_* pAllocator, _T_** ppPtrOut, _ARGS_&&... args)
        {
            void* pMem = pAllocator->Allocate(sizeof(_T_));
            if(pMem)
            {
                *ppPtrOut = new(pMem) _T_(args...);
                return VKE_OK;
            }
            *ppPtrOut = nullptr;
            return VKE_FAIL;
        }


        template<typename _T_, typename _A_>
        vke_force_inline void DestroyObject(_A_* pAllocator, _T_** ppPtrOut)
        {
            (*ppPtrOut)->~_T_();
            pAllocator->Free(sizeof(_T_), reinterpret_cast<void**>(ppPtrOut));
        }

    } // Memory
} // vke