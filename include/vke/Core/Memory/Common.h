#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/CLogger.h"
#include "Core/Utils/TCString.h"

namespace VKE
{
    
    struct VKE_API SAllocateMemoryInfo
    {
        struct SDebugInfo
        {
            vke_string Name;
        };
        uint32_t    size;
        uint32_t    alignment;
#if VKE_MEMORY_DEBUG
        SDebugInfo  Debug;
#endif
    };

    struct SMemoryPoolManagerDesc
    {
        uint16_t    poolTypeCount = 1;
        uint16_t    indexTypeCount = 1;
        uint32_t    defaultAllocationCount = 10000;
    };

    struct SMemoryAllocationInfo
    {
        handle_t    hMemory;
        uint32_t    offset;
        uint32_t    size;
    };
} // VKE