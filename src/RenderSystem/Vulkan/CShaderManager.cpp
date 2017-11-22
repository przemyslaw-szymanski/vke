#include "RenderSystem/Vulkan/Managers/CShaderManager.h"
#if VKE_VULKAN_RENDERER

#include "Core/VKEConfig.h"
namespace VKE
{
    namespace RenderSystem
    {
        void CShaderManager::Destroy()
        {
            for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            {
                m_aShaderFreeListPools[ i ].Destroy();
            }
            m_ShaderProgramFreeListPool.Destroy();
        }

        Result CShaderManager::Create(const SShaderManagerDesc& Desc)
        {
            Result res = VKE_FAIL;
            m_Desc = Desc;
            res = Memory::CreateObject( &HeapAllocator, &m_pCompiler, this );
            if( VKE_SUCCEEDED( res ) )
            {
                SShaderCompilerDesc CompilerDesc;
                res = m_pCompiler->Create( CompilerDesc );
                if( VKE_SUCCEEDED( res ) )
                {
                    m_Desc.aMaxShaderCounts[ ShaderTypes::VERTEX ] = std::max( m_Desc.aMaxShaderCounts[ ShaderTypes::VERTEX ], Config::Resource::Shader::MAX_VERTEX_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_HULL ] = std::max( m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_HULL ], Config::Resource::Shader::MAX_TESSELATION_HULL_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_DOMAIN ] = std::max( m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_DOMAIN ], Config::Resource::Shader::MAX_TESSELATION_DOMAIN_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::GEOMETRY ] = std::max( m_Desc.aMaxShaderCounts[ ShaderTypes::GEOMETRY ], Config::Resource::Shader::MAX_GEOMETRY_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::PIXEL ] = std::max( m_Desc.aMaxShaderCounts[ ShaderTypes::PIXEL ], Config::Resource::Shader::MAX_PIXEL_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::COMPUTE ] = std::max( m_Desc.aMaxShaderCounts[ ShaderTypes::COMPUTE ], Config::Resource::Shader::MAX_COMPUTE_SHADER_COUNT );
                    m_Desc.maxShaderProgramCount = std::max( m_Desc.maxShaderProgramCount, Config::Resource::Shader::MAX_SHADER_PROGRAM_COUNT );
                    
                    const uint32_t shaderSize = sizeof( glslang::TShader );
                    bool success = true;
                    for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                    {
                        res = m_aShaderFreeListPools[ i ].Create( m_Desc.aMaxShaderCounts[ i ], shaderSize, 1 );
                        if( VKE_FAILED( res ) )
                        {
                            success = false;
                        }
                    }
                    if( success )
                    {
                        const uint32_t programSize = sizeof( glslang::TProgram );
                        res = m_ShaderProgramFreeListPool.Create( m_Desc.maxShaderProgramCount, programSize, 1 );
                    }
                }
            }
            return res;
        }


        static const EShLanguage g_aLanguages[ EShLangCount ] =
        {
            EShLangVertex,
            EShLangTessControl,
            EShLangTessEvaluation,
            EShLangGeometry,
            EShLangFragment,
            EShLangCompute
        };

        Result CShaderManager::Compile()
        {
            Result res = VKE_FAIL;
            SHADER_TYPE shaderType = ShaderTypes::VERTEX;
            EShLanguage lang = g_aLanguages[ shaderType ];

            SCompileShaderInfo Info;
            Info.pBuffer = "#version 140 \nvoid main() {}";
            Info.bufferSize = strlen( Info.pBuffer );
            Info.type = shaderType;
            auto& Allocator = m_aShaderFreeListPools[ shaderType ];
            if( VKE_SUCCEEDED( Memory::CreateObject( &Allocator, &Info.pShader, lang ) ) )
            {
                SCompileShaderData Data;
                res = m_pCompiler->Compile( Info, &Data );
                if( VKE_SUCCEEDED( res ) )
                {
                    //m_CurrCompilationUnit.apShaders[ shaderType ] = Info.pShader;
                    m_CurrCompilationUnit.aInfos[ shaderType ] = Info;
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to allocate memory for glslang::TShader object." );
            }
            return res;
        }

        Result CShaderManager::Link()
        {
            Result res = VKE_FAIL;
            SLinkShaderInfo Info;
            res = Memory::CreateObject( &m_ShaderProgramFreeListPool, &Info.pProgram );
            if( VKE_SUCCEEDED( res ) )
            {
                //Memory::Copy< glslang::TShader*, ShaderTypes::_MAX_COUNT >( Info.apShaders, m_CurrCompilationUnit.apShaders );
                for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                {
                    Info.apShaders[ i ] = m_CurrCompilationUnit.aInfos[ i ].pShader;
                }
                SLinkShaderData Data;
                res = m_pCompiler->Link( Info, &Data );
                m_pCompiler->WriteToHeaderFile( "test.h", m_CurrCompilationUnit.aInfos[ ShaderTypes::VERTEX ], Data );
                m_pCompiler->WriteToBinaryFile( "test.spv", m_CurrCompilationUnit.aInfos[ ShaderTypes::VERTEX ], Data );
            }
            else
            {
                VKE_LOG_ERR("Unable to allocate memory for glslang::TProgram object.");
            }
            return res;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER