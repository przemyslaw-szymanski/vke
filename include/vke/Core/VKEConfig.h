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
        };
    };
} // VKE