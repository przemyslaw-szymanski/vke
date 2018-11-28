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
                static const uint32_t MAX_INCLUDE_PATH_LENGTH = 64;
                static const uint32_t MAX_PREPROCESSOR_DIRECTIVE_LENGTH = 32;
            };

            struct Pipeline
            {
                static const uint32_t MAX_PIPELINE_COUNT = 2048; // Max num of pipelines that could be allocated
                static const uint32_t MAX_PIPELINE_LAYOUT_COUNT = 4096; // Max num of pipeline layout that could be allocated
                static const uint32_t MAX_VERTEX_ATTRIBUTE_COUNT = 16; // Max attributes per vertex
                static const uint32_t MAX_BLEND_STATE_COUNT = 8; // Max blend state count per one PSO
                static const uint32_t MAX_VERTEX_INPUT_BINDING_COUNT = 16; // Max num of binding for vertex
                static const uint32_t MAX_VIEWPORT_COUNT = 8;
                static const uint32_t MAX_SCISSOR_COUNT = 8;
                static const uint32_t MAX_DESCRIPTOR_BINDING_COUNT = 32;
                static const uint32_t MAX_DESCRIPTOR_SET_COUNT = 2048; // Default count per each descriptor set type
                static const uint32_t MAX_DESCRIPTOR_SET_LAYOUT_COUNT = 4096;
                static const uint32_t MAX_PIPELINE_LAYOUT_DESCRIPTOR_SET_COUNT = 16;
                static const uint32_t MAX_DESCRIPTOR_TYPE_COUNT = 128;
            };

            struct Texture
            {
                static const uint32_t MAX_COUNT = 1024;               
                static const uint32_t MAX_VIEW_COUNT = 4;
            };

            struct TextureView
            {
                static const uint32_t MAX_COUNT = 1024;
            };

            struct RenderTarget
            {

            };

            struct Buffer
            {
                static const uint32_t MAX_VERTEX_BUFFER_COUNT = 4096 * 2;
                static const uint32_t MAX_VERTEX_BUFFER_VIEW_COUNT = 4096 * 2;
                static const uint32_t MAX_INDEX_BUFFER_COUNT = 4096 * 2;
                static const uint32_t MAX_INDEX_BUFFER_VIEW_COUNT = 4096 * 2;
                static const uint32_t MAX_UNIFORM_BUFFER_COUNT = 1024;
                static const uint32_t MAX_UNIFORM_BUFFER_VIEW_COUNT = 1024;
                static const uint32_t MAX_BUFFER_COUNT =
                    MAX_VERTEX_BUFFER_COUNT +
                    MAX_VERTEX_BUFFER_VIEW_COUNT +
                    MAX_INDEX_BUFFER_COUNT +
                    MAX_INDEX_BUFFER_VIEW_COUNT +
                    MAX_UNIFORM_BUFFER_COUNT +
                    MAX_UNIFORM_BUFFER_VIEW_COUNT;
            };

        } // RenderSystem
    } // Config
} // VKE