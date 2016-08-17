#pragma once

#include "Core/VKECommon.h"
#include "Core/Memory/CDefaultAllocator.h"

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
            assert(ppPtrOut && *ppPtrOut);
            (*ppPtrOut)->~_T_();
            pAllocator->Free(sizeof(_T_), reinterpret_cast<void**>(ppPtrOut));
        }

        template<typename _T_>
        static vke_inline void Set(_T_* pData, const _T_& tValue, const uint32_t count = 1)
        {
            memset(pData, tValue, sizeof(_T_) * count);
        }

        template<typename _T_>
        static vke_inline void Zero(_T_* pData, const uint32_t count = 1)
        {
            Set(pData, 0, count);
        }

        static vke_inline void Copy(void* pDst, const size_t dstSize, const void* pSrc, const size_t bytesToCopy)
        {
#if _MSC_VER
            memcpy_s(pDst, dstSize, pSrc, bytesToCopy);
#else
            memcpy(pDst, pSrc, bytesToCopy);
#endif
        }

    } // Memory
} // vke