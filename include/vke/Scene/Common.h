#pragma once

#include "Core/VKECommon.h"
#include "Core/CObject.h"
#include "Core/Utils/CLogger.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace Scene
    {
#if VKE_SCENE_DEBUG
#   define VKE_SCENE_DEBUG_NAME    vke_string  dbgName
#   define VKE_SCENE_DEBUG_NAME_SET(_obj, _text) do{ (_obj).dbgName = (_text) } while(0,0)
#   define VKE_SCENE_DEBUG_NAME_GET(_obj)   ((_obj).dbgName)
#else
#   define VKE_SCENE_DEBUG_NAME
#endif // VKE_SCENE_DEBUG

        struct PartitionSystems
        {
            enum SYSTEM
            {
                NONE,
                OCTREE,
                QUADTREE,
                BVH,
                _MAX_COUNT
            };
        };
        using PARTITION_SYSTEM = PartitionSystems::SYSTEM;

        struct SOctreeDesc
        {
            ExtentF32   MaxSize;
            ExtentF32   MinSize;
            uint32_t    depth;
            float       extraSize = 0.0f;
        };

        struct SSceneDesc
        {
            ExtentF32           Size;
            void*               pPartitionDesc;
            PARTITION_SYSTEM    partitionSystem;
        };
    } // Scene
} // VKE