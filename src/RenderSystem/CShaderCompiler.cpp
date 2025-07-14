#include "RenderSystem/Vulkan/CShaderCompiler.h"
#if VKE_USE_GLSL_COMPILER
#include "ThirdParty/glslang/SPIRV/GlslangToSpv.h"

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

        struct CompilerOptions
        {
            enum TOptions
            {
                EOptionNone = 0,
                EOptionIntermediate = ( 1 << 0 ),
                EOptionSuppressInfolog = ( 1 << 1 ),
                EOptionMemoryLeakMode = ( 1 << 2 ),
                EOptionRelaxedErrors = ( 1 << 3 ),
                EOptionGiveWarnings = ( 1 << 4 ),
                EOptionLinkProgram = ( 1 << 5 ),
                EOptionMultiThreaded = ( 1 << 6 ),
                EOptionDumpConfig = ( 1 << 7 ),
                EOptionDumpReflection = ( 1 << 8 ),
                EOptionSuppressWarnings = ( 1 << 9 ),
                EOptionDumpVersions = ( 1 << 10 ),
                EOptionSpv = ( 1 << 11 ),
                EOptionHumanReadableSpv = ( 1 << 12 ),
                EOptionVulkanRules = ( 1 << 13 ),
                EOptionDefaultDesktop = ( 1 << 14 ),
                EOptionOutputPreprocessed = ( 1 << 15 ),
                EOptionOutputHexadecimal = ( 1 << 16 ),
                EOptionReadHlsl = ( 1 << 17 ),
                EOptionCascadingErrors = ( 1 << 18 ),
                EOptionAutoMapBindings = ( 1 << 19 ),
                EOptionFlattenUniformArrays = ( 1 << 20 ),
                EOptionNoStorageFormat = ( 1 << 21 ),
                EOptionKeepUncalled = ( 1 << 22 ),
                EOptionHlslOffsets = ( 1 << 23 ),
                EOptionHlslIoMapping = ( 1 << 24 ),
                EOptionAutoMapLocations = ( 1 << 25 ),
                EOptionDebug = ( 1 << 26 ),
                EOptionStdin = ( 1 << 27 ),
                EOptionOptimizeDisable = ( 1 << 28 ),
                EOptionOptimizeSize = ( 1 << 29 )
            };
        };
        const TBuiltInResource DefaultTBuiltInResource = {
            /* .MaxLights = */ 32,
            /* .MaxClipPlanes = */ 6,
            /* .MaxTextureUnits = */ 32,
            /* .MaxTextureCoords = */ 32,
            /* .MaxVertexAttribs = */ 64,
            /* .MaxVertexUniformComponents = */ 4096,
            /* .MaxVaryingFloats = */ 64,
            /* .MaxVertexTextureImageUnits = */ 32,
            /* .MaxCombinedTextureImageUnits = */ 80,
            /* .MaxTextureImageUnits = */ 32,
            /* .MaxFragmentUniformComponents = */ 4096,
            /* .MaxDrawBuffers = */ 32,
            /* .MaxVertexUniformVectors = */ 128,
            /* .MaxVaryingVectors = */ 8,
            /* .MaxFragmentUniformVectors = */ 16,
            /* .MaxVertexOutputVectors = */ 16,
            /* .MaxFragmentInputVectors = */ 15,
            /* .MinProgramTexelOffset = */ -8,
            /* .MaxProgramTexelOffset = */ 7,
            /* .MaxClipDistances = */ 8,
            /* .MaxComputeWorkGroupCountX = */ 65535,
            /* .MaxComputeWorkGroupCountY = */ 65535,
            /* .MaxComputeWorkGroupCountZ = */ 65535,
            /* .MaxComputeWorkGroupSizeX = */ 1024,
            /* .MaxComputeWorkGroupSizeY = */ 1024,
            /* .MaxComputeWorkGroupSizeZ = */ 64,
            /* .MaxComputeUniformComponents = */ 1024,
            /* .MaxComputeTextureImageUnits = */ 16,
            /* .MaxComputeImageUniforms = */ 8,
            /* .MaxComputeAtomicCounters = */ 8,
            /* .MaxComputeAtomicCounterBuffers = */ 1,
            /* .MaxVaryingComponents = */ 60,
            /* .MaxVertexOutputComponents = */ 64,
            /* .MaxGeometryInputComponents = */ 64,
            /* .MaxGeometryOutputComponents = */ 128,
            /* .MaxFragmentInputComponents = */ 128,
            /* .MaxImageUnits = */ 8,
            /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
            /* .MaxCombinedShaderOutputResources = */ 8,
            /* .MaxImageSamples = */ 0,
            /* .MaxVertexImageUniforms = */ 0,
            /* .MaxTessControlImageUniforms = */ 0,
            /* .MaxTessEvaluationImageUniforms = */ 0,
            /* .MaxGeometryImageUniforms = */ 0,
            /* .MaxFragmentImageUniforms = */ 8,
            /* .MaxCombinedImageUniforms = */ 8,
            /* .MaxGeometryTextureImageUnits = */ 16,
            /* .MaxGeometryOutputVertices = */ 256,
            /* .MaxGeometryTotalOutputComponents = */ 1024,
            /* .MaxGeometryUniformComponents = */ 1024,
            /* .MaxGeometryVaryingComponents = */ 64,
            /* .MaxTessControlInputComponents = */ 128,
            /* .MaxTessControlOutputComponents = */ 128,
            /* .MaxTessControlTextureImageUnits = */ 16,
            /* .MaxTessControlUniformComponents = */ 1024,
            /* .MaxTessControlTotalOutputComponents = */ 4096,
            /* .MaxTessEvaluationInputComponents = */ 128,
            /* .MaxTessEvaluationOutputComponents = */ 128,
            /* .MaxTessEvaluationTextureImageUnits = */ 16,
            /* .MaxTessEvaluationUniformComponents = */ 1024,
            /* .MaxTessPatchComponents = */ 120,
            /* .MaxPatchVertices = */ 32,
            /* .MaxTessGenLevel = */ 64,
            /* .MaxViewports = */ 16,
            /* .MaxVertexAtomicCounters = */ 0,
            /* .MaxTessControlAtomicCounters = */ 0,
            /* .MaxTessEvaluationAtomicCounters = */ 0,
            /* .MaxGeometryAtomicCounters = */ 0,
            /* .MaxFragmentAtomicCounters = */ 8,
            /* .MaxCombinedAtomicCounters = */ 8,
            /* .MaxAtomicCounterBindings = */ 1,
            /* .MaxVertexAtomicCounterBuffers = */ 0,
            /* .MaxTessControlAtomicCounterBuffers = */ 0,
            /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
            /* .MaxGeometryAtomicCounterBuffers = */ 0,
            /* .MaxFragmentAtomicCounterBuffers = */ 1,
            /* .MaxCombinedAtomicCounterBuffers = */ 1,
            /* .MaxAtomicCounterBufferSize = */ 16384,
            /* .MaxTransformFeedbackBuffers = */ 4,
            /* .MaxTransformFeedbackInterleavedComponents = */ 64,
            /* .MaxCullDistances = */ 8,
            /* .MaxCombinedClipAndCullDistances = */ 8,
            /* .MaxSamples = */ 4,
            /* .limits = */{
                /* .nonInductiveForLoops = */ 1,
                /* .whileLoops = */ 1,
                /* .doWhileLoops = */ 1,
                /* .generalUniformIndexing = */ 1,
                /* .generalAttributeMatrixVectorIndexing = */ 1,
                /* .generalVaryingIndexing = */ 1,
                /* .generalSamplerIndexing = */ 1,
                /* .generalVariableIndexing = */ 1,
                /* .generalConstantMatrixVectorIndexing = */ 1,
            } };

        struct SCompilerData
        {
            using ShaderBinaryData = vke_vector < uint32_t >;

            uint8_t             ShaderMemory[sizeof(glslang::TShader)];
            uint8_t				ProgramMemory[sizeof(glslang::TProgram)];
            glslang::TShader* pShader = nullptr;
            glslang::TProgram* pProgram = nullptr;

            ~SCompilerData()
            {
                Destroy();
            }

            void Create(EShLanguage lang)
            {
                pShader = ::new(ShaderMemory) glslang::TShader(lang);
                pProgram = ::new(ProgramMemory) glslang::TProgram();
            }

            void Destroy()
            {
                if (pProgram)
                {
                    pProgram->~TProgram();
                }
                if (pShader)
                {
                    pShader->~TShader();
                }
                pProgram = nullptr;
                pShader = nullptr;
            }
        };

        CShaderCompiler::CShaderCompiler(CShaderManager* pMgr) :
            m_pShaderMgr{ pMgr }
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
                glslang::FinalizeProcess();
                m_isCreated = false;
            }
        }

        Result CShaderCompiler::Create(const SShaderCompilerDesc& Desc)
        {
            Result res = VKE_FAIL;
            m_Desc = Desc;
            if( glslang::InitializeProcess() )
            {
                res = VKE_OK;
                m_isCreated = true;
            }
            return res;
        }

        Result CShaderCompiler::Compile(const SCompileShaderInfo& Info, SCompileShaderData* pOut)
        {
            Result ret = VKE_FAIL;
            EShLanguage type = g_aLanguages[Info.pDesc->type];
            //SCompilerData* pCompilerData = reinterpret_cast<SCompilerData*>(Info.pCompilerData);
            SCompilerData CompilerData;
            CompilerData.Create(type);

            CompilerData.pShader->setEntryPoint(Info.pDesc->EntryPoint.GetData());
            CompilerData.pShader->setStrings(&Info.pBuffer, 1);

            const bool isGLSL = Info.pBuffer[0] == '#';
            EShMessages useHLSL = EShMsgDefault;
            if (m_Desc.useHLSLSyntax && !isGLSL)
            {
                useHLSL = EShMsgReadHlsl;
            }
            const EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules | useHLSL);
            bool result = CompilerData.pShader->parse(&DefaultTBuiltInResource, 110, false, messages);
            if (result)
            {
                CompilerData.pProgram->addShader(CompilerData.pShader);
                result = CompilerData.pProgram->link(messages);
                if (result)
                {
                    result = CompilerData.pProgram->buildReflection();
                    if (result)
                    {
                        glslang::SpvOptions Options;
#if VKE_RENDER_SYSTEM_DEBUG
                        Options.disableOptimizer = true;
                        Options.generateDebugInfo = true;
                        Options.optimizeSize = false;
#else
                        Options.disableOptimizer = false;
                        Options.generateDebugInfo = false;
                        Options.optimizeSize = true;
#endif
                        spv::SpvBuildLogger Logger;
                        //VKE_LOG("dbg1: " << Info.pName);
                        glslang::TIntermediate* pIntermediate = CompilerData.pProgram->getIntermediate(type);
                        if (pIntermediate)
                        {
                            auto& vData = pOut->vShaderBinary;
                            vData.reserve(Config::RenderSystem::Shader::DEFAULT_SHADER_BINARY_SIZE);
                            //VKE_LOG("dbg2: " << Info.pName);
                            glslang::GlslangToSpv(*pIntermediate, vData, &Logger, &Options);
                            pOut->codeByteSize = static_cast<uint32_t>(sizeof(SCompileShaderData::BinaryElement) * vData.size());
                            //VKE_LOG( "dbg3: " << Info.pName );
                            //char tmp[ 4096 ]/*, tmp2[1024]*/;
                            //vke_sprintf( tmp, sizeof(tmp), "%s_%s.bin", Info.pName, Info.pEntryPoint );
                            //VKE_LOG( "dbg4: " << Info.pName );
                            //glslang::OutputSpvBin( vData, tmp );
                            //vke_sprintf( tmp, sizeof(tmp), "%s_%s.hex", Info.pName, Info.pEntryPoint );
                            //VKE_LOG( "dbg5: " << Info.pName );
                            //glslang::OutputSpvHex( vData, tmp, tmp );
                        }
#if VKE_RENDER_SYSTEM_DEBUG
                        //VKE_LOG("dbg6: " << Info.pName);
                        //VKE_LOG("Reflection for shader: " << Info.pName);
                        CompilerData.pProgram->dumpReflection();
                        //VKE_LOG("dbg7: " << Info.pName);
#endif // VKE_RENDER_SYSTEM_DEBUG
                        ret = VKE_OK;
                    }
                    else
                    {
                        VKE_LOG_ERR("Failed to build linker reflection.\n" << CompilerData.pProgram->getInfoLog());
                    }
                }
                else
                {
                    VKE_LOG_ERR("Failed to link shaders.\n" << CompilerData.pProgram->getInfoLog() <<
                        "\nEntry point: " << Info.pDesc->EntryPoint << "\n\n" << Info.pBuffer << "\n\n");
                }
            }
            else
            {
                VKE_LOG_ERR("\n---------------------------------------------------------------------\n" <<
                    "Compiile shader: " << Info.pDesc->Name << " failed.\n\n" << CompilerData.pShader->getInfoLog() <<
                    "\n---------------------------------------------------------------------\n\n");

            }
            VKE_ASSERT2(ret == VKE_OK, "");
            return ret;
        }

        Result CShaderCompiler::ConvertToBinary(const SLinkShaderData& LinkData, SShaderBinaryData* pOut)
        {
            Result res = VKE_FAIL;
            for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            {
                const SLinkShaderData::ShaderBinaryData& vData = LinkData.aShaderBinaries[ i ];
                const size_t dataSize = vData.size() * 4;
                VKE_ASSERT2( dataSize <= pOut->aBinarySizes[ i ], "Wrong output buffer size." );
                if( Memory::Copy( pOut->apBinaries[ i ], pOut->aBinarySizes[ i ], &vData[ 0 ], dataSize ) )
                {
                    res = VKE_OK;
                }
            }
            return res;
        }

        Result CShaderCompiler::WriteToHeaderFile(const char* pFileName, const SCompileShaderInfo& Info,
                                                  const SLinkShaderData& Data)
        {
            Result res = VKE_FAIL;
            const auto& vData = Data.aShaderBinaries[ Info.pDesc->type ];
            if( !vData.empty() )
            {
                glslang::OutputSpvHex( vData, pFileName, Info.pDesc->Name.GetData() );
                res = VKE_OK;
            }
            return res;
        }

        Result CShaderCompiler::WriteToBinaryFile(const char* pFileName, const SCompileShaderInfo& Info, const SLinkShaderData& Data)
        {
            Result res = VKE_FAIL;
            const auto& vData = Data.aShaderBinaries[ Info.pDesc->type];
            if( !vData.empty() )
            {
                WriteToBinaryFile( pFileName, vData );
                res = VKE_OK;
            }
            return res;
        }

        Result CShaderCompiler::WriteToBinaryFile(cstr_t pFileName, const std::vector<uint32_t>& vBinary )
        {
            cstr_t pExt = Platform::File::GetExtension( pFileName );
            if( strcmp( pExt, "spv" ) == 0 )
            {
                glslang::OutputSpvBin( vBinary, pFileName );
            }
            else
            {
                char buff[ 4096 ];
                vke_sprintf( buff, sizeof( buff ), "%s.spv", pFileName );
                glslang::OutputSpvBin( vBinary, buff );
            }
            return VKE_OK;
        }

    } // RenderSystem
} // VKE
#endif // VKE_USE_GLSL_COMPILER