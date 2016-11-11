#pragma once

#include "Core/VKECommon.h"
#include "Core/Memory/CDefaultAllocator.h"

namespace VKE
{
    namespace Memory
    {
        

        template<typename T, typename A>
        vke_force_inline void* AllocMemory(A* pAllocator)
        {
            void* pMem = pAllocator->Allocate(sizeof(T));
            return pMem;
        }

        template<typename T, typename A>
        vke_force_inline T* AllocMemory(A* pAllocator, T** ppOut, uint32_t count)
        {
            T* pMem = reinterpret_cast<T*>(pAllocator->Allocate(sizeof(T), count));
            *ppOut = pMem;
            return pMem;
        }

        template<typename T, typename A>
        vke_force_inline void FreeMemory(A* pAllocator, T** ppPtr)
        {
            pAllocator->Free(sizeof(T), reinterpret_cast<void**>(ppPtr));
        }

        template<typename T, typename... _ARGS_>
        vke_force_inline T* CreateObject(void* pMemory, _ARGS_&&... args)
        {
            assert(pMemory);
            return new(pMemory)T(args...);
        }

        template<typename T, typename A, typename... _ARGS_>
        vke_force_inline Result CreateObject(A* pAllocator, T** ppPtrOut, _ARGS_&&... args)
        {
            void* pMem = pAllocator->Allocate(sizeof(T));
            if(pMem)
            {
                *ppPtrOut = new(pMem) T(args...);
                return VKE_OK;
            }
            *ppPtrOut = nullptr;
            return VKE_FAIL;
        }


        template<typename T, typename A>
        vke_force_inline void DestroyObject(A* pAllocator, T** ppPtrOut)
        {
            assert(ppPtrOut);
            if( ( *ppPtrOut ) )
            {
                ( *ppPtrOut )->~T();
                pAllocator->Free(sizeof(T), reinterpret_cast< void** >( ppPtrOut ));
            }
        }

        template<typename T>
        static vke_inline void Set(T* pData, const T& tValue, const uint32_t count = 1)
        {
            memset(pData, tValue, sizeof(T) * count);
        }

        template<typename T>
        static vke_inline void Zero(T* pData, const uint32_t count = 1)
        {
            memset(pData, 0, sizeof(T) * count);
        }

        static vke_force_inline
        void Copy(void* pDst, const size_t dstSize, const void* pSrc, const size_t bytesToCopy)
        {
            assert( pDst && "pDst MUST NOT BE NULL" );
#if _MSC_VER
            memcpy_s(pDst, dstSize, pSrc, bytesToCopy);
#else
            memcpy(pDst, pSrc, bytesToCopy);
#endif
        }

        template<typename T, uint32_t count = 1> vke_force_inline
        void Copy(T* pDst, const T* pSrc)
        {
            const auto size = sizeof( T ) * count;
            Copy( pDst, size, pSrc, size );
        }

    } // Memory
} // vke