#include "CCommandLineArgs.h"

namespace VKE
{

    CCommandLineArgs::CCommandLineArgs()
    {
    }

    template<typename DataT, class MapT>
    void Parse( const std::regex& Reg, const vke_string& str, MapT* pMap )
    {
        MapT& mMap = *pMap;
        auto itr = std::sregex_iterator( str.begin(), str.end(), Reg );
        for( ; itr != std::sregex_iterator(); ++itr )
        {
            auto Match         = *itr;
            auto a             = Match[ 2 ];
            std::string b;
            if( Match.size() > 2 )
            {
                b = Match[ 3 ];
            }
            CCommandLineArgs::SArg Arg;
            if constexpr( std::is_same_v<DataT, int> )
            {
                Arg.intValue    = atoi( b.data() );
                mMap[ a.str() ] = Arg;
            }
            else if constexpr (std::is_same_v<DataT, float>)
            {
                Arg.floatValue  = ( float )atof( b.data() );
                mMap[ a.str() ] = Arg;
            }
            else if constexpr(std::is_same_v<DataT, bool>)
            {
                Arg.boolValue   = true;
                mMap[ a.str() ] = Arg;
            }
        }
    }
    void CCommandLineArgs::Parse()
    {
        vke_string CmdLine = Platform::GetCmdLine();
        std::regex reInt( "(--([a-zA-Z_\\-\\.]+)=(-?[0-9]+))", std::regex_constants::ECMAScript );
        std::regex reBool( "(--([a-zA-Z_\\-\\.]+))", std::regex_constants::ECMAScript );
        std::regex reFloat( "(--([a-zA-Z_\\-\\.]+)=(-?[0-9]+\\.?[0-9]+))", std::regex_constants::ECMAScript );
        std::regex reStr( "(--([a-zA-Z_\\-\\.]+)=(.*))", std::regex_constants::ECMAScript );
        {
            auto itr = std::sregex_iterator( CmdLine.begin(), CmdLine.end(), reInt );
            for( ; itr != std::sregex_iterator(); ++itr )
            {
                auto Match         = *itr;
                auto a             = Match[ 2 ];
                auto b             = Match[ 3 ];
                //m_mInts[ a.str() ] = atoi( b.str().data() );
            }
        }

        // bool is buggy. It takes everything with '=' as well and sets as bool=true
        VKE::Parse<bool>( reBool, CmdLine, &m_Args );
        VKE::Parse<int>( reInt, CmdLine, &m_Args );
        VKE::Parse<float>( reFloat, CmdLine, &m_Args );
        
    }
} // VKE