#pragma once

#include "Core/VKECommon.h"
#include "Core/Platform/CPlatform.h"

#include <regex>

namespace VKE
{
    class VKE_API CCommandLineArgs
    {
      public:
          template<typename T>
          using ArgType = std::optional< T >;
          template<typename T>
          using MapType = vke_hash_map< vke_string, T >;
          using BoolMap = MapType< ArgType<bool > >;
          using IntMap = MapType< ArgType<int32_t> >;

      public:

          void Parse();

          template<typename T>
          const ArgType<T>&    GetArg(std::string_view);

          template<typename T>
          bool GetArg( std::string_view, T, T* );

          template<typename T>
          T GetArg(std::string_view name, T defVal)
          {
              T ret;
              GetArg( name, defVal, &ret );
              return ret;
          }

     protected:

         template<typename T1, typename T2>
         bool _GetArg( const T1&, std::string_view, T2, T2* );
          
     protected:

        BoolMap m_mBools;
        IntMap m_mInts;
    };

    template<typename T1, typename T2>
    bool CCommandLineArgs::_GetArg(const T1& mMap, std::string_view name,
        T2 defValue, T2* pOut)
    {
        const auto& Itr = mMap.find( name.data() );
        bool        ret = ( Itr != mMap.end() );
        *pOut           = ret ? Itr->second.value() : defValue;
        return ret;
    }

    template<typename T>
    const CCommandLineArgs::ArgType<T>& CCommandLineArgs::GetArg(
        std::string_view name)
    {
        static_assert( std::is_same_v<T, bool> || std::is_same_v<T, int32_t>,
                       "Invalid argument type. Supported types are: bool, int32_t" );
        if constexpr (std::is_same_v<T, bool>)
        {
            return m_mBools[ name.data() ];
        }
        else if constexpr (std::is_same_v<T, int32_t>)
        {
            return m_mInts[ name.data() ];
        }
    }

    template<typename T>
    bool CCommandLineArgs::GetArg(std::string_view name, T defValue, T* pOut)
    {
        bool ret = false;
        static_assert(
            std::is_same_v<T, bool> || std::is_same_v<T, int32_t>,
            "Invalid argument type. Supported types are: bool, int32_t" );
        if constexpr( std::is_same_v<T, bool> )
        {
            ret = _GetArg( m_mBools, name, defValue, pOut );
        }
        else if constexpr( std::is_same_v<T, int32_t> )
        {
            ret = _GetArg( m_mInts, name, defValue, pOut );
        }

        return ret;
    }


} // VKE