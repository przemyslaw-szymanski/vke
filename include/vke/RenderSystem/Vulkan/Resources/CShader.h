#pragma once
#include "RenderSystem/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#if VKE_VULKAN_RENDERER
#include "ThirdParty/glslang/glslang/Include/ShHandle.h"
#include "RenderSystem/Vulkan/Vulkan.h"
#include "Core/Resources/CFile.h"
#include "Core/VKEConfig.h"
#include "RenderSystem/Vulkan/CShaderCompiler.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SShaderInitInfo
        {
        };

        class VKE_API CShader final : public Resources::CResource
        {
            friend class CShaderManager;
            friend class SShaderCompiler;
            public:

				struct SCompilerData
				{
					uint8_t             ShaderMemory[ sizeof( glslang::TShader ) ];
					uint8_t				ProgramMemory[ sizeof( glslang::TProgram ) ];
					glslang::TShader*   pShader = nullptr;
					glslang::TProgram*	pProgram = nullptr;
					SCompileShaderData	CompileData;

					~SCompilerData()
					{
						Release();
					}

					void Release()
					{
						pProgram->~TProgram();
						pShader->~TShader();
						pProgram = nullptr;
						pShader = nullptr;
						CompileData.~SCompileShaderData();
					}
				};

                using InitInfo = SShaderInitInfo;
                using ShaderBinaryBuffer = Utils::TCDynamicArray< uint8_t, Config::RenderSystem::Shader::DEFAULT_SHADER_BINARY_SIZE >;

            public:

                        CShader(CShaderManager* pMgr, SHADER_TYPE type);
                        ~CShader();

                void    operator delete(void*);

                static hash_t   CalcHash(const SShaderDesc&);
                void            Init(const SShaderDesc& Info);
                void            Release();
                Result          Compile();

				const SCompilerData&    GetCompilerData() const { return m_CompilerData; }
                const SShaderDesc&      GetDesc() const { return m_Desc; }
                const VkShaderModule&   GetNative() const { return m_vkModule; }

            protected:

				SCompilerData		m_CompilerData;
				SShaderDesc         m_Desc;
                CShaderManager*     m_pMgr;
                FileRefPtr          m_pFile;
                VkShaderModule      m_vkModule = VK_NULL_HANDLE;          
        };


        struct SShaderProgramDesc
        {
            using ShaderArray = ShaderRefPtr[ ShaderTypes::_MAX_COUNT ];
            using EntryPointArray = cstr_t[ ShaderTypes::_MAX_COUNT ];
            SResourceDesc   Base;
            ShaderArray     apShaders;
            EntryPointArray apEntryPoints = { nullptr };
        };

        class VKE_API CShaderProgram final : public Resources::CResource
        {
            friend class CShaderManager;
            public:

            public:

                        CShaderProgram(CShaderManager* pMgr);
                        ~CShaderProgram();

                void    operator delete(void*);

                void    Init(const SShaderProgramDesc& Desc);
                void    Release();

            protected:

                glslang::TProgram   m_Program;
                CShaderManager*     m_pMgr;
                FileRefPtr          m_pFile;
                SShaderProgramDesc  m_Desc;
        };

    } // RenderSystem
    using ShaderPtr = Utils::TCWeakPtr< RenderSystem::CShader >;
    using ShaderRefPtr = Utils::TCObjectSmartPtr< RenderSystem::CShader >;
    using ShaderProgramPtr = Utils::TCWeakPtr< RenderSystem::CShaderProgram >;
    using ShaderProgramRefPtr = Utils::TCObjectSmartPtr< RenderSystem::CShaderProgram >;
} // VKE
#endif // VKE_VULKAN_RENDERER