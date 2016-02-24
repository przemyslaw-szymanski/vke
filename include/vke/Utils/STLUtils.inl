#ifndef __VKE_ERROR_HANDLING_H__
#define __VKE_ERROR_HANDLING_H__

#include <cstdint>

#include "Preprocessor.h"

namespace VKE
{
    using Result = int32_t;

    struct Results
    {
        enum ERROR
        {
            OK,
            FAIL
        };
    };

    static const Result VKE_OK = Results::OK;
    static const Result VKE_FAIL = Results::FAIL;

} // VKE

#endif // __VKE_ERROR_HANDLING_H__