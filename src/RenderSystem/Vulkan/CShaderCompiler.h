#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "ThirdParty/SpirV/glslang/Include/ShHandle.h"
#include "ThirdParty/SpirV/glslang/Public/ShaderLang.h"
#include "ThirdParty/SpirV/glslang/OSDependent/osinclude.h"
#include "Core/VKEConfig.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CShaderManager;

        struct SCompileShaderInfo
        {
            const char*         pName = "Unknown";
            const char*         pEntryPoint = "main";
            const char*         pBuffer = nullptr;
            glslang::TShader*   pShader = nullptr;
            uint32_t            bufferSize = 0;
            SHADER_TYPE         type;
            uint8_t             tid = 0;
        };

        struct SLinkShaderInfo
        {
            glslang::TShader*   apShaders[ ShaderTypes::_MAX_COUNT ] = { nullptr };
            glslang::TProgram*  pProgram = nullptr;
        };

        struct SLinkShaderData
        {
            using ShaderBinaryData = vke_vector < uint32_t >;
            ShaderBinaryData   aShaderBinaries[ ShaderTypes::_MAX_COUNT ];
        };

        struct SShaderBinaryData
        {
            uint8_t*    apBinaries[ ShaderTypes::_MAX_COUNT ] = { nullptr };
            uint32_t    aBinarySizes[ ShaderTypes::_MAX_COUNT ] = { 0 };
        };

        struct SCompileShaderData
        {
        };

        struct SShaderCompilerDesc
        {

        };

        class CShaderCompiler
        {
            public:

                CShaderCompiler(CShaderManager* pMgr);
                ~CShaderCompiler();
                Result Create(const SShaderCompilerDesc& Desc);
                void Destroy();
                Result Compile(const SCompileShaderInfo& Info, SCompileShaderData* pOut);
                Result Link(const SLinkShaderInfo& Info, SLinkShaderData* pOut);
                Result ConvertToBinary(const SLinkShaderData& LinkData, SShaderBinaryData* pOut);
                Result WriteToHeaderFile(const char* pFileName, const SCompileShaderInfo& Info, const SLinkShaderData& Data);
                Result WriteToBinaryFile(const char* pFileName, const SCompileShaderInfo& Info, const SLinkShaderData& Data);

            protected:

                CShaderManager* m_pShaderMgr;
        };
    }
}

#endif // VKE_VULKAN_RENDERER