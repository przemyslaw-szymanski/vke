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

        CShader::CShader(CShaderManager* pMgr, SHADER_TYPE type) :
            Resources::CResource( 1 )
            , m_Shader{ g_aLanguages[ type ] }
            , m_pMgr{ pMgr }
        {
        }

        CShader::~CShader()
        {
        }

        void CShader::operator delete(void* pShader)
        {
            CShader* pThis = static_cast< CShader* >( pShader );
            VKE_ASSERT( pThis, "Can not delete null pointer." );
            pThis->Release();
        }

        void CShader::Init(const SShaderDesc& Info)
        {
            m_Desc = Info;
            const hash_t h1 = CalcHash( m_Desc.Base );
            const hash_t h2 = CalcHash( m_Desc.pEntryPoint );
            const hash_t h3 = m_Desc.type;
            const hash_t h4 = h1 ^ ( h2 << 1 );
            const hash_t h5 = h4 ^ ( h3 << 1 );
            const hash_t h6 = h5 ^ ( h4 << 1 );
            this->m_resourceHash = h6;
            this->m_resourceState = ResourceStates::CREATED;
        }

        void CShader::Release()
        {
            if( this->GetRefCount() == 0 )
            {
                m_pMgr->_FreeShader( this );
            }
        }


        CShaderProgram::CShaderProgram(CShaderManager* pMgr) :
            Resources::CResource( 1 )
            , m_pMgr( pMgr )
        {
            this->m_objRefCount = 0;
        }

        CShaderProgram::~CShaderProgram()
        {}

        void CShaderProgram::operator delete(void* pProgram)
        {
            CShaderProgram* pThis = static_cast< CShaderProgram* >( pProgram );
            VKE_ASSERT( pThis, "Can not delete null pointer." );
            pThis->Release();
        }
        
        void CShaderProgram::Release()
        {
            for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            {
                m_Desc.apShaders[ i ] = ShaderRefPtr();
            }
            //if( this->GetRefCount() == 0 )
            {
                m_pMgr->_FreeProgram( this );
            }
        }

        void CShaderProgram::Init(const SShaderProgramDesc& Desc)
        {
            m_Desc = Desc;
            this->m_resourceState = ResourceStates::CREATED;
            hash_t h1 = CalcHash( Desc.Base );
            hash_t h2 = 0;
            hash_t h3 = 0;
            for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            {
                h2 ^= ( CalcHash( Desc.apEntryPoints[ i ] ) << 1 );
                if( Desc.apShaders[ i ].IsValid() )
                {
                    h3 ^= ( Desc.apShaders[ i ]->GetResourceHash() << 1 );
                }
            }
            hash_t h4 = h1 ^ ( h2 << 1 );
            hash_t h5 = h4 ^ ( h3 << 1 );
            this->m_resourceHash = h5;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER