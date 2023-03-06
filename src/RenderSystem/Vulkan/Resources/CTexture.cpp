#include "RenderSystem/Resources/CTexture.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/Vulkan/Managers/CTextureManager.h"
#include "RenderSystem/CDeviceContext.h"

namespace VKE
{
    namespace RenderSystem
    {
        TEXTURE_ASPECT CTexture::ConvertFormatToAspect( const TEXTURE_FORMAT format )
        {
            TEXTURE_ASPECT ret;

            switch( format )
            {
                case Formats::D32_SFLOAT:
                case Formats::X8_D24_UNORM_PACK32:
                case Formats::D16_UNORM:
                {
                    ret = TextureAspects::DEPTH;
                }
                break;
                case Formats::D32_SFLOAT_S8_UINT:
                case Formats::D24_UNORM_S8_UINT:
                case Formats::D16_UNORM_S8_UINT:
                {
                    ret = TextureAspects::DEPTH_STENCIL;
                }
                break;
                default:
                {
                    ret = TextureAspects::COLOR;
                }
                break;
            };
            return ret;
        }

        MEMORY_ACCESS_TYPE CTexture::ConvertStateToSrcMemoryAccess( const TEXTURE_STATE currState,
            const TEXTURE_STATE newState )
        {
            static const MEMORY_ACCESS_TYPE aaTypes[TextureStates::_MAX_COUNT][TextureStates::_MAX_COUNT] =
            {
                // From undefined
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ, // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE, // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From general
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From color render target
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From depth render target
                {
                    MemoryAccessTypes::SHADER_READ,                      // undefined
                    MemoryAccessTypes::SHADER_READ,                      // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ,         // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE,                     // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ                   // present
                },
                // From stencil render target
                {
                    MemoryAccessTypes::SHADER_READ,                      // undefined
                    MemoryAccessTypes::SHADER_READ,                      // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ,         // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE,                     // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ                   // present
                },
                // From depth stencil render target
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From dpeth buffer
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From stencil buffer
                {
                    MemoryAccessTypes::SHADER_READ,                      // undefined
                    MemoryAccessTypes::SHADER_READ,                      // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ,         // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE,                     // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ                   // present
                },
                // From dpeth stencil buffer
                {
                    MemoryAccessTypes::SHADER_READ,                      // undefined
                    MemoryAccessTypes::SHADER_READ,                      // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ,         // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE,                     // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ                   // present
                },
                // From shader read
                {
                    MemoryAccessTypes::SHADER_WRITE, // undefined
                    MemoryAccessTypes::SHADER_WRITE, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_WRITE, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ,  // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ,  // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_WRITE, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                  // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                  // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,                // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,               // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From transfer src
                {
                    MemoryAccessTypes::GPU_MEMORY_READ,                  // to undefined
                    MemoryAccessTypes::GPU_MEMORY_READ,                  // to general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ,         // to color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // to depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // to stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // to depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_READ,                  // to depth buffer
                    MemoryAccessTypes::GPU_MEMORY_READ,                  // to stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_READ,                  // to depth stencil buffer
                    MemoryAccessTypes::SHADER_READ,                      // to shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // to transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // to transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ                   // to present
                },
                // From transfer dst
                {
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // to undefined
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // to general
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // to color render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // to depth render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // to stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // to depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // to depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // to stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // to depth stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // to shader read
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // to transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // to transfer dst
                    MemoryAccessTypes::GPU_MEMORY_WRITE                  // to present
                },
                // From present
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                }
            };
            static const MEMORY_ACCESS_TYPE aTypes[ TextureStates::_MAX_COUNT ] =
            {
                MemoryAccessTypes::GPU_MEMORY_WRITE, // from undefined
                MemoryAccessTypes::CPU_MEMORY_READ, // from general
                MemoryAccessTypes::GPU_MEMORY_WRITE,     // from color rt
                MemoryAccessTypes::GPU_MEMORY_WRITE,                 // from depth rt
                MemoryAccessTypes::GPU_MEMORY_WRITE,                 // from stencil rt
                MemoryAccessTypes::GPU_MEMORY_WRITE,                 // from d/s rt
                MemoryAccessTypes::GPU_MEMORY_READ,                 // from depth buffer
                MemoryAccessTypes::GPU_MEMORY_READ,                 // from stencil buffer
                MemoryAccessTypes::GPU_MEMORY_READ,                 // from d/s buffer
                MemoryAccessTypes::GPU_MEMORY_READ,  // from shader read
                MemoryAccessTypes::GPU_MEMORY_READ,  // from transfer src
                MemoryAccessTypes::GPU_MEMORY_WRITE,  // from transfer dst
                MemoryAccessTypes::GPU_MEMORY_READ,  // from present
            };
            ( void )newState;
            return aTypes[currState];
        }

        MEMORY_ACCESS_TYPE CTexture::ConvertStateToDstMemoryAccess( const TEXTURE_STATE currState,
            const TEXTURE_STATE newState )
        {
            static const MEMORY_ACCESS_TYPE aaTypes[TextureStates::_MAX_COUNT][TextureStates::_MAX_COUNT] =
            {
                // From undefined
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From general
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From color render target
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_READ, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_READ,                  // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_READ,                  // depth stencil buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From depth render target
                {
                    MemoryAccessTypes::SHADER_READ,                      // undefined
                    MemoryAccessTypes::SHADER_READ,                      // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ,         // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_READ,                      // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ                   // present
                },
                // From stencil render target
                {
                    MemoryAccessTypes::SHADER_READ,                      // undefined
                    MemoryAccessTypes::SHADER_READ,                      // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ,         // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_READ,                      // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ                   // present
                },
                // From depth stencil render target
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                  // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                  // depth stencil buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From dpeth buffer
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                  // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                  // depth stencil buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From stencil buffer
                {
                    MemoryAccessTypes::SHADER_READ,                      // undefined
                    MemoryAccessTypes::SHADER_READ,                      // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ,         // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_READ,                      // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ                   // present
                },
                // From dpeth stencil buffer
                {
                    MemoryAccessTypes::SHADER_READ,                      // undefined
                    MemoryAccessTypes::SHADER_READ,                      // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ,         // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_READ,                      // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ                   // present
                },
                // From shader read
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From transfer src
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From transfer dst
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_READ, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                },
                // From present
                {
                    MemoryAccessTypes::SHADER_READ, // undefined
                    MemoryAccessTypes::SHADER_READ, // general
                    MemoryAccessTypes::COLOR_RENDER_TARGET_READ, // color render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // stencil render target
                    MemoryAccessTypes::DEPTH_STENCIL_RENDER_TARGET_READ, // depth stencil render target
                    MemoryAccessTypes::GPU_MEMORY_WRITE, // depth buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // stencil buffer
                    MemoryAccessTypes::GPU_MEMORY_WRITE,                 // depth stencil buffer
                    MemoryAccessTypes::SHADER_WRITE, // shader read
                    MemoryAccessTypes::DATA_TRANSFER_READ,               // transfer src
                    MemoryAccessTypes::DATA_TRANSFER_WRITE,              // transfer dst
                    MemoryAccessTypes::GPU_MEMORY_READ // present
                }
            };
            static const MEMORY_ACCESS_TYPE aTypes[ TextureStates::_MAX_COUNT ] =
            {
                MemoryAccessTypes::GPU_MEMORY_READ, // to undefined
                MemoryAccessTypes::CPU_MEMORY_WRITE,  // to general
                MemoryAccessTypes::GPU_MEMORY_READ, // to color rt
                MemoryAccessTypes::GPU_MEMORY_READ, // to depth rt
                MemoryAccessTypes::GPU_MEMORY_READ, // to stencil rt
                MemoryAccessTypes::GPU_MEMORY_READ, // to d/s rt
                MemoryAccessTypes::GPU_MEMORY_WRITE,  // to depth buffer
                MemoryAccessTypes::GPU_MEMORY_WRITE,  // to stencil buffer
                MemoryAccessTypes::GPU_MEMORY_WRITE,  // to d/s buffer
                MemoryAccessTypes::GPU_MEMORY_WRITE,  // to shader read
                MemoryAccessTypes::GPU_MEMORY_WRITE,  // to transfer src
                MemoryAccessTypes::GPU_MEMORY_READ, // to transfer dst
                MemoryAccessTypes::GPU_MEMORY_WRITE,  // to present
            };
            //return aaTypes[currState][newState];
            ( void )currState;
            return aTypes[ newState ];
        }

        CSampler::CSampler( CTextureManager* )
        {}

        CSampler::~CSampler()
        {}

        void CSampler::_Destroy()
        {}

        void CSampler::Init( const SSamplerDesc& Desc )
        {
            m_Desc = Desc;
        }

        hash_t CSampler::CalcHash( const SSamplerDesc& Desc )
        {
            Utils::SHash Hash;
            Hash.Combine( Desc.AddressMode.U, Desc.AddressMode.V, Desc.AddressMode.W );
            Hash.Combine( Desc.borderColor, Desc.compareFunc, Desc.enableAnisotropy, Desc.enableCompare );
            Hash.Combine( Desc.Filter.mag, Desc.Filter.min, Desc.LOD.min, Desc.LOD.max );
            Hash.Combine( Desc.maxAnisotropy, Desc.mipLODBias, Desc.mipmapMode, Desc.unnormalizedCoordinates );
            return Hash.value;
        }

        CTexture::CTexture(CTextureManager* pMgr) :
            m_pMgr{ pMgr }
            , m_isColor{ 0 }
            , m_isDepth{ 0 }
            , m_isStencil{ 0 }
            , m_isReady{ 0 }
        {}

        CTexture::~CTexture()
        {}

        void CTexture::Init(const STextureDesc& Desc)
        {
            if( !this->IsResourceStateSet( Core::ResourceStates::INITIALIZED ) )
            {
                m_Desc = Desc;
                TEXTURE_ASPECT aspect = ConvertFormatToAspect( m_Desc.format );
                switch( aspect )
                {
                    case TextureAspects::COLOR: m_isColor = true; break;
                    case TextureAspects::DEPTH: m_isDepth = true; break;
                    case TextureAspects::STENCIL: m_isStencil = true; break;
                    case TextureAspects::DEPTH_STENCIL:
                        m_isDepth = true;
                        m_isStencil = true;
                        break;
                }
                this->m_hDDIObject = m_Desc.hNative;
                this->_AddResourceState( Core::ResourceStates::INITIALIZED );
                if(m_Desc.hNative != DDI_NULL_HANDLE)
                {
                    this->_AddResourceState( Core::ResourceStates::CREATED );
                }
            }
        }

        bool CTexture::SetState( const TEXTURE_STATE& state, STextureBarrierInfo* pOut )
        {
            bool ret = false;
            if( m_state != state )
            {
                pOut->currentState = m_state;
                pOut->hDDITexture = GetDDIObject();
                pOut->newState = state;
                pOut->SubresourceRange.aspect = ConvertFormatToAspect(m_Desc.format);
                pOut->SubresourceRange.beginArrayLayer = 0;
                pOut->SubresourceRange.beginMipmapLevel = 0;
                pOut->SubresourceRange.layerCount = 1;
                pOut->SubresourceRange.mipmapLevelCount = m_Desc.mipmapCount;
                pOut->srcMemoryAccess = ConvertStateToSrcMemoryAccess(m_state, state);
                pOut->dstMemoryAccess = ConvertStateToDstMemoryAccess(m_state, state);
                m_state = state;
                ret = true;
            }
            return ret;
        }

        TextureViewRefPtr CTexture::GetView()
        {
            return m_pMgr->GetTextureView( m_hView );
        }

        SamplerRefPtr CTexture::GetSampler()
        {
            return m_pMgr->GetSampler( m_hSampler );
        }

        hash_t CTexture::CalcHash( const STextureDesc& Desc )
        {
            Utils::SHash Hash;
            Hash.Combine( Desc.format, Desc.memoryUsage, Desc.mipmapCount, Desc.multisampling,
                Desc.Size.width, Desc.Size.height, Desc.type, Desc.usage );
            return Hash.value;
        }

        hash_t CTexture::CalcHash(cstr_t pName)
        {
            Utils::SHash Hash;
            Hash += pName;
            return Hash.value;
        }

        bool CTexture::IsReady()
        {
            if(m_isReady)
            {
                return true;
            }
            bool ret = m_isReady || IsResourceReady();
            if( ret )
            {
                if( ( m_Desc.usage & TextureUsages::TRANSFER_DST ) == TextureUsages::TRANSFER_DST )
                {
                    ret = m_isDataUploaded;
                }
            }
            m_isReady = ret;
            return ret;
        }

        void CTexture::NotifyReady()
        {
            m_pImage->Release();
            //m_hFence = DDI_NULL_HANDLE;
            m_pCommandBufferState = nullptr;
            m_isReady = true;
            _RemoveResourceState( Core::ResourceStates::PENDING );
        }

        CTextureView::CTextureView()
        {}

        CTextureView::~CTextureView()
        {}

        void CTextureView::Init( const STextureViewDesc& Desc, TexturePtr pTexture )
        {
            m_Desc = Desc;
            this->m_hDDIObject = m_Desc.hNative;
        }

        hash_t CTextureView::CalcHash( const STextureViewDesc& Desc )
        {
            Utils::SHash Hash;
            Hash.Combine( Desc.format, Desc.hTexture.handle, Desc.SubresourceRange.aspect,
                Desc.SubresourceRange.beginArrayLayer, Desc.SubresourceRange.beginMipmapLevel,
                Desc.SubresourceRange.layerCount, Desc.SubresourceRange.mipmapLevelCount,
                Desc.type );
            return Hash.value;
        }

        CRenderTarget::CRenderTarget()
        {}

        CRenderTarget::~CRenderTarget()
        {}

        void CRenderTarget::_Destroy()
        {

        }

        void CRenderTarget::Init( const SRenderTargetDesc& Desc )
        {
            m_Desc.beginState = Desc.beginState;
            m_Desc.ClearValue = Desc.ClearValue;
            m_Desc.endState = Desc.endState;
            m_Desc.format = Desc.format;
            m_Desc.sampleCount = Desc.multisampling;
            m_Desc.usage = Desc.renderPassUsage;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM