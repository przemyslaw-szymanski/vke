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

    static constexpr Result VKE_OK   = Results::OK;
    static constexpr Result VKE_FAIL = Results::FAIL;

} // namespace VKE

#endif // __VKE_ERROR_HANDLING_H__