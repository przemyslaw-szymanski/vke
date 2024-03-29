#pragma once
#include "RenderSystem/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "ThirdParty/glslang/glslang/Include/ShHandle.h"
#include "RenderSystem/CDDI.h"
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

        class VKE_API CShader
        {
            friend class CShaderManager;
            friend class SShaderCompiler;

            VKE_ALIGN(sizeof(uint64_t)) struct SHandle
            {
                union
                {
                    struct
                    {
                        uint64_t    hash : 61;
                        uint64_t    type : 3;
                    };
                    uint64_t        value;
                };
            };

            VKE_ADD_DDI_OBJECT( DDIShader );
            VKE_DECL_BASE_OBJECT( ShaderHandle );
            VKE_DECL_BASE_RESOURCE();

            public:

                //using CompilerData = CDDI::CompilerData;
                using InitInfo = SShaderInitInfo;
                using ShaderBinaryBuffer = Utils::TCDynamicArray< uint8_t, Config::RenderSystem::Shader::DEFAULT_SHADER_BINARY_SIZE >;

            public:

                        CShader(CShaderManager* pMgr, SHADER_TYPE type);
                        ~CShader();

                static hash_t   CalcHash(const SShaderDesc&);
                void            Init(const SShaderDesc& Info, const hash_t& hash);
                void            Release();
                Result          Compile();

                //const CompilerData*     GetCompilerData() const { return &m_CompilerData; }
                const SShaderDesc&      GetDesc() const { return m_Desc; }
                //const VkShaderModule&   GetNative() const { return m_vkModule; }

            protected:

                void            _SetFile( Core::FilePtr pFile );

            protected:

                SShaderDesc             m_Desc;
                SShaderData             m_Data;
                //CompilerData		m_CompilerData;
                CShaderManager*         m_pMgr;
                Core::FileRefPtr        m_pFile;
                Core::RESOURCE_STAGES   m_resourceStages;
        };


        struct SShaderProgramDesc
        {
            using ShaderArray = ShaderRefPtr[ ShaderTypes::_MAX_COUNT ];
            using EntryPointArray = cstr_t[ ShaderTypes::_MAX_COUNT ];
            Core::SFileInfo FileInfo;
            ShaderArray     apShaders;
            EntryPointArray apEntryPoints = { nullptr };
        };

        class VKE_API CShaderProgram
        {
            friend class CShaderManager;

            VKE_DECL_BASE_OBJECT( handle_t );
            VKE_DECL_BASE_RESOURCE();

            public:

                        CShaderProgram(CShaderManager* pMgr);
                        ~CShaderProgram();

                void    operator delete(void*);

                void    Init(const SShaderProgramDesc& Desc);
                void    Release();

            protected:

                glslang::TProgram   m_Program;
                CShaderManager*     m_pMgr;
                Core::FileRefPtr          m_pFile;
                SShaderProgramDesc  m_Desc;
        };

    } // RenderSystem
    using ShaderPtr = Utils::TCWeakPtr< RenderSystem::CShader >;
    using ShaderRefPtr = Utils::TCObjectSmartPtr< RenderSystem::CShader >;
    using ShaderProgramPtr = Utils::TCWeakPtr< RenderSystem::CShaderProgram >;
    using ShaderProgramRefPtr = Utils::TCObjectSmartPtr< RenderSystem::CShaderProgram >;
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM