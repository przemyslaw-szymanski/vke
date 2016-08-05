#pragma once

#include "Core/VKECommon.h"
#include "RenderSystem/Vulkan/VKEImageFormats.h"
#include "Core/Platform/CWindow.h"

namespace VKE
{
    class CRenderSystem;

    template<typename _T_>
    struct TSArray
    {
        uint32_t    count = 0;
        const _T_*  pData = nullptr;
    };

    namespace RenderSystem
    {
        struct SViewportInfo
        {
            ExtentU32   Size;
        };

        struct SPipelineInfo
        {
            uint32_t bEnableDepth : 1;
            uint32_t bEnableStencil : 1;
            uint32_t bEnableBlending : 1;
        };

        struct ResourceTypes
        {
            enum TYPE
            {
                PIPELINE,
                VERTEX_BUFFER,
                INDEX_BUFFER,
                CONSTANT_BUFFER,
                IMAGE,
                SAMPLER,
                VERTEX_SHADER,
                HULL_SHADER,
                DOMAIN_SHADER,
                GEOMETRY_SHADER,
                PIXEL_SHADER,
                COMPUTE_SHADER,
                _MAX_COUNT
            };
        };

        struct DeviceTypes
        {
            enum TYPE
            {
                UNKNOWN,
                INTEGRATED,
                DISCRETE,
                VIRTUAL,
                CPU,
                _MAX_COUNT
            };
        };

        using ADAPTER_TYPE = DeviceTypes::TYPE;

        struct GraphicsQueueTypes
        {
            static const uint32_t RENDER = VKE_SET_BIT(0);
            static const uint32_t COMPUTE = VKE_SET_BIT(1);
            static const uint32_t TRANSFER = VKE_SET_BIT(2);
            static const uint32_t _MAX_COUNT = 3;
            static const uint32_t GENERAL = RENDER | COMPUTE | TRANSFER;
        };

        using GRAPHICS_QUEUE_TYPE = uint32_t;

        struct SAdapterLimits
        {

        };

        struct SAdapterInfo
        {
            char            name[Constants::MAX_NAME_LENGTH];
            SAdapterLimits  limits;
            uint32_t        apiVersion;
            uint32_t        driverVersion;
            uint32_t        deviceID;
            uint32_t        vendorID;
            ADAPTER_TYPE    type;
            handle_t        handle;
        };

        struct SSwapChainInfo
        {
            handle_t        hWnd = NULL_HANDLE;
            handle_t        hPlatform = NULL_HANDLE;
            ExtentU32       Size = { 800, 600 };
            IMAGE_FORMAT    format = ImageFormats::R8G8B8A8_UNORM;
            uint16_t        elementCount = Constants::OPTIMAL;
        };

        struct SContextInfo
        {
            const SAdapterInfo*         pAdapterInfo = nullptr;
            TSArray< SSwapChainInfo >   SwapChains;

            SContextInfo()
            {
                SwapChains.count = 0;
            }
        };

        struct SDeviceInfo
        {
            TSArray< SContextInfo > Contexts;
            const SAdapterInfo*     pAdapterInfo = nullptr;
            handle_t                hAPIInstance = NULL_HANDLE;

            SDeviceInfo()
            {
                Contexts.count = 0;
            }
        };

        

        struct SRenderSystemMemoryInfo
        {
            SRenderSystemMemoryInfo() { memset(aResourceTypes, UNDEFINED, sizeof(aResourceTypes)); }
            uint16_t    aResourceTypes[ResourceTypes::_MAX_COUNT];
        };     

        struct SRenderSystemInfo
        {         
            SRenderSystemMemoryInfo     Memory;
            TSArray< SWindowInfo >      Windows;

            SRenderSystemInfo()
            {
            }
        };

        struct SRendersystemLimits
        {

        };

        struct SGraphicsQueueInfo
        {
            GRAPHICS_QUEUE_TYPE     type = GraphicsQueueTypes::GENERAL;
            uint32_t                priority = 0;
        };

    } // RenderSystem

    using SRenderSystemInfo = RenderSystem::SRenderSystemInfo;

} // VKE
