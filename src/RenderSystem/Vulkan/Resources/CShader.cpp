#include "RenderSystem/Vulkan/Resources/CShader.h"
#if VKE_VULKAN_RENDERER
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


        CShaderProgram::CShaderProgram()
        {}

        CShaderProgram::~CShaderProgram()
        {}

        void CShaderProgram::Init(const InitInfo& Info)
        {
            m_Info = Info;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER