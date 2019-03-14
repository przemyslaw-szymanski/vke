#ifndef __VKE_ERROR_HANDLING_H__
#define __VKE_ERROR_HANDLING_H__

#include <cstdint>

#include "VKEPreprocessor.h"

namespace VKE
{
    enum class Results : int32_t
    {
        OK,
        FAIL,
        NO_MEMORY,
        NOT_FONUD,
        NOT_READY,
        TIMEOUT,
        _MAX_COUNT
    };

    using Result = Results;

    static const Result VKE_OK          = Results::OK;
    static const Result VKE_FAIL        = Results::FAIL;
    static const Result VKE_ENOMEMORY   = Results::NO_MEMORY;
    static const Result VKE_ENOTFOUND   = Results::NOT_FONUD;
    static const Result VKE_ENOTREADY   = Results::NOT_READY;
    static const Result VKE_TIMEOUT     = Results::TIMEOUT;
    static const uint32_t VKE_TRUE      = 1;
    static const uint32_t VKE_FALSE     = 0;

} // VKE

#define VKE_PRINT(_msg) VKE_LOGGER_LOG_CONSOLE(_msg)



#endif // __VKE_ERROR_HANDLING_H__