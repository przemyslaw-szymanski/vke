#include "RenderSystem/Vulkan/CShaderCompiler.h"
#if VKE_VULKAN_RENDERER
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

        Result CShaderCompiler::Create(const SShaderCompilerDesc& /*Desc*/)
        {
            Result res = VKE_FAIL;
            if( glslang::InitializeProcess() )
            {
                res = VKE_OK;
                m_isCreated = true;
            }
            
            return res;
        }

        Result CShaderCompiler::Compile(const SCompileShaderInfo& Info, SCompileShaderData* pOut)
        {
            Result res = VKE_FAIL;
            assert( Info.pShader );

            Info.pShader->setStrings( &Info.pBuffer, 1 );
            Info.pShader->setEntryPoint( Info.pEntryPoint );
            EShMessages messages = static_cast< EShMessages >( EShMsgSpvRules | EShMsgVulkanRules );
            bool result = Info.pShader->parse( &DefaultTBuiltInResource, 110, false, messages );
            if( result )
			{
				// Link
				Info.pProgram->addShader( Info.pShader );
				EShMessages messages = static_cast<EShMessages>(EShMsgSpvRules | EShMsgVulkanRules);
				result = Info.pProgram->link(messages);
				if (result)
				{
					result = Info.pProgram->buildReflection();
					if (result)
					{
						glslang::SpvOptions Options;
#if VKE_RENDERER_DEBUG
						Options.disableOptimizer = true;
						Options.generateDebugInfo = true;
						Options.optimizeSize = false;
#else
						Options.disableOptimizer = false;
						Options.generateDebugInfo = false;
						Options.optimizeSize = true;
#endif
						spv::SpvBuildLogger Logger;
						EShLanguage lang = g_aLanguages[ Info.type ];
						glslang::TIntermediate* pIntermediate = Info.pProgram->getIntermediate(lang);
						if (pIntermediate)
						{
							auto& vData = pOut->vShaderBinary;
							vData.reserve(Config::Resource::Shader::DEFAULT_SHADER_BINARY_SIZE);
							glslang::GlslangToSpv(*pIntermediate, vData, &Logger, &Options);
						}
#if VKE_RENDERER_DEBUG
						Info.pProgram->dumpReflection();
#endif // VKE_RENDERER_DEBUG
						res = VKE_OK;
					}
					else
					{
						VKE_LOG_ERR("Failed to build linker reflection.\n" << Info.pProgram->getInfoLog());
					}
				}
				else
				{
					VKE_LOG_ERR("Failed to link shaders.\n" << Info.pProgram->getInfoLog());
				}
            }
            else
            {
                VKE_LOG_ERR("Compiile shader: " << Info.pName << " failed.\n" << Info.pShader->getInfoLog());
            }
            return res;
        }

        Result CShaderCompiler::ConvertToBinary(const SLinkShaderData& LinkData, SShaderBinaryData* pOut)
        {
            Result res = VKE_FAIL;
            for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            {
                const SLinkShaderData::ShaderBinaryData& vData = LinkData.aShaderBinaries[ i ];
                const size_t dataSize = vData.size() * 4;
                VKE_ASSERT( dataSize <= pOut->aBinarySizes[ i ], "Wrong output buffer size." );
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
            const auto& vData = Data.aShaderBinaries[ Info.type ];
            if( !vData.empty() )
            {
                glslang::OutputSpvHex( vData, pFileName, Info.pName );
                res = VKE_OK;
            }
            return res;
        }

        Result CShaderCompiler::WriteToBinaryFile(const char* pFileName, const SCompileShaderInfo& Info, const SLinkShaderData& Data)
        {
            Result res = VKE_FAIL;
            const auto& vData = Data.aShaderBinaries[ Info.type ];
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
#endif // VKE_VULKAN_RENDERER