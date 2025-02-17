#pragma once

#include "Core/VKECommon.h"
#include "CDefaultAllocator.h"

namespace VKE
{
    namespace Memory
    {

        template<typename T>
        vke_force_inline T CalcAlignedSize(const T& size, const T& alignment)
        {
            T ret = size;
            T remainder = size % alignment;
            if( remainder > 0 )
            {
                ret = size + alignment - remainder;
            }

            return ret;
        }

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
            *ppPtr = nullptr;
        }

        template<typename T, typename... _ARGS_>
        vke_force_inline T* CreateObject(void* pMemory, _ARGS_&&... args)
        {
            assert(pMemory);
            return ::new(pMemory)T(args...);
        }

        template<typename T, typename A, typename... _ARGS_>
        vke_force_inline Result CreateObject(A* pAllocator, T** ppPtrOut, _ARGS_&&... args)
        {
            void* pMem = pAllocator->Allocate(sizeof(T));
            if(pMem)
            {
                *ppPtrOut = ::new(pMem) T(args...);
                return VKE_OK;
            }
            *ppPtrOut = nullptr;
            return VKE_FAIL;
        }

        template<typename T, typename A, typename... _ARGS_>
        vke_force_inline Result CreateObjects(A* pAllocator, T** ppPtrOut, uint32_t count, _ARGS_&&... args)
        {
            T* pMem = reinterpret_cast<T*>( pAllocator->Allocate( sizeof(T), count ) );
            if (pMem)
            {
                for (uint32_t i = count; i-- > 0;)
                {
                    ::new(&pMem[i])T(args...);
                }
                *ppPtrOut = pMem;
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
                *ppPtrOut = nullptr;
            }
        }

        template<typename T, typename A>
        vke_force_inline void DestroyObjects(A* pAllocator, T** ppPtrOut, const uint32_t& count)
        {
            assert(ppPtrOut && *ppPtrOut);
            if( (*ppPtrOut) )
            {
                T* pArray = *ppPtrOut;
                for (uint32_t i = count; i-- > 0;)
                {
                    pArray[i].~T();
                }
                 pAllocator->Free( sizeof( T ), count, (void**)ppPtrOut );
            }
        }

        template<typename T>
        static vke_inline void Set(T* pData, const T& tValue, const uint32_t& count = 1)
        {
            memset(pData, tValue, sizeof(T) * count);
        }

        template<typename T>
        static vke_inline void Zero(T* pData, const uint32_t count = 1)
        {
            memset(pData, 0, sizeof(T) * count);
        }

        static vke_force_inline
        bool Copy(void* pDst, const size_t& dstSize, const void* pSrc, const size_t& bytesToCopy)
        {
            assert( pDst && "pDst MUST NOT BE NULL" );
#if _MSC_VER
            return memcpy_s(pDst, dstSize, pSrc, bytesToCopy) == 0;
#else
            return memcpy(pDst, pSrc, bytesToCopy) != nullptr;
#endif
        }

        template<uint32_t count = 1, typename T> vke_force_inline
        void Copy(T* pDst, const T* pSrc)
        {
            const auto size = sizeof( T ) * count;
            Copy( pDst, size, pSrc, size );
        }

        template<typename T> vke_force_inline
        void Copy(T* pDst, const T* pSrc, const uint32_t count)
        {
            const auto size = sizeof(T) * count;
            Copy(pDst, size, pSrc, size);
        }

    } // Memory
} // vke