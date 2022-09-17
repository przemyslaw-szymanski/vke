#pragma once
#include "Core/VKECommon.h"
#include "Core/Utils/CLogger.h"
#include "Core/Utils/TCString.h"

namespace VKE
{
    namespace Core
    {
        struct ResourceStates
        {
            enum STATE : uint16_t
            {
                UNKNOWN = 0x0,
                ALLOCATED = VKE_BIT( 1 ),
                CREATED = VKE_BIT( 2 ),
                INITIALIZED = VKE_BIT( 3 ),
                LOADED = VKE_BIT( 4 ),
                PREPARED = VKE_BIT( 5 ),
                UNLOADED = VKE_BIT( 6 ),
                INVALIDATED = VKE_BIT( 7 ),
                INVALID = VKE_BIT( 8 ),
                DESTROYED = VKE_BIT( 9 ),
                LOCKED = VKE_BIT(10),
                PENDING = VKE_BIT(11),
                _MAX_COUNT = 12
            };
        };
        using RESOURCE_STATE = uint16_t;
        struct ResourceStages
        {
            enum STAGE
            {
                UNKNOWN = 0x0,
                CREATE = VKE_BIT( 1 ),
                INIT = VKE_BIT( 2 ),
                LOAD = VKE_BIT( 3 ),
                PREPARE = VKE_BIT( 4 ),
                UNLOAD = VKE_BIT( 5 ),
                INVALID = VKE_BIT( 6 ),
                _MAX_COUNT = 7,
                FULL_LOAD = CREATE | INIT | LOAD | PREPARE,
            };
        };
        using RESOURCE_STAGES = uint8_t;
    } // Core
} // VKE