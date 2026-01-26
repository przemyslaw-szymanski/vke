#include "RenderSystem/CShaderCompiler.h"

#if VKE_USE_DIRECTX_SHADER_COMPILER
#include <directx-dxc/dxcapi.h>

#include <codecvt>
#include <locale>

namespace VKE
{
#if VKE_WINDOWS
#define VKE_HR_OK S_OK
#define VKE_HR_FAIL S_FALSE
#else
#define VKE_HR_OK 0
#define VKE_HR_FAIL = -1
#endif

    namespace RenderSystem
    {
        struct SDXC
        {
            IDxcCompiler*            pCompiler            = nullptr;
            IDxcLibrary*             pLibrary             = nullptr;
            IDxcContainerReflection* pContainerReflection = nullptr;
            IDxcVersionInfo*         pVersionInfo         = nullptr;
            IDxcVersionInfo2*        pVersionInfo2        = nullptr;
            DxcCreateInstanceProc    DxcCreateInstance    = nullptr;
            DxcCreateInstance2Proc   DxcCreateInstance2   = nullptr;
            handle_t                 hDll;

            Result Initialize()
            {
                const char* pDllName;
#if _WIN32
                pDllName = "dxcompiler.dll";
#elif __APPLE__
                pDllName = "libdxcompiler.dylib";
#else
                pDllName = "libdxcompiler.so";
#endif
                hDll = Platform::DynamicLibrary::Load( pDllName );
                if( hDll != INVALID_HANDLE )
                {
                    DxcCreateInstance = static_cast< DxcCreateInstanceProc >(
                        Platform::DynamicLibrary::GetProcAddress( hDll, "DxcCreateInstance" ) );
                    DxcCreateInstance2 = static_cast< DxcCreateInstance2Proc >(
                        Platform::DynamicLibrary::GetProcAddress( hDll, "DxcCreateInstance2" ) );
                    if( DxcCreateInstance == nullptr )
                    {
                        Platform::DynamicLibrary::Close( hDll );
                        VKE_LOG_ERR( "Unable to load DxcCreateInstance function from dxcompiler library." );
                        return VKE_FAIL;
                    }
                    return VKE_OK;
                }
                VKE_LOG_ERR( "Unable to load dxcompiler dynamic library." );
                return VKE_FAIL;
            }

            template< class T >
            Result CreateInstance( REFCLSID clsid, T** ppOut )
            {
                HRESULT hr = DxcCreateInstance( clsid, __uuidof( T ), (LPVOID*)ppOut );
                return hr == VKE_HR_OK ? VKE_OK : VKE_FAIL;
            }
        };

        static SDXC g_DXC;

        CShaderCompiler::CShaderCompiler( CShaderManager* pMgr ) : m_pShaderMgr{ pMgr }
        {
        }

        CShaderCompiler::~CShaderCompiler()
        {
            Destroy();
        }

        void CShaderCompiler::Destroy()
        {
            if( m_isCreated )
            {
                g_DXC.pLibrary->Release();
                g_DXC.pCompiler->Release();
                m_isCreated = false;
            }
        }

        Result CShaderCompiler::Create( const SShaderCompilerDesc& Desc )
        {
            Result ret = VKE_FAIL;
            m_Desc     = Desc;
            ret        = g_DXC.Initialize();
            if( VKE_SUCCEEDED( ret ) )
            {
                if( VKE_FAILED( g_DXC.CreateInstance( CLSID_DxcCompiler, &g_DXC.pCompiler ) ) )
                {
                    VKE_LOG_ERR( "Unable to initialize DirectX ShaderCompiler." );
                    return VKE_FAIL;
                }
                if( VKE_FAILED( g_DXC.CreateInstance( CLSID_DxcCompiler, &g_DXC.pVersionInfo ) ) )
                {
                    VKE_LOG_ERR( "Unable to initialize DirectX ShaderCompiler." );
                    return VKE_FAIL;
                }
                if( VKE_FAILED( g_DXC.CreateInstance( CLSID_DxcCompiler, &g_DXC.pVersionInfo2 ) ) )
                {
                    VKE_LOG_ERR( "Unable to initialize DirectX ShaderCompiler." );
                    return VKE_FAIL;
                }
                if( VKE_FAILED( g_DXC.CreateInstance( CLSID_DxcLibrary, &g_DXC.pLibrary ) ) )
                {
                    VKE_LOG_ERR( "Unable to initialize DirectX ShaderCompiler." );
                    return VKE_FAIL;
                }
                if( VKE_FAILED( g_DXC.CreateInstance( CLSID_DxcContainerReflection, &g_DXC.pContainerReflection ) ) )
                {
                    VKE_LOG_ERR( "Unable to initialize DirectX ShaderCompiler." );
                    return VKE_FAIL;
                }
            }
            else
            {
                VKE_LOG_ERR( "Unable to initialize DirectX ShaderCompiler." );
            }
            return ret;
        }

        size_t vke_force_inline ConvertCSTRToWCSTR( wchar_t* pDst, uint32_t dstSize, cstr_t pSrc, uint32_t srcSize )
        {
            size_t ret;
            mbstowcs_s( &ret, pDst, dstSize, pSrc, srcSize );
            return ret;
        }

        // void GetProfile(SHADER_TYPE type, SHADER_PROFILE profile, wchar_t* pOut)
        //{
        //     static cwstr_t aTypes[ShaderTypes::_MAX_COUNT] =
        //     {
        //         L"vs",
        //         L"hs",
        //         L"ds",
        //         L"gs",
        //         L"ps",
        //         L"cs",
        //     };
        //     static cwstr_t aProfiles[ShaderProfiles::_MAX_COUNT] =
        //     {
        //         L"6_0",
        //         L"6_1",
        //         L"6_2",
        //         L"6_3",
        //         L"6_4",
        //     };

        //    //wchar_t* pOut = *ppOut;
        //    pOut[0] = aTypes[type][0];
        //    pOut[1] = aTypes[type][1];
        //    pOut[2] = L'_';
        //    pOut[3] = aProfiles[profile][0];
        //    pOut[4] = aProfiles[profile][1];
        //    pOut[5] = aProfiles[profile][2];
        //    pOut[6] = 0;
        //}

        // std::wstring GetProfile2(SHADER_TYPE type, SHADER_PROFILE profile)
        //{
        //     static cwstr_t aTypes[ShaderTypes::_MAX_COUNT] =
        //     {
        //         L"vs",
        //         L"hs",
        //         L"ds",
        //         L"gs",
        //         L"ps",
        //         L"cs",
        //     };
        //     static cwstr_t aProfiles[ShaderProfiles::_MAX_COUNT] =
        //     {
        //         L"6_0",
        //         L"6_1",
        //         L"6_2",
        //         L"6_3",
        //         L"6_4",
        //     };

        //    std::wstring ret;
        //    ret = aTypes[type] + std::wstring(L"_") + aProfiles[profile];
        //    return ret;
        //}

        SShaderDesc::NameWString GetProfile( SHADER_TYPE type, SHADER_PROFILE profile )
        {
            struct SShaderProfile
            {
                cwstr_t pType;
                cwstr_t pVer;
            };

            static cwstr_t aTypes[ ShaderTypes::_MAX_COUNT ] = {
                L"vs",  // vertex
                L"hs",  // hull
                L"ds",  // domain
                L"gs",  // geometry
                L"ps",  // pixel
                L"cs",  // compute
                L"as",  // task
                L"ms",  // mesh
                L"lib", // raytracing
            };
            static cwstr_t aProfiles[ ShaderProfiles::_MAX_COUNT ] = {
                L"6_0", L"6_1", L"6_2", L"6_3", L"6_4", L"6_5", L"6_6", L"6_7",
            };

            static SShaderProfile aShaderProfiles[ ShaderTypes::_MAX_COUNT ] = {
                { aTypes[ ShaderTypes::VERTEX ], aProfiles[ ShaderProfiles::PROFILE_6_0 ] },
                { aTypes[ ShaderTypes::HULL ], aProfiles[ ShaderProfiles::PROFILE_6_0 ] },
                { aTypes[ ShaderTypes::DOMAIN ], aProfiles[ ShaderProfiles::PROFILE_6_0 ] },
                { aTypes[ ShaderTypes::GEOMETRY ], aProfiles[ ShaderProfiles::PROFILE_6_0 ] },
                { aTypes[ ShaderTypes::PIXEL ], aProfiles[ ShaderProfiles::PROFILE_6_0 ] },
                { aTypes[ ShaderTypes::COMPUTE ], aProfiles[ ShaderProfiles::PROFILE_6_0 ] },
                { aTypes[ ShaderTypes::TASK ], aProfiles[ ShaderProfiles::PROFILE_6_5 ] },
                { aTypes[ ShaderTypes::MESH ], aProfiles[ ShaderProfiles::PROFILE_6_5 ] },
                { aTypes[ ShaderTypes::RAYGEN ], aProfiles[ ShaderProfiles::PROFILE_6_5 ] },
                { aTypes[ ShaderTypes::ANY_HIT ], aProfiles[ ShaderProfiles::PROFILE_6_5 ] },
                { aTypes[ ShaderTypes::CLOSEST_HIT ], aProfiles[ ShaderProfiles::PROFILE_6_5 ] },
                { aTypes[ ShaderTypes::MISS ], aProfiles[ ShaderProfiles::PROFILE_6_5 ] },
                { aTypes[ ShaderTypes::CALLABLE ], aProfiles[ ShaderProfiles::PROFILE_6_5 ] },
                { aTypes[ ShaderTypes::INTERSECTION ], aProfiles[ ShaderProfiles::PROFILE_6_5 ] },
            };

            SShaderDesc::NameWString ret;
            SShaderProfile           Prof;
            if( profile != SHADER_PROFILE::UNKNOWN )
            {
                VKE_ASSERT2( aTypes[ type ] != nullptr, "Shader type not implemented." );

                Prof.pType = aTypes[ type ];
                Prof.pVer  = aProfiles[ profile ];
            }
            else
            {
                Prof = aShaderProfiles[ type ];
            }
            VKE_ASSERT2( Prof.pType != nullptr, "Unknown shader type and profile." );
            if( Prof.pType != nullptr )
            {
                ret  = Prof.pType;
                ret += SShaderDesc::NameWString( L"_" );
                ret += Prof.pVer;
            }
            return ret;
        }

        void CopyBytecode( const void* pData, uint32_t dataSize, SCompileShaderData* pOut )
        {
            auto& vData        = pOut->vShaderBinary;
            pOut->codeByteSize = dataSize;

            vData.reserve( dataSize );
            const uint8_t* pSrc = (const uint8_t*)pData;
            const uint8_t* pEnd = pSrc + dataSize;

            while( pSrc != pEnd )
            {
                vData.push_back( *pSrc++ );
            }
        }

        /*std::string ToUtf8(const wchar_t* pStr)
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conv;
            return conv.to_bytes(pStr);
        }

        std::wstring ToUtf16(const char* pStr)
        {
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conv;
            return conv.from_bytes(pStr);
        }

        size_t ToUtf16(cstr_t pSrc, const uint32_t srcSize, wchar_t* pDst, const uint32_t dstLength)
        {
            size_t ret;
            mbstowcs_s(&ret, pDst, dstLength, pSrc, srcSize);
            return ret;
        }*/

        Result CShaderCompiler::Compile( const SCompileShaderInfo& Info, SCompileShaderData* pOut )
        {
            Result               ret = VKE_FAIL;
            IDxcBlobEncoding*    pTextBlob;
            IDxcOperationResult* pCompilerResult;

            const auto pText    = Info.pBuffer;
            const auto textSize = Info.bufferSize;
            HRESULT    res = g_DXC.pLibrary->CreateBlobWithEncodingFromPinned( pText, textSize, CP_UTF8, &pTextBlob );
            if( res == VKE_HR_OK )
            {
                vke_vector< const wchar_t* > vArgs;
                vArgs.reserve( 32 );

                vArgs.push_back( L"-Ges" );
                vArgs.push_back( L"-WX" );
                vArgs.push_back( L"-H" );
                vArgs.push_back( L"-Vi" );

                // TODO(blturkot): Support this to output debug info to a file, ideally bin/symbols/
                // vArgs.push_back( L"-Fd" );
#if VKE_RENDER_SYSTEM_DEBUG
                vArgs.push_back( L"-Zi" );
                vArgs.push_back( L"-Od" );
                // TODO(blturkot): compiler reports warning because theres no .pdb output specified. Requesting
                // explicitly embed_debug to prevent warning but eventually .pdb should be outputted to a file. (most
                // likely in bin/).
                vArgs.push_back( L"-Qembed_debug" );
#else
                vArgs.push_back( L"-O3" );
#endif
#if VKE_RENDER_SYSTEM_VULKAN
                vArgs.push_back( L"-spirv" );
                vArgs.push_back( L"-fvk-use-gl-layout" );
                vArgs.push_back( L"-fspv-target-env=vulkan1.2" );
                // vArgs.push_back(L"-fvk-bind-globals 128 0");
#endif

                const SShaderDesc::NameWString Name       = Info.pDesc->Name;
                const SShaderDesc::NameWString EntryPoint = ( Info.pDesc->EntryPoint );
                const SShaderDesc::NameWString Profile    = GetProfile( Info.pDesc->type, Info.pDesc->profile );

                const auto pName       = Name.GetData(); // Name.c_str();
                const auto pEntryPoint = EntryPoint.GetData();
                const auto pProfile    = Profile.GetData();

                Utils::TCDynamicArray< DxcDefine > vDefines;
                for( uint32_t i = 0; i < Info.pDesc->vDefines.GetCount(); ++i )
                {
                    DxcDefine Define;
                    Define.Name  = Info.pDesc->vDefines[ i ].Name.GetData();
                    Define.Value = Info.pDesc->vDefines[ i ].Value.GetData();
                    vDefines.PushBack( Define );
                }

                res = g_DXC.pCompiler->Compile( pTextBlob,
                                                pName,
                                                pEntryPoint,
                                                pProfile,
                                                &vArgs[ 0 ],
                                                (uint32_t)vArgs.size(),
                                                vDefines.GetData(),
                                                vDefines.GetCount(),
                                                nullptr,
                                                &pCompilerResult );
                if( res == VKE_HR_OK )
                {
                    pCompilerResult->GetStatus( &res );
                    IDxcBlobEncoding* pErrorBlob;
                    pCompilerResult->GetErrorBuffer( &pErrorBlob );
                    cstr_t pErr = (cstr_t)pErrorBlob->GetBufferPointer();
                    // Treat all warnings as errors
                    if( pErr )
                    {
                        VKE_LOG_ERR( "\n--------------------------------------------------------\n"
                                     << "Shader: \"" << Info.pDesc->Name.GetData() << "\" compilation errors:\n\n"
                                     << pErr << "\n--------------------------------------------------------\n" );
                        res = VKE_HR_FAIL;
                    }

                    if( res == VKE_HR_OK )
                    {
                        IDxcBlob* pBytecodeBlob;
                        pCompilerResult->GetResult( &pBytecodeBlob );
                        {
                            CopyBytecode(
                                pBytecodeBlob->GetBufferPointer(), (uint32_t)pBytecodeBlob->GetBufferSize(), pOut );
                        }
                        pBytecodeBlob->Release();
                        ret = VKE_OK;
                    }

                    pErrorBlob->Release();
                    pCompilerResult->Release();
                    pTextBlob->Release();
                }
            }
            VKE_ASSERT2( ret == VKE_OK, "" );
            return ret;
        }

        Result CShaderCompiler::ConvertToBinary( const SLinkShaderData& LinkData, SShaderBinaryData* pOut )
        {
            Result ret = VKE_FAIL;
            VKE_ASSERT2( ret == VKE_OK, "" );
            return ret;
        }

        Result CShaderCompiler::WriteToHeaderFile( const char* pFileName, const SCompileShaderInfo& Info,
                                                   const SLinkShaderData& Data )
        {
            Result ret = VKE_FAIL;
            VKE_ASSERT2( ret == VKE_OK, "" );
            return ret;
        }

        Result CShaderCompiler::WriteToBinaryFile( const char* pFileName, const SCompileShaderInfo& Info,
                                                   const SLinkShaderData& Data )
        {
            Result ret = VKE_FAIL;
            VKE_ASSERT2( ret == VKE_OK, "" );
            return ret;
        }

        Result CShaderCompiler::WriteToBinaryFile( cstr_t pFileName, const std::vector< uint32_t >& vBinary )
        {
            // cstr_t const pExt = Platform::File::GetExtension( pFileName );

            return VKE_OK;
        }

    } // namespace RenderSystem
} // namespace VKE
#endif // VKE_USE_DIRECTX_SHADER_COMPILER
