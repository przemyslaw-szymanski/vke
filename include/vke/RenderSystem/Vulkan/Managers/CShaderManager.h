#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "Core/Memory/TCFreeListManager.h"
#include "RenderSystem/Vulkan/CShaderCompiler.h"

namespace glslang
{
    class TShader;
} // glslang

namespace VKE
{
    namespace RenderSystem
    {
        struct SShaderManagerDesc
        {
            uint32_t    aMaxShaderCounts[ ShaderTypes::_MAX_COUNT ] = { 0 };
            uint32_t    maxShaderProgramCount = 0;
        };

        class CShaderCompiler;
        class CShaderManager
        {
            using ShaderTypeArray = glslang::TShader*[ ShaderTypes::_MAX_COUNT ];
            struct SCompilationUnit
            {
                //ShaderTypeArray   apShaders = { nullptr };
                SCompileShaderInfo  aInfos[ ShaderTypes::_MAX_COUNT ];
            };

            public:

                Result          Create(const SShaderManagerDesc& Desc);
                void            Destroy();

                Result          Compile();
                Result          Link();

            protected:

                SShaderManagerDesc          m_Desc;
                Memory::CFreeListPool       m_aShaderFreeListPools[ ShaderTypes::_MAX_COUNT ];
                Memory::CFreeListPool       m_ShaderProgramFreeListPool;
                CShaderCompiler*            m_pCompiler = nullptr;
                SCompilationUnit            m_CurrCompilationUnit;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER