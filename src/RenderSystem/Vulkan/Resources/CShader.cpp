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

        CShader::CShader(CShaderManager* pMgr, SHADER_TYPE) :
            Resources::CResource( 0 )
            //, m_ShaderMemory{ g_aLanguages[ type ] }
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

        hash_t CShader::CalcHash(const SShaderDesc& Desc)
        {
            const hash_t h1 = CResource::CalcHash( Desc.Base );
            const hash_t h2 = CResource::CalcHash( Desc.pEntryPoint );
            const hash_t h3 = Desc.type;
            const hash_t h4 = h1 ^ ( h2 << 1 );
            const hash_t h5 = h4 ^ ( h3 << 1 );
            const hash_t h6 = h5 ^ ( h4 << 1 );
            hash_t incHash = 0;
            hash_t prepHash = 0;
            for (uint32_t i = 0; i < Desc.vIncludes.GetCount(); ++i)
            {
                const hash_t h = CResource::CalcHash( Desc.vIncludes[ i ] );
                incHash = h ^ ( incHash << 1 );
            }

            for (uint32_t i = 0; i < Desc.vPreprocessor.GetCount(); ++i)
            {
                const hash_t h = CResource::CalcHash( Desc.vPreprocessor[ i ] );
                prepHash = h ^ ( prepHash << 1 );
            }
            const hash_t hash = h6 ^ ( incHash ) ^ ( prepHash );
            return hash;
        }

        void CShader::Init(const SShaderDesc& Info)
        {
            if( !( this->m_resourceState & ResourceStates::INITIALIZED ) )
            {
                m_CompilerData.pShader = ::new( &m_CompilerData.ShaderMemory ) glslang::TShader( g_aLanguages[ Info.type ] );
				m_CompilerData.pProgram = ::new( &m_CompilerData.ProgramMemory ) glslang::TProgram();
                m_Desc = Info;
                SHandle Handle;
                Handle.value = 0;
                Handle.hash = CalcHash( m_Desc );
                Handle.type = Info.type;
                this->m_hObjHandle = Handle.value;
                this->m_resourceState |= ResourceStates::INITIALIZED;
            }
        }

        void CShader::Release()
        {
			m_CompilerData.Release();
            this->m_resourceState = ResourceStates::INVALIDATED;
            if( this->GetRefCount() == 0 )
            {
                m_pMgr->_FreeShader( this );
            }
        }

        Result CShader::Compile()
        {
            VKE_ASSERT( m_pMgr, "Shader manager is not set." );
            Result res = VKE_OK;
            if( m_vkModule == VK_NULL_HANDLE )
            {
                CShader* pThis = this;
                res = m_pMgr->_PrepareShaderTask( &pThis );
            }
            return res;
        }


        CShaderProgram::CShaderProgram(CShaderManager* pMgr) :
            Resources::CResource( 0 )
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
                m_Desc.apShaders[ i ] = nullptr;
            }
            this->m_resourceState = ResourceStates::INVALIDATED;
            if( this->GetRefCount() == 0 )
            {
                //m_pMgr->_FreeProgram( this );
            }
        }

        void CShaderProgram::Init(const SShaderProgramDesc& Desc)
        {
            m_Desc = Desc;
            this->m_resourceState = ResourceStates::CREATED;
            SHash Hash;
            Hash += CalcHash( Desc.Base );
            
            for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            {
                //h2 ^= ( CalcHash( Desc.apEntryPoints[ i ] ) << 1 );
                Hash += Desc.apEntryPoints[ i ];
                if( Desc.apShaders[ i ].IsValid() )
                {
                    //h3 ^= ( Desc.apShaders[ i ]->GetHandle() << 1 );
                    Hash += Desc.apShaders[ i ]->GetHandle();
                }
            }

            this->m_hObjHandle = Hash.value;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER