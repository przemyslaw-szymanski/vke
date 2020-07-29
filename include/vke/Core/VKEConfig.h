#pragma once

#include "VKEPreprocessor.h"
#include "VKETypes.h"

namespace VKE
{
#define VKE_CFG_LOG_PROGRESS 1

    namespace Config
    {
        static const uint32_t BYTE = 1;
        static const uint32_t KILOBYTE = BYTE       * 1024;
        static const uint32_t MEGABYTE = KILOBYTE   * 1024;
        static const uint32_t GIGABYTE = MEGABYTE   * 1024;

#define VKE_KILOBYTES(_num) (VKE::Config::KILOBYTE * (_num))
#define VKE_MEGABYTES(_num) (VKE::Config::MEGABYTE * (_num))

        static const uint32_t MAX_BACK_BUFFER_COUNT = 4;
        static const uint32_t DEFAULT_GPU_ACCESS_MEMORY_POOL_SIZE = MEGABYTE * 10;
        static const uint32_t DEFAULT_CPU_ACCESS_MEMORY_POOL_SIZE = MEGABYTE * 10;

        namespace Utils
        {
            struct String
            {
                static const uint32_t DEFAULT_ELEMENT_COUNT = 32;
            };
        }

        struct Resource
        {
            static const uint32_t MAX_NAME_LENGTH = 128;

            struct Image
            {

            };

            struct File
            {
                static const uint32_t DEFAULT_COUNT = 1024;
            };
        };
    } // Config
} // VKE