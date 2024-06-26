#ifndef __VKE_STL_UTILS_H__
#define __VKE_STL_UTILS_H__

#include "Core/VKETypes.h"

namespace VKE
{
    namespace Utils
    {
        namespace Map
        {
            template<typename _KEY_, typename _VALUE_>
            auto Insert(std::map<_KEY_, _VALUE_>& mMap, const _KEY_& tKey, const _VALUE_& tValue) ->
                decltype(std::map<_KEY_, _VALUE_>::iterator)
            {
                return mMap.insert(std::map<_KEY_, _VALUE_>::value_type(tKey, tValue)).first;
            }

            template<typename _KEY_, typename _VALUE_>
            auto Insert(std::unordered_map<_KEY_, _VALUE_>& mMap, const _KEY_& tKey, const _VALUE_& tValue) ->
                decltype(std::unordered_map<_KEY_, _VALUE_>::iterator)
            {
                return mMap.insert(std::unordered_map<_KEY_, _VALUE_>::value_type(tKey, tValue)).first;
            }

        } // Map

        class CDataInterpreter
        {
          public:
            CDataInterpreter( void* pPtr )
                : m_pPtr{ static_cast<uint8_t*>( pPtr ) }
            {
            }
            
            template<class T> T* GetPtr()
            {
                T* pRet = reinterpret_cast<T*>( m_pPtr );
                m_pPtr += sizeof( T );
                return pRet;
            }

            protected:
                uint8_t* m_pPtr;
        };

        template<typename... ArgsT> constexpr size_t GetArgumentCount()
        {
            return sizeof...( ArgsT );
        }
        template<typename... ArgsT> constexpr size_t GetArgumentTotalSize()
        {
            return ( sizeof( ArgsT ) + ... + 0 );
        }
        template<typename FirstArgT, typename... ArgsT>
        void* StoreArguments( void* pDstMemory, FirstArgT FirstArg, ArgsT&&... args )
        {
            void* pRet = nullptr;
            FirstArgT* pDst;
            if constexpr( std::is_trivially_copyable_v<FirstArgT> )
            {
                pDst = static_cast<FirstArgT*>( pDstMemory );
                // Memory::Copy<FirstArg>( pDst, &FirstArg );
                *pDst = std::move( FirstArg );
            }
            else
            {
                pDst = new( pDstMemory ) FirstArgT{ std::move( FirstArg ) };
            }
            constexpr auto size = sizeof( FirstArgT );
            pRet = ( uint8_t* )pDstMemory + size;
            if constexpr( GetArgumentCount<ArgsT...>() > 0 )
            {
                pRet = StoreArguments( pRet, std::forward<ArgsT>( args )... );
            }
            return pRet;
        }

        template<typename FirstArgT, typename... ArgsT>
        void* LoadArguments( void* pSrcMemory, FirstArgT** ppOut, ArgsT&&... args )
        {
            void* pRet = pSrcMemory;
            *ppOut = static_cast<FirstArgT*>( pSrcMemory );
            if constexpr( GetArgumentCount<ArgsT...>() > 0 )
            {
                void* pNext = ( uint8_t* )pSrcMemory + sizeof( FirstArgT );
                pRet = LoadArguments( pNext, std::forward<ArgsT>( args )... );
            }
            return pRet;
        }
        //template<typename FirstT, typename... ArgsT>
        //void* LoadArguments( void* pSrcMemory, FirstT& First, ArgsT&&... args )
        //{
        //    void* pRet = pSrcMemory;
        //    // FirstT& Data = *(FirstT*)( pSrcMemory );
        //    FirstT* pData = ( FirstT* )( pSrcMemory );
        //    First = *pData;
        //    //*pFirst = pData;
        //    if constexpr( GetArgumentCount<ArgsT...>() > 0 )
        //    {
        //        constexpr auto size = sizeof( FirstT );
        //        void* pNext = ( uint8_t* )pSrcMemory + size;
        //        pRet = LoadArguments( pNext, std::forward<ArgsT>( args )... );
        //    }
        //    return pRet;
        //}
    } // Utils
} // VKE

#endif // __VKE_STL_UTILS_H__