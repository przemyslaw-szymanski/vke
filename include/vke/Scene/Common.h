#pragma once

#include "Core/VKECommon.h"
#include "Core/CObject.h"
#include "Core/Utils/CLogger.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Utils/TCDynamicArray.h"

#include "Core/Math/CVector.h"

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

        struct GraphSystems
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
        using GRAPH_SYSTEM = GraphSystems::SYSTEM;

        struct SOctreeDesc
        {
            Math::CVector3  vec3Center = Math::CVector3::ZERO;
            Math::CVector3  vec3MaxSize = { 1000.0f };
            Math::CVector3  vec3MinSize = { 1.0f };
            uint32_t        maxDepth = 5;
            float           extraSizePercent = 0.1f; // 0-1 range
        };

        struct SSceneDesc
        {
            ExtentF32       Size;
            void*           pPartitionDesc;
            GRAPH_SYSTEM    graphSystem = GraphSystems::NONE;
            SOctreeDesc     OctreeDesc;
        };

        union UObjectBits
        {
            UObjectBits() {}
            explicit UObjectBits( const bool& isVisible, const uint32_t& idx ) : visible{ isVisible }, index{ idx } {}

            struct
            {
                uint32_t     visible : 1;
                uint32_t     index : 20;
                uint32_t     reserved : 11;
            };
            uint32_t value;
        };
    } // Scene
} // VKE