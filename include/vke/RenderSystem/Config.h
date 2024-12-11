#pragma once

#include "Common.h"
#include "Core/VKEConfig.h"

namespace VKE
{
#if VKE_VULKAN_RENDER_SYSTEM
#   define VKE_SHADER_BIN_EXT "spv"
#   define VKE_PSO_CACHE_EXT "pso"
#elif VKE_D3D12_RENDERER
#   define VKE_SHADER_BIN_EXT "dxil"
#   define VKE_PSO_CACHE_EXT "pso"
#endif

    namespace Config
    {
        namespace RenderSystem
        {
            struct Shader
            {
                static constexpr cstr_t DEFAULT_CACHE_DIRECTORY = "Cache/Shaders";
                static constexpr cstr_t CACHE_FILE_EXT = VKE_SHADER_BIN_EXT;
                static const uint32_t DEFAULT_SHADER_BINARY_SIZE = 256 * KILOBYTE;
               /* static const uint32_t MAX_VERTEX_SHADER_COUNT = 1024;
                static const uint32_t MAX_TESSELATION_HULL_SHADER_COUNT = 1024;
                static const uint32_t MAX_TESSELATION_DOMAIN_SHADER_COUNT = 1024;
                static const uint32_t MAX_GEOMETRY_SHADER_COUNT = 1024;
                static const uint32_t MAX_PIXEL_SHADER_COUNT = 1024;
                static const uint32_t MAX_COMPUTE_SHADER_COUNT = 1024;
                static const uint32_t MAX_SHADER_PROGRAM_COUNT = 1024;*/
                static const uint32_t MAX_SHADER_COUNT_PER_TYPE = 1024;
                static const uint32_t MAX_INCLUDE_PATH_LENGTH = 64;
                static const uint32_t MAX_PREPROCESSOR_DIRECTIVE_LENGTH = 32;
                static const uint32_t MAX_ENTRY_POINT_NAME_LENGTH = 32;
            };

            struct Pipeline
            {
                static constexpr cstr_t DEFAULT_CACHE_DIRECTORY = "Cache/PSO";
                static constexpr cstr_t CACHE_FILE_EXT = VKE_PSO_CACHE_EXT;
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
                static const uint32_t MAX_COUNT = 128;
            };

            struct Sampler
            {
                static const uint32_t MAX_COUNT = 1024;
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
                
                static const uint32_t STAGING_BUFFER_PAGE_SIZE = VKE_KILOBYTES( 64 );
                /// <summary>
                /// Current implementation of StagingBufferManager has 12 bits max count of available page count
                /// per staging buffer. This size indicate max size of a single allocation.
                /// E.g. there is no way to upload more than STAGING_BUFFER_PAGE_SIZE * 4095 into a single buffer.
                /// To increase this max buffer capacity STAGING_BUFFER_PAGE_SIZE must be increased.
                /// </summary>
                static const uint32_t STAGING_BUFFER_SIZE = ( STAGING_BUFFER_PAGE_SIZE * 4095 );
                static const uint32_t STAGING_BUFFER_CHUNK_SIZE = VKE_MEGABYTES( 4 );
                
            };

            struct CommandBuffer
            {
                static const uint32_t DEFAULT_COUNT_IN_POOL = 32;
            };

            struct Bindings
            {
                static const uint32_t DEFAULT_COUNT_IN_POOL = 1024;
            };

            struct SwapChain
            {
                static const uint32_t MAX_BACK_BUFFER_COUNT = 4; // tripple buffering
            };

            struct FrameBudget
            {
                static const uint32_t MAX_TEXTURE_LOAD_COUNT = 32;
                static const uint32_t MAX_BUFFER_LOAD_COUNT = 32;
                static const uint32_t MAX_SHADER_COMPILATION_COUNT = 32;
                static const uint32_t MAX_PIPELINE_COMPILATION_COUNT = 32;
                static const uint32_t MAX_TEXTURE_LOAD_STAGING_BUFFER_MEMORY_SIZE = 128;
                static const uint32_t MAX_BUFFER_LOAD_STAGING_BUFFER_MEMORY_SIZE = 32;
            };

        } // RenderSystem
    } // Config
} // VKE