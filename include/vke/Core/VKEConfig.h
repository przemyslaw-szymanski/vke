#pragma once

#include "VKEPreprocessor.h"
#include "VKETypes.h"

namespace VKE
{
    struct Config
    {
        static const uint32_t BYTE = 1;
        static const uint32_t KILOBYTE = BYTE       * 1024;
        static const uint32_t MEGABYTE = KILOBYTE   * 1024;
        static const uint32_t GIGABYTE = MEGABYTE   * 1024;

        static const uint32_t MAX_BACK_BUFFER_COUNT = 4;
        static const uint32_t DEFAULT_GPU_ACCESS_MEMORY_POOL_SIZE = MEGABYTE * 10;
        static const uint32_t DEFAULT_CPU_ACCESS_MEMORY_POOL_SIZE = MEGABYTE * 10;

        struct Resource
        {
            struct Texture
            {
                static const uint32_t MAX_VIEW_PER_TEXTURE = 4;
                static const uint32_t DEFAULT_COUNT = 1024;
            };

            struct TextureView
            {

            };

            struct RenderTarget
            {

            };

            struct Image
            {

            };

            struct File
            {
                static const uint32_t DEFAULT_COUNT = 1024;
            };

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
        };
    };
} // VKE