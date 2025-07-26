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

          static CCommandLineArgs& GetInstance()
          {
              static CCommandLineArgs Instance;
              return Instance;
          }

          void Parse();

          template<typename T>
          const ArgType<T>&    GetArg(std::string_view);
          
     protected:

        BoolMap m_mBools;
        IntMap m_mInts;
    };

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

    template<typename T> const CCommandLineArgs::ArgType<T>& GetCommandLineParam( std::string_view name )
    {
        return CCommandLineArgs::GetInstance().GetArg<T>( name );
    }

} // VKE