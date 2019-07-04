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
            m_pMgr{ pMgr }
        {
        }

        CShader::~CShader()
        {
        }


        hash_t CShader::CalcHash(const SShaderDesc& Desc)
        {
            const hash_t h1 = Core::CResource::CalcHash( Desc.Base );
            const hash_t h2 = Core::CResource::CalcHash( Desc.pEntryPoint );
            const hash_t h3 = Desc.type;
            const hash_t h4 = h1 ^ ( h2 << 1 );
            const hash_t h5 = h4 ^ ( h3 << 1 );
            const hash_t h6 = h5 ^ ( h4 << 1 );
            hash_t incHash = 0;
            hash_t prepHash = 0;
            for (uint32_t i = 0; i < Desc.vIncludes.GetCount(); ++i)
            {
                const hash_t h = Core::CResource::CalcHash( Desc.vIncludes[ i ] );
                incHash = h ^ ( incHash << 1 );
            }

            for (uint32_t i = 0; i < Desc.vPreprocessor.GetCount(); ++i)
            {
                const hash_t h = Core::CResource::CalcHash( Desc.vPreprocessor[ i ] );
                prepHash = h ^ ( prepHash << 1 );
            }
            const hash_t hash = h6 ^ ( incHash ) ^ ( prepHash );
            SHandle Handle;
            Handle.value = 0;
            Handle.hash = hash;
            Handle.type = Desc.type;
            return Handle.value;
        }

        void CShader::Init(const SShaderDesc& Info, const hash_t& hash)
        {
            {
                //m_CompilerData.pShader = ::new( &m_CompilerData.ShaderMemory ) glslang::TShader( g_aLanguages[ Info.type ] );
				//m_CompilerData.pProgram = ::new( &m_CompilerData.ProgramMemory ) glslang::TProgram();
                m_Desc = Info;
                if( Info.pData )
                {
                    m_Data = *Info.pData;
                }
                this->m_hObject.handle = hash;
                _SetResourceState( Core::ResourceStates::INITIALIZED );
            }
        }

        void CShader::Release()
        {
			//m_CompilerData.Release();
            _SetResourceState( Core::ResourceStates::INVALID );
            
            if( this->GetRefCount() == 0 )
            {
                m_Data.pCode = nullptr;
                m_Data.codeSize = 0;
                m_Data.stage = ShaderCompilationStages::UNKNOWN;
                m_pFile = nullptr;
                m_pMgr->_FreeShader( this );
            }
        }

        Result CShader::Compile()
        {
            VKE_ASSERT( m_pMgr, "Shader manager is not set." );
            Result res = VKE_OK;
            if( this->m_hDDIObject == DDI_NULL_HANDLE )
            {
                CShader* pThis = this;
                res = m_pMgr->_PrepareShaderTask( &pThis );
            }
            return res;
        }

        void CShader::_SetFile( Core::FilePtr pFile )
        {
            VKE_ASSERT( m_pFile.IsNull(), "File already set. Be sure a shader is properly created." );
            m_pFile = pFile;
            m_Data.pCode = pFile->GetData();
            m_Data.codeSize = pFile->GetDataSize();
            m_Data.stage = ShaderCompilationStages::HIGH_LEVEL_TEXT;
            _AddResourceState( Core::ResourceStates::LOADED );
        }


        CShaderProgram::CShaderProgram(CShaderManager* pMgr) :
            m_pMgr( pMgr )
        {
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
            _SetResourceState( Core::ResourceStates::INVALID );
            if( this->GetRefCount() == 0 )
            {
                //m_pMgr->_FreeProgram( this );
            }
        }

        void CShaderProgram::Init(const SShaderProgramDesc& Desc)
        {
            m_Desc = Desc;
            _SetResourceState( Core::ResourceStates::CREATED );
            SHash Hash;
            Hash += Core::CResource::CalcHash( Desc.Base );
            
            for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            {
                //h2 ^= ( CalcHash( Desc.apEntryPoints[ i ] ) << 1 );
                Hash += Desc.apEntryPoints[ i ];
                if( Desc.apShaders[ i ].IsValid() )
                {
                    //h3 ^= ( Desc.apShaders[ i ]->GetHandle() << 1 );
                    Hash += Desc.apShaders[ i ]->GetHandle().handle;
                }
            }

            this->m_hObject = Hash.value;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER