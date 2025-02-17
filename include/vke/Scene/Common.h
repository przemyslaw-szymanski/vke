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
    namespace RenderSystem
    {
        class CDeviceContext;
    }

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

        VKE_DECLARE_HANDLE( Light );

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
            RenderSystem::CommandBufferPtr  pCommandBuffer;
            ExtentF32                       Size;
            RenderSystem::SFrameGraphDesc2   FrameGraphDesc;
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
            //using NameString = Utils::TCString< char >;
            using StringArray = Utils::TCDynamicArray< ResourceName >;
            using String2DArray = Utils::TCDynamicArray< StringArray >;
            using Uint8Array = Utils::TCDynamicArray< uint8_t, 16 >;

            struct STextureInfo
            {
                ResourceName  Heightmap;
                ResourceName  HeightmapNormal;
                StringArray vSplatmaps;
            };

            using TextureInfoArray = Utils::TCDynamicArray< STextureInfo, 1 >;

            struct STextureDesc
            {
                TextureInfoArray vTextures;
                cstr_t          pLowResFileName = nullptr;
                cstr_t          pLowResNormalFileName = nullptr;
            };

            using DDIRenderPassArray = Utils::TCDynamicArray < RenderSystem::DDIRenderPass >;
            using RenderPassArray = Utils::TCDynamicArray< RenderSystem::RenderPassHandle >;
            /// Terrain size. This value will be resized to nearest pow(2) as a terrain is a quadtree
            /// containting pow(2) sized nodes.
            uint32_t                size;
            /// Min and max tile sizes (width/depth). Min indicates the smallest (highest LOD) tile while Max is the root.
            /// A terrain is a grid of roots (max tile sizes).
            /// Min and max will be resized to nearest pow(2).
            ExtentU16               TileSize = {32, 2048};
            /// Min and max height for a terrain.
            ExtentF32               Height = {0, 100};
            Math::CVector3          vecCenter = Math::CVector3::ZERO;
            float                   vertexDistance = 1.0f; // vertex distance (in units) in a highest lod tile
            float                   maxViewDistance = 1000.0f;
            float                   lodTreshold = 5.0f;
            uint32_t                maxVisibleTiles = UINT32_MAX;
            DDIRenderPassArray      vDDIRenderPasses;
            RenderPassArray         vRenderPasses;
            STerrainRendererDesc    Renderer;
            TextureInfoArray        vTileTextures;
            ImageSize               HeightmapOffset = { 0, 0 };
            struct
            {
                /// min, max factors. 0 means tesselation is disabled
                ExtentU16 Factors = { 0, 0 }; 
                /// <summary>
                /// maxDistance > 0 enables tesselation quality based on distance to a patch.
                /// maxDistance indicates what is max distance to calculate tessellation level of details for.
                /// Every patch with distance > maxDistance will use Factors.min as tessellation level.
                /// </summary>
                uint16_t maxDistance = 0;
                /// <summary>
                /// 
                /// </summary>
                float lodReductionFactor = 0.12f;
                /// <summary>
                /// 
                /// </summary>
                float lodReductionSpeed = 2;
                /// <summary>
                /// 
                /// </summary>
                bool flatSurfaceReduction = true;
                bool quadMode = true; // quad or triangle, 4 or 3 control points
            } Tesselation;
            /// Maximum LOD count for a terrain. This value can be recalculated to fit to TileSize.min.
            /// Note that there can't be smaller node/LOD than TileSize.min.
            /// 0 to auto calculation
            uint8_t lodCount = 0;
            bool distanceSort = false;
        };

        struct STerrainUpdateBindingData
        {
            uint32_t    index; // binding index
            RenderSystem::TextureViewHandle     hHeightmap = INVALID_HANDLE;
            RenderSystem::TextureViewHandle     hHeightmapNormal = INVALID_HANDLE;
            RenderSystem::SamplerHandle hBilinearSampler = INVALID_HANDLE;
        };

        struct LightTypes
        {
            enum TYPE
            {
                DIRECTIONAL,
                SPOT,
                OMNI,
                _MAX_COUNT
            };
        };
        using LIGHT_TYPE = LightTypes::TYPE;

        struct SLightDesc
        {
            Math::CVector3 vecPosition = Math::CVector3::ZERO;
            Math::CVector3 vecDirection = Math::CVector3::ZERO;
            RenderSystem::SColor Color = { 1, 1,1, 1 };
            float attenuation = 1;
            float radius = 1;
            LIGHT_TYPE type = LightTypes::DIRECTIONAL;
            ResourceName Name;
        };


        struct SUpdateSceneInfo
        {
            RenderSystem::CommandBufferPtr pCommandBuffer;
        };

        struct SCameraDesc
        {
            Math::CVector3 vecPosition = Math::CVector3::ZERO;
            Math::CVector3 vecLookAt = Math::CVector3::ZERO;
            Math::CVector3 vecUp = Math::CVector3::Y;
            Math::CVector3 vecRight = Math::CVector3::X;
            ExtentF32 Viewport = {800, 600};
            ExtentF32 ClipPlanes = {1.0f, 1000.0f};
            ResourceName Name;
        };

    } // Scene
} // VKE