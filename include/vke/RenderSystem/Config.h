#pragma once

#include "Common.h"
#include "Core/VKEConfig.h"

namespace VKE
{
    namespace Config
    {
        namespace RenderSystem
        {
            struct Shader
            {
                static const uint32_t DEFAULT_SHADER_BINARY_SIZE = 256 * KILOBYTE;
                static const uint32_t MAX_VERTEX_SHADER_COUNT = 1024;
                static const uint32_t MAX_TESSELATION_HULL_SHADER_COUNT = 1024;
                static const uint32_t MAX_TESSELATION_DOMAIN_SHADER_COUNT = 1024;
                static const uint32_t MAX_GEOMETRY_SHADER_COUNT = 1024;
                static const uint32_t MAX_PIXEL_SHADER_COUNT = 1024;
                static const uint32_t MAX_COMPUTE_SHADER_COUNT = 1024;
                static const uint32_t MAX_SHADER_PROGRAM_COUNT = 1024;
            };

            struct Pipeline
            {
                static const uint32_t MAX_PIPELINE_COUNT = 2048; // Max num of pipelines that could be allocated
                static const uint32_t MAX_VERTEX_ATTRIBUTE_COUNT = 16; // Max attributes per vertex
                static const uint32_t MAX_BLEND_STATE_COUNT = 8; // Max blend state count per one PSO
                static const uint32_t MAX_VERTEX_INPUT_BINDING_COUNT = 16; // Max num of binding for vertex
            };
        } // RenderSystem
    } // Config
} // VKE