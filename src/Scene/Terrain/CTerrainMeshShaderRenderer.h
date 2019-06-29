#pragma once

#include "ITerrainRenderer.h"

namespace VKE
{
    namespace Scene
    {
        static cstr_t const TERRAIN_MESH_SHADING_RENDERER_NAME = "VKE_TERRAIN_MESH_SHADING_RENDERER";

        class CTerrainMeshShadingRenderer : public ITerrainRenderer
        {
        };
    } // Scene
} // VKE