#include "RenderSystem/Vulkan/Managers/CShaderManager.h"
#if VKE_VULKAN_RENDERER
#include "Core/VKEConfig.h"
#include "RenderSystem/Vulkan/Resources/CShader.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CVkEngine.h"
#include "Core/Threads/CThreadPool.h"

namespace VKE
{
    namespace RenderSystem
    {
        CShaderManager::CShaderManager(CDeviceContext* pCtx) :
            m_pCtx{ pCtx }
        {}

        CShaderManager::~CShaderManager()
        {}

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
                    
                    const uint32_t shaderSize = sizeof( CShader );
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
                        const uint32_t programSize = sizeof( CShaderProgram );
                        res = m_ShaderProgramFreeListPool.Create( m_Desc.maxShaderProgramCount, programSize, 1 );
                        if( VKE_SUCCEEDED( res ) )
                        {

                        }
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

        ShaderPtr CShaderManager::CreateShader(const SResourceCreateDesc& Desc)
        {
            if( Desc.async )
            {
                ShaderManagerTasks::SCreateShaderTask* pTask;
                if( m_CreateShaderTaskPool.vFreeElements.PopBack( pTask ) )
                {

                }
                m_pCtx->GetRenderSystem()->GetEngine()->GetThreadPool()->AddTask( -1, );
            }
            else
            {
                return _CreateShaderTask( Desc );
            }
        }

        ShaderPtr CShaderManager::_CreateShaderTask(const SResourceCreateDesc& Desc)
        {
            ShaderPtr pRet;
            VKE_ASSERT( Desc.pTypeDesc, "Resource (SShaderDesc) type desc must be set." );
            SShaderDesc* pShaderDesc = reinterpret_cast< SShaderDesc* >( Desc.pTypeDesc );
            auto& Allocator = m_aShaderFreeListPools[ pShaderDesc->type ];
            CShader* pShader;
            if( VKE_SUCCEEDED( Memory::CreateObject( &Allocator, &pShader, pShaderDesc->type ) ) )
            {
                pShader->m_Desc = *pShaderDesc;
                pShader->m_ResourceDesc = Desc;
                pShader->m_resourceState = ResourceStates::CREATED;
                pRet = ShaderPtr( pShader );
                if( Desc.stages & ResourceStageBits::PREPARE )
                {
                    if( VKE_SUCCEEDED( PrepareShader( &pRet ) ) )
                    {
                        if( Desc.stages & ResourceStageBits::LOAD )
                        {
                            if( VKE_SUCCEEDED( LoadShader( &pRet ) ) )
                            {

                            }
                            else
                            {
                                goto FAIL;
                            }
                        }
                        else
                        {
                            goto FAIL;
                        }
                    }
                    else
                    {
                        goto FAIL;
                    }
                }
            }
            else
            {
                VKE_LOG_ERR("Unable to allocate memory for CShader object.");
            }
            return pRet;
        FAIL:
            Memory::DestroyObject( &Allocator, &pShader );
            return ShaderPtr();
        }

        Result CShaderManager::PrepareShader(ShaderPtr* pShader)
        {
            Result res = VKE_FAIL;

            return res;
        }

        Result CShaderManager::LoadShader(ShaderPtr* pShader)
        {
            Result res = VKE_FAIL;

            return res;
        }

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