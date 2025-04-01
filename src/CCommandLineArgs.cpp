#include "CCommandLineArgs.h"

namespace VKE
{
    void CCommandLineArgs::Parse()
    {
        vke_string CmdLine = Platform::GetCmdLine();
        
        std::regex reInt( "(--([a-zA-Z]+)=([0-9]+))", std::regex_constants::ECMAScript );
        auto itr = std::sregex_iterator( CmdLine.begin(), CmdLine.end(), reInt );
        for (; itr != std::sregex_iterator(); ++itr)
        {
            auto Match = *itr;
            auto a = Match[2];
            auto b = Match[3];
            m_mInts[ a.str() ] = atoi( b.str().data() );
        }
    }
} // VKE