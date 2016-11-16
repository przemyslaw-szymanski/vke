#pragma once

#include "RenderSystem/Vulkan/Common.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct EnumDisabledEnabled
        {
            enum STATE
            {
                DISABLED,
                ENABLED,
                _ENUM_COUNT
            };
        };

        struct RenderStates
        {
            struct Raster
            {
                struct DepthClamp : EnumDisabledEnabled
                {};

                struct Discard : EnumDisabledEnabled
                {};

                struct 
            };

            struct Blend
            {
                struct States : EnumDisabledEnabled
                {
                   
                };

                struct Ops
                {
                    enum OP
                    {

                    };
                };
            };

            struct Depth
            {
                struct States : EnumDisabledEnabled
                {
                    
                };

                struct Ops
                {
                    enum OP
                    {
                        GREATER,
                        LESS,
                        GREATER_OR_EQUAL,
                        LESS_OR_EQUAL,
                        _ENUM_COUNT
                    };
                };
            };

            struct Topology
            {
                struct Types
                {
                    enum TYPE
                    {
                        POINT_LIST,
                        LINE_LIST,
                        LINE_STRIP,
                        TRIANGLE_LIST,
                        TRIANGLE_STRIP,
                        TRIANGLE_FAN,
                        PATCH_LIST,
                        _ENUM_COUNT
                    };
                };
            };

        };

        using BlendStates = RenderStates::Blend::States;
        using BlendOps = RenderStates::Blend::Ops;
        using BLEND_STATE = BlendStates::STATE;
        using BLEND_OP = BlendOps::OP;
        using DepthStates = RenderStates::Depth::States;
        using DepthOps = RenderStates::Depth::Ops;
        using DEPTH_STATE = DepthStates::STATE;
        using DEPTH_OP = DepthOps::OP;
        using PrimitiveTopologies = RenderStates::Topology::Types;
        using PRIMITIVE_TOPOLOGY = PrimitiveTopologies::TYPE;

        class CRenderTreeNode
        {
            public:

            struct States
            {
                enum STATE
                {
                    RASTER,
                    VIEWPORT,
                    STENCIL,
                    DEPTH,
                    BLEND,
                    MULTISAMPLING,
                    PRIMITIVE_TOPOLOGY,
                    VERTEX_SHADER,
                    HULL_SHADER,
                    DOMAIN_SHADER,
                    TEXTURE,
                    PIXEL_SHADER,
                    _ENUM_COUNT
                };
            };

            using TreeNodeVec = vke_vector< CRenderTreeNode >;


            protected:

            // depth
            // blend
            // viewport
            // raster
            // stencil
            // vertex shader
            // geometry shader
            // hull shader
            // domain shader
            // pixel shader
            // primitive topology
            // multisampling

        };

        class CRenderTree
        {
            public:

            protected:
        };
    }
}