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
        _MAX_COUNT
    };

    using Result = Results;

    static const Result VKE_OK          = Results::OK;
    static const Result VKE_FAIL        = Results::FAIL;
    static const Result VKE_ENOMEMORY   = Results::NO_MEMORY;
    static const Result VKE_ENOTFOUND   = Results::NOT_FONUD;
    static const uint32_t VKE_TRUE      = 1;
    static const uint32_t VKE_FALSE     = 0;

} // VKE

#define VKE_PRINT(_msg) VKE_LOGGER_LOG_CONSOLE(_msg)

#define VKE_LOG(_msg) VKE_LOGGER_LOG(_msg)
#define VKE_LOG_ERR(_msg) VKE_LOGGER_LOG_ERROR(VKE_FAIL, _msg)
#define VKE_LOG_WRN(_msg) VKE_LOGGER_LOG_WARNING(_msg)
#define VKE_LOG_RET(_ret, _msg)  VKE_CODE( VKE_LOGGER_LOG_MSG( (_ret), _msg ); return (_ret); )
#define VKE_LOG_ERR_RET(_ret, _msg) VKE_CODE( VKE_LOGGER_LOG_ERROR( (_ret), _msg ); return (_ret); )
#define VKE_LOG_WRN_RET(_ret, _msg) VKE_CODE( VKE_LOGGER_LOG_WARNING( (_ret), _msg ); return (_ret); )

#endif // __VKE_ERROR_HANDLING_H__