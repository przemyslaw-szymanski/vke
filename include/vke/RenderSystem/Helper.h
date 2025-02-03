#pragma once

#include "Common.h"

namespace VKE::RenderSystem::Helper
{
    static bool IsDepthStencilFormat( FORMAT );
    static bool IsStencilFormat( FORMAT );
    static bool IsDepthFormat( FORMAT );
    static bool HasDepth( FORMAT );
    static bool HasStencil( FORMAT );
    
} // namespace RenderSystem::Helper


namespace VKE::RenderSystem::Helper
{
    bool IsDepthFormat(FORMAT fmt)
    {
        bool ret = false;
        switch (fmt)
        {
            case Formats::D32_SFLOAT:
            case Formats::D16_UNORM:
            case Formats::X8_D24_UNORM_PACK32:
                    ret = true;
                 break;
        }
        return ret;
    }

    bool IsDepthStencilFormat(FORMAT fmt)
    {
        bool ret = false;
        switch (fmt)
        {
            case Formats::D32_SFLOAT_S8_UINT:
            case Formats::D24_UNORM_S8_UINT:
            case Formats::D16_UNORM_S8_UINT:
                     ret = true;
                break;
        }
        return ret;
    }

    bool IsStencilFormat(FORMAT fmt)
    {
        bool ret = false;
        switch (fmt)
        {
            case Formats::S8_UINT: ret = true; break;
        }
        return ret;
    }

    bool HasDepth(FORMAT fmt)
    {
        bool ret = false;
        switch (fmt)
        {
            case Formats::D32_SFLOAT:
            case Formats::D32_SFLOAT_S8_UINT:
            case Formats::D24_UNORM_S8_UINT:
            case Formats::X8_D24_UNORM_PACK32:
            case Formats::D16_UNORM:
            case Formats::D16_UNORM_S8_UINT:
                     ret = true;
                break;
        }
        return ret;
    }

    bool HasStencil(FORMAT fmt)
    {
        bool ret = false;
        switch (fmt)
        {
            case Formats::S8_UINT:
            case Formats::D24_UNORM_S8_UINT:
            case Formats::D16_UNORM_S8_UINT:
            case Formats::D32_SFLOAT_S8_UINT:
                     ret = true;
                break;
        }
        return ret;
    }
}