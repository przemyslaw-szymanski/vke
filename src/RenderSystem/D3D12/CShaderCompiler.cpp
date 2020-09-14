#if VKE_USE_DIRECTXSHADERCOMPILER
#include "RenderSystem/CShaderCompiler.h"
#include "dxc/dxcapi.h"
#include "dxc/Support/dxcapi.use.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SDxc
        {
            dxc::DxcDllSupport  Support;
            IDxcLibrary*        pLibrary = nullptr;
            IDxcCompiler*       pCompiler = nullptr;
        };
        static SDxc g_DXC;

        CShaderCompiler::CShaderCompiler( CShaderManager* pMgr ) :
            m_pShaderMgr{ pMgr }
        {
            
        }

        CShaderCompiler::~CShaderCompiler()
        {}

        void CShaderCompiler::Destroy()
        {

        }

        Result CShaderCompiler::Create( const SShaderCompilerDesc& Desc )
        {
            Result ret = VKE_FAIL;
            auto res = g_DXC.Support.Initialize();
            if( res == 0 )
            {
                IDxcCompiler* pCompiler;
                res = g_DXC.Support.CreateInstance( CLSID_DxcCompiler, &g_DXC.pCompiler );
                if( res == 0 )
                {
                    res = g_DXC.Support.CreateInstance( CLSID_DxcLibrary, &g_DXC.pLibrary );
                    if( res == 0 )
                    {
                        ret = VKE_OK;
                    }
                }
            }
            m_isCreated = true;
            return ret;
        }

        Result CShaderCompiler::Compile( const SCompileShaderInfo& Info, SCompileShaderData* pOut )
        {
            Result ret = VKE_FAIL;
            IDxcBlobEncoding* pBlob = nullptr;
            ::HRESULT res;
            if( Info.pFileName )
            {
                const wchar_t* pFileName = (const wchar_t*)Info.pFileName;
                uint32_t codePage;
                res = g_DXC.pLibrary->CreateBlobFromFile( pFileName, &codePage, &pBlob );
            }
            else
            {
                res = g_DXC.pLibrary->CreateBlobWithEncodingFromPinned( Info.pBuffer, Info.bufferSize, DXC_CP_ACP,
                                                                  &pBlob );
            }
            if( res == 0 )
            {
                wchar_t pName[ Config::Resource::MAX_NAME_LENGTH ];
                wchar_t pEntryPoint[ Config::Resource::MAX_NAME_LENGTH ];
                std::mbtowc( pBuff, Info.pName, Config::Resource::MAX_NAME_LENGTH );
                res = g_DXC.pCompiler->Compile(pBlob. pBuff, )
            }
            //auto res = g_DXC.pCompiler->Compile
            return ret;
        }
    }
}
#endif // VKE_USE_DIRECTXSHADERCOMPILER