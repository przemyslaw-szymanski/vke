#pragma once

#include "Core/VKECommon.h"

namespace VKE::Core
{
    struct CommandLineArgTypes
    {
        enum TYPE
        {
            BOOL,
            UINT,
            INT,
            FLOAT,
            STRING,
            _MAX_COUNT
        };
    };

    using COMMAND_LINE_ARG_TYPE = CommandLineArgTypes::TYPE;

    struct VKE_API SCommandLineArg
    {
        union
        {
            bool     boolValue;
            uint32_t uintValue;
            int32_t  intValue;
            float    floatValue;
            std::string stringValue;
        };

        COMMAND_LINE_ARG_TYPE type;
    };

    class VKE_API CCommandLine
    {
        using ArgMap = vke_hash_map< vke_string, SCommandLineArg >;
    public:

        template<typename T>
        Result Register(std::string_view name, T&& defaultValue)
        {
            auto Itr = m_mArgs.find( name.data() );
            if( Itr == m_mArgs.end() )
            {
                if constexpr( std::is_same_v< T, bool > )
                {
                    m_mArgs[ name.data() ] = SCommandLineArg{ .boolValue = defaultValue, .type = COMMAND_LINE_ARG_TYPE::BOOL };
                }
                else if constexpr( std::is_same_v< T, uint32_t > || std::is_integral_v< T > )
                {
                    m_mArgs[ name.data() ] = SCommandLineArg{ .uintValue = defaultValue, .type = COMMAND_LINE_ARG_TYPE::UINT };
                }
                else if constexpr( std::is_same_v< T, int32_t > || ( std::is_integral_v< T > && std::is_signed_v< T > ) )
                {
                    m_mArgs[ name.data() ] = SCommandLineArg{ .intValue = defaultValue, .type = COMMAND_LINE_ARG_TYPE::INT };
                }
                else if constexpr( std::is_same_v< T, float > )
                {
                    m_mArgs[ name.data() ] = SCommandLineArg{ .floatValue = defaultValue, .type = COMMAND_LINE_ARG_TYPE::FLOAT };
                }
                else if constexpr( std::is_same_v< T, vke_string > || std::is_same_v< T, cstr_t > || std::is_same_v< T, std::string_view > )
                {
                    m_mArgs[ name.data() ] = SCommandLineArg{ .stringValue = defaultValue, .type = COMMAND_LINE_ARG_TYPE::STRING };
                }
                else if constexpr( std::is_enum_v< T > )
                {
                    if constexpr( std::is_signed_v< std::underlying_type_t< T> > )
                    {
                        m_mArgs[ name.data() ] = SCommandLineArg{ .intValue = static_cast< int32_t >( defaultValue ), .type = COMMAND_LINE_ARG_TYPE::INT };
                    }
                    else
                    {
                        m_mArgs[ name.data() ] = SCommandLineArg{ .uintValue = static_cast< uint32_t >( defaultValue ), .type = COMMAND_LINE_ARG_TYPE::UINT };
                    }
                }
                return VKE_OK;
            }
            return VKE_FAIL;
        }

    protected:

        ArgMap m_mArgs;
    };
} // namespace VKE::Core