#pragma once

#include "Core/VKEConfig.h"
#include "Core/Resources/CFile.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Resources/CResource.h"

#include "RenderSystem/Common.h"
#include "RenderSystem/RHI.h"
#include "RenderSystem/Vulkan/CShaderCompiler.h"

#include "glslang/Public/ShaderLang.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CShaderManager;

        struct SShaderInitInfo
        {
        };

        class VKE_API CShader : public Core::TCResource< CShader >
        {
            friend class CShaderManager;
            friend class SShaderCompiler;

            struct alignas( sizeof( uint64_t ) ) SHandle
            {
                union
                {
                    struct
                    {
                        uint64_t hash : 61;
                        uint64_t type : 3;
                    };

                    uint64_t value;
                };
            };

            VKE_ADD_DDI_OBJECT( RHI::Shader );
            VKE_DECL_BASE_OBJECT( ShaderHandle );

        public:
            // using CompilerData = CAPI::CompilerData;
            using InitInfo = SShaderInitInfo;
            using ShaderBinaryBuffer =
                Utils::TCDynamicArray< uint8_t, Config::RenderSystem::Shader::DEFAULT_SHADER_BINARY_SIZE >;

        public:
            CShader( CShaderManager* pMgr, SHADER_TYPE type );
            ~CShader();
            static hash_t CalcHash( const SShaderDesc& );
            void          Init( const SShaderDesc& Info, const hash_t& hash );
            void          Release();
            Result        Compile();

            // const CompilerData*     GetCompilerData() const { return &m_CompilerData; }
            const SShaderDesc& GetDesc() const
            {
                return m_Desc;
            }

            // const VkShaderModule&   GetNative() const { return m_vkModule; }
        protected:
            void _SetFile( Core::FilePtr pFile );

        protected:
            SShaderDesc m_Desc;
            SShaderData m_Data;
            // CompilerData		m_CompilerData;
            CShaderManager*       m_pMgr;
            Core::FileRefPtr      m_pFile;
            Core::RESOURCE_STAGES m_resourceStages;
        };

        struct SShaderProgramDesc
        {
            using ShaderArray     = ShaderRefPtr[ ShaderTypes::_MAX_COUNT ];
            using EntryPointArray = cstr_t[ ShaderTypes::_MAX_COUNT ];
            Core::SFileInfo FileInfo;
            ShaderArray     apShaders;
            EntryPointArray apEntryPoints = { nullptr };
        };

        class VKE_API CShaderProgram : public Core::TCResource< CShaderProgram >
        {
            friend class CShaderManager;
            VKE_DECL_BASE_OBJECT( handle_t );

        public:
            CShaderProgram( CShaderManager* pMgr );
            ~CShaderProgram();
            void operator delete( void* );
            void Init( const SShaderProgramDesc& Desc );
            void Release();

        protected:
            glslang::TProgram  m_Program;
            CShaderManager*    m_pMgr;
            Core::FileRefPtr   m_pFile;
            SShaderProgramDesc m_Desc;
        };
    } // namespace RenderSystem

    using ShaderPtr           = Utils::TCWeakPtr< RenderSystem::CShader >;
    using ShaderRefPtr        = Utils::TCObjectSmartPtr< RenderSystem::CShader >;
    using ShaderProgramPtr    = Utils::TCWeakPtr< RenderSystem::CShaderProgram >;
    using ShaderProgramRefPtr = Utils::TCObjectSmartPtr< RenderSystem::CShaderProgram >;

} // namespace VKE
