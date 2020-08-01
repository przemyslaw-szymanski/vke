#ifndef __VKE_ERROR_HANDLING_H__
#define __VKE_ERROR_HANDLING_H__

#include <cstdint>

#include "VKEPreprocessor.h"

namespace VKE
{
    struct Results
    {
        enum RESULT : uint32_t
        {
            OK,
            FAIL,
            NO_MEMORY,
            NOT_FONUD,
            NOT_READY,
            TIMEOUT,
            DEVICE_LOST,
            OUT_OF_DATE,
            _MAX_COUNT
        };
    };
    using Result = Results::RESULT;

    static const Result VKE_OK          = Results::OK;
    static const Result VKE_FAIL        = Results::FAIL;
    static const Result VKE_ENOMEMORY   = Results::NO_MEMORY;
    static const Result VKE_ENOTFOUND   = Results::NOT_FONUD;
    static const Result VKE_ENOTREADY   = Results::NOT_READY;
    static const Result VKE_TIMEOUT     = Results::TIMEOUT;
    static const Result VKE_EDEVICELOST = Results::DEVICE_LOST;
    static const Result VKE_EOUTOFDATE  = Results::OUT_OF_DATE;
    static const uint32_t VKE_TRUE      = 1;
    static const uint32_t VKE_FALSE     = 0;

} // VKE

#define VKE_PRINT(_msg) VKE_LOGGER_LOG_CONSOLE(_msg)



#endif // __VKE_ERROR_HANDLING_H__