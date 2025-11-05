#pragma once

#include "Core/VKECommon.h"
#include "Core/Platform/CPlatform.h"

#include <regex>

namespace VKE
{
    class VKE_API CCommandLineArgs
    {
    public:
        struct SArg
        {
            union
            {
                bool     boolValue;
                uint32_t uintValue;
                int32_t  intValue;
                float    floatValue;
            };

            operator bool() const
            {
                return boolValue;
            }

            operator uint32_t() const
            {
                return uintValue;
            }

            operator int32_t() const
            {
                return intValue;
            }

            operator float() const
            {
                return floatValue;
            }
        };

        using ArgType = std::optional< SArg >;
        using MapType = vke_hash_map< vke_string, ArgType >;

    public:
        CCommandLineArgs();

        static CCommandLineArgs& GetInstance()
        {
            static CCommandLineArgs Instance;
            return Instance;
        }

        void Parse();

        const ArgType& GetArg( std::string_view name )
        {
            return m_Args[ name.data() ];
        }

    protected:
        template< typename T >
        void RegisterParam( std::string_view name, T defaultValue );

        template< class MapT, typename DataT >
        void RegisterParam( std::string_view name, DataT defaultValue, MapT* mMap );

        template< typename T >
        constexpr bool IsString()
        {
            return std::is_same_v< T, vke_string > || std::is_same_v< T, cstr_t > ||
                   std::is_same_v< T, std::string_view >;
        }

    protected:
        MapType m_Args;
    };

    template< typename T >
    const CCommandLineArgs::ArgType& GetCommandLineParam( std::string_view name )
    {
        return CCommandLineArgs::GetInstance().GetArg( name );
    }

    template< typename T >
    T GetCommandLineParam( std::string_view name, T defaultValue )
    {
        const auto& v = CCommandLineArgs::GetInstance().GetArg( name );
        return v.has_value() ? static_cast< T >( v.value() ) : defaultValue;
    }

} // namespace VKE