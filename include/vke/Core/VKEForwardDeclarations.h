#pragma once

#include "Core/Utils/TCSmartPtr.h"

namespace VKE
{
    class CVkEngine;
} // VKE

// Platform
namespace VKE
{
    class CWindow;
}

// Render system types
namespace VKE
{
    class CRenderSystem;
    namespace RenderSystem
    {
        class CRenderTarget;
        class CPipeline;
        class CTexture;
        class CVertexBuffer;
        class CIndexBuffer;
        class CShader;
        class CShaderProgram;
        class CVertexShader;
        class CPixelShader;
        class CGeometryShader;
        class CComputeShader;
        class CHullShader;
        class CDomainShader;
        class CSampler;
        class CImage;
        class CViewport;
        class CCommandBuffer;
        class CRenderQueue;
    } // Render System
} // VKE

// Scene
namespace VKE
{
    class CScene;
    class CSceneManager;
    class CScenePartition;
    class CScenePartitionOctree;
} // VKE

namespace VKE
{
    class CThreadPool;
    class CThreadWorker;
} // VKE

namespace VKE
{
#define VKE_DECL_SMART_PTRS(_name) \
    using _name##Ptr = Utils::TCWeakPtr< C##_name >; \
    using _name##OwnPtr = Utils::TCUniquePtr< C##_name >; \
    using _name##RefPtr = Utils::TCObjectSmartPtr< C##_name >;

    VKE_DECL_SMART_PTRS(Window);
    namespace RenderSystem
    {
        VKE_DECL_SMART_PTRS( RenderTarget );
        VKE_DECL_SMART_PTRS( Pipeline );
        VKE_DECL_SMART_PTRS( Texture );
        VKE_DECL_SMART_PTRS( VertexBuffer );
        VKE_DECL_SMART_PTRS( IndexBuffer );
        VKE_DECL_SMART_PTRS( Shader );
        VKE_DECL_SMART_PTRS( ShaderProgram );
        VKE_DECL_SMART_PTRS( VertexShader );
        VKE_DECL_SMART_PTRS( PixelShader );
        VKE_DECL_SMART_PTRS( ComputeShader );
        VKE_DECL_SMART_PTRS( GeometryShader );
        VKE_DECL_SMART_PTRS( HullShader );
        VKE_DECL_SMART_PTRS( DomainShader );
        VKE_DECL_SMART_PTRS( Sampler );
        VKE_DECL_SMART_PTRS( Image );
        VKE_DECL_SMART_PTRS( Viewport );
        VKE_DECL_SMART_PTRS( CommandBuffer );
        VKE_DECL_SMART_PTRS( RenderQueue );
    } // RnderSystem
} // VKE
