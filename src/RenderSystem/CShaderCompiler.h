#pragma once
#include "RenderSystem/Common.h"
#include "ThirdParty/glslang/glslang/Include/ShHandle.h"
#include "ThirdParty/glslang/glslang/Public/ShaderLang.h"
#include "ThirdParty/glslang/glslang/OSDependent/osinclude.h"
#include "Core/VKEConfig.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CShaderManager;

        

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
                Result ConvertToBinary(const SLinkShaderData& LinkData, SShaderBinaryData* pOut);
                Result WriteToHeaderFile(const char* pFileName, const SCompileShaderInfo& Info, const SLinkShaderData& Data);
                Result WriteToBinaryFile(const char* pFileName, const SCompileShaderInfo& Info, const SLinkShaderData& Data);
                Result WriteToBinaryFile(cstr_t pFileName, const std::vector<uint32_t>& vBinary);


            protected:

                CShaderManager* m_pShaderMgr;
                bool            m_isCreated = false;
        };
    } // RenderSystem
} // VKE