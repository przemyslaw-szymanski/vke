#pragma once

#include "Core/VKECommon.h"
#include "Scene/Config.h"

#include "Core/CObject.h"
#include "Core/Utils/CLogger.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Utils/TCDynamicArray.h"

#include "Core/Math/CVector.h"

#include "RenderSystem/IFrameGraph.h"

namespace VKE
{
    namespace Scene
    {
#if VKE_SCENE_DEBUG
#   define VKE_SCENE_DEBUG_NAME_TYPE vke_string
#   define VKE_SCNEE_DEBUG_VAR_NAME dbgName
#   define VKE_SCENE_SET_DEBUG_NAME(_obj, _text) do{ (_obj).VKE_SCNEE_DEBUG_VAR_NAME = (_text); } while(0,0)
#   define VKE_SCENE_GET_DEBUG_NAME(_obj)   ((_obj).VKE_SCNEE_DEBUG_VAR_NAME)
#else
#   define VKE_SCENE_DEBUG_NAME_TYPE
#   define VKE_SCNEE_DEBUG_VAR_NAME
#   define VKE_SCENE_SET_DEBUG_NAME(_obj, _text)
#   define VKE_SCENE_GET_DEBUG_NAME(_obj) "" 
#endif // VKE_SCENE_DEBUG

#define VKE_DECL_SCENE_OBJECT_DEBUG() \
    protected: VKE_SCENE_DEBUG_NAME_TYPE VKE_SCNEE_DEBUG_VAR_NAME

        static cstr_t     SCENE_GRAPH_OCTREE_NAME     = "VKE_OCTREE";
        static cstr_t     SCENE_GRAPH_QUADTREE_NAME   = "VKE_QUADTREE";
        static cstr_t     SCENE_GRAPH_BVH_NAME        = "VKE_BVH";

        using NodeHandleType = uint32_t;

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

        struct SSceneGraphDesc
        {
            cstr_t  pName = SCENE_GRAPH_OCTREE_NAME;
            void*   pDesc = nullptr;
        };

        struct SSceneDesc
        {
            ExtentF32                       Size;
            RenderSystem::SFrameGraphDesc   FrameGraphDesc;
            SSceneGraphDesc                 SceneGraphDesc;
        };

        using UObjectHandle = RenderSystem::UObjectHandle;
        using UObjectBits = RenderSystem::UObjectBits;

        struct SDrawcallDesc
        {
            RenderSystem::DRAWCALL_TYPE type = RenderSystem::DrawcallTypes::STATIC_OPAQUE;
        };

        struct STerrainVertexFetchRendererDesc
        {

        };

        struct STerrainRendererDesc
        {
            cstr_t      pName = nullptr;
            void*       pDesc;
        };

        struct STerrainDesc
        {
            using RenderPassArray = Utils::TCDynamicArray < RenderSystem::DDIRenderPass >;
            uint32_t                size;
            ExtentF32               Height;
            Math::CVector3          vecCenter = Math::CVector3::ZERO;
            uint16_t                tileRowVertexCount = 33;
            float                   vertexDistance = 1.0f;
            uint8_t                 lodCount = 4;
            float                   maxViewDistance = 1000.0f;
            RenderPassArray         vDDIRenderPasses;
            STerrainRendererDesc    Renderer;
        };

    } // Scene
} // VKE