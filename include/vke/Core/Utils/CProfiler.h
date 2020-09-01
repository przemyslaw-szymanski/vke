#pragma once

#include "Core/Utils/CTimer.h"
#include "Core/Utils/CLogger.h"

namespace VKE
{
    namespace Utils
    {
        struct VKE_API SSimpleProfiler
        {
            CTimer      Timer;
            cstr_t      pFuncName = "";
            cstr_t      pFileName = "";
            uint32_t    line = 0;

            SSimpleProfiler( cstr_t pFile, cstr_t pFunc, uint32_t line ) :
                pFileName{ pFile }
                , pFuncName{ pFunc }
                , line{ line }
            {
                Timer.Start();
            }

            ~SSimpleProfiler()
            {
                const auto elapsed = Timer.GetElapsedTime();
                VKE_LOG( "Profile: " << pFileName << "/" << pFuncName << "/" << line << ": " << elapsed << " us" );
            }
        };
    } // Utils
} // VKE

#define VKE_PROFILE_SIMPLE() VKE::Utils::SSimpleProfiler _VkeProfiler( __FILE__, __FUNCTION__, __LINE__ )
#define VKE_PROFILE_SIMPLE2(_name) VKE::Utils::SSimpleProfiler _VkeProfiler( __FILE__, (_name), __LINE__ )