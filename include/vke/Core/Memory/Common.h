#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/CLogger.h"

namespace VKE
{
    struct SAllocateMemoryInfo
    {
        uint32_t    size;
        uint32_t    alignment;
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