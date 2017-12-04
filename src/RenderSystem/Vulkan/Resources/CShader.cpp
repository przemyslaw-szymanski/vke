#include "RenderSystem/Vulkan/Resources/CShader.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Managers/CShaderManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        static const EShLanguage g_aLanguages[ EShLangCount ] =
        {
            EShLangVertex,
            EShLangTessControl,
            EShLangTessEvaluation,
            EShLangGeometry,
            EShLangFragment,
            EShLangCompute
        };

        CShader::CShader(SHADER_TYPE type) :
            m_Shader{ g_aLanguages[ type ] }
        {

        }

        CShader::~CShader()
        {
        }

        void CShader::Init(const SShaderInitInfo& Info)
        {
            m_Info = Info;
        }

        void CShader::Release(CShaderManager* pMgr)
        {
            ShaderPtr pShader( this );
            pMgr->FreeShader( &pShader );
        }


        CShaderProgram::CShaderProgram()
        {}

        CShaderProgram::~CShaderProgram()
        {}

        
        void CShaderProgram::Release(CShaderManager* pMgr)
        {
            for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            {
                m_Desc.apShaders[ i ]->Release( pMgr );
                m_Desc.apShaders[ i ] = ShaderPtr();
            }
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER