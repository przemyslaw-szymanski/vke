#include "RenderSystem/Vulkan/Managers/CShaderManager.h"
#if VKE_VULKAN_RENDERER
#include "Core/VKEConfig.h"
#include "RenderSystem/Vulkan/Resources/CShader.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CVkEngine.h"
#include "Core/Threads/CThreadPool.h"
#include "Core/Managers/CFileManager.h"
#include "Core/Math/Math.h"

#include <regex>

namespace VKE
{
    namespace RenderSystem
    {
        TaskState ShaderManagerTasks::SCreateShaderTask::_OnStart(uint32_t /*tid*/)
        {
            TaskState state = TaskStateBits::FAIL;
            pShader = pMgr->_CreateShaderTask( Desc );
            if( Desc.Create.pfnCallback )
            {
                Desc.Create.pfnCallback( &Desc.Shader, &pShader );
            }
            if( pShader.IsValid() )
            {
                state = TaskStateBits::OK;
            }
            return state;
        }

        void ShaderManagerTasks::SCreateShaderTask::_OnGet(void** ppOut)
        {
            ShaderPtr* ppShaderOut = reinterpret_cast< ShaderPtr* >( ppOut );
            *ppShaderOut = pShader;
        }

        TaskState ShaderManagerTasks::SCreateShadersTask::_OnStart(uint32_t /*tid*/)
        {
            TaskState state = TaskStateBits::FAIL;

            return state;
        }

        TaskState ShaderManagerTasks::SCreateProgramTask::_OnStart(uint32_t /*tid*/)
        {
            TaskState state = TaskStateBits::FAIL;

            return state;
        }

        void ShaderManagerTasks::SCreateProgramTask::_OnGet( void** /*ppOut*/ )
        {

        }

        struct SShaderTaskGroups
        {
            struct SCreateGroup
            {
                SShadersCreateDesc          Desc;
                CShaderManager::ShaderVec   vpShaders;
                CShaderManager*             pMgr = nullptr;
                uint32_t                    taskFinishedCount = 0;

                struct SCreateTask : public Threads::ITask
                {
                    SCreateGroup*   pGroup;
                    ExtentU16       DescRange;
                    bool            block = true;

                    TaskState _OnStart( uint32_t tid ) override
                    {
                        TaskState state = TaskStateBits::NOT_ACTIVE;
                        auto& vDescs = pGroup->Desc.vCreateDescs;
                        CShaderManager::ShaderVec& vpTmpShaders = pGroup->vpShaders;
                        if( block )
                        {
                            for( uint32_t i = DescRange.begin; i < DescRange.end; ++i )
                            {
                                SShaderCreateDesc& TmpDesc = vDescs[ i ];
                                TmpDesc.Create.async = false;
                                ShaderPtr pShader = pGroup->pMgr->CreateShader( TmpDesc );
                                vpTmpShaders[ i ] = pShader;
                                //state = TaskStateBits::OK;
                            }
                            pGroup->taskFinishedCount += ( DescRange.end - DescRange.begin );
                            if( pGroup->taskFinishedCount == pGroup->Desc.vCreateDescs.GetCount() )
                            {
                                pGroup->Desc.pfnCallback( &tid, &vpTmpShaders );
                            }
                        }
                        return state;
                    }
                };

                using TaskVec = Utils::TCDynamicArray< SCreateTask, 16 >;
                TaskVec vTasks;

                Result Init()
                {
                    return VKE_OK;
                }

                void FullClear()
                {
                    Desc.vCreateDescs.ClearFull();
                    vpShaders.ClearFull();
                    vTasks.ClearFull();
                }

                void Clear()
                {
                    Desc.vCreateDescs.Clear();
                    vpShaders.Clear();
                    vTasks.Clear();
                }
            };

            template<class GroupType>
            Result InitGroup(CShaderManager* pMgr, GroupType* pInOut)
            {
                GroupType& Group = *pInOut;
                Group.pMgr = pMgr;
                const uint32_t res = Group.vTasks.Resize( Platform::Thread::GetMaxConcurrentThreadCount() - 1 );
                if( res != Utils::INVALID_POSITION )
                {
                    for( uint32_t i = 0; i < Group.vTasks.GetCount(); ++i )
                    {
                        Group.vTasks[ i ].pGroup = &Group;
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to create memory for SCreateTask objects." );
                    return VKE_ENOMEMORY;
                }

                return Group.Init();
            }

            template<class GroupType>
            using TaskBuffer = Utils::TSFreePool< GroupType, GroupType* >;

            TaskBuffer< SCreateGroup > CreateTaskBuffer;

            template<class TasksBufferType, class TaskGroupType>
            Result CreateTaskGroup(CShaderManager* pMgr, TasksBufferType* pInOut, TaskGroupType** ppOut)
            {
                if( !pInOut->vFreeElements.PopBack( &(*ppOut) ) )
                {
                    TaskGroupType Group;
                    uint32_t idx = pInOut->vPool.PushBack( Group );
                    if( idx != Utils::INVALID_POSITION )
                    {
                        TaskGroupType* pGroup = &pInOut->vPool[ idx ];
                        InitGroup( pMgr, pGroup );
                        pInOut->vFreeElements.PushBack( pGroup );
                        return CreateTaskGroup( pMgr, pInOut, ppOut );
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to create memory for TaskGroup." );
                        return VKE_ENOMEMORY;
                    }
                }
                return VKE_OK;
            }

            template<class TasksBufferType, class TaskGroupType>
            void FreeTaskGroup(TasksBufferType* pInOut, TaskGroupType* pIn)
            {
                pInOut->vFreeElements.PushBack( pIn );
            }
        };

        CShaderManager::CShaderManager(CDeviceContext* pCtx) :
            m_pCtx{ pCtx },
            m_pFileMgr{ pCtx->GetRenderSystem()->GetEngine()->GetManagers().pFileMgr }
        {}

        CShaderManager::~CShaderManager()
        {}

        void CShaderManager::Destroy()
        {
            if( m_pCompiler )
            {
                //m_CreateShaderTaskPool.FullClear();
                //m_CreateProgramTaskPool.FullClear();

                auto& vPrograms = m_ProgramBuffer.vPool;
                for( uint32_t i = 0; i < vPrograms.GetCount(); ++i )
                {
                    auto& pProgram = vPrograms[ i ];
                    Memory::DestroyObject( &m_ShaderProgramFreeListPool, &pProgram );
                }
                m_ProgramBuffer.Clear();

                for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                {
                    ShaderBuffer& Buffer = m_aShaderBuffers[ i ];
                    auto& Allocator = m_aShaderFreeListPools[ i ];
                    //const uint32_t count = Buffer.vPool.GetCount();
                    const uint32_t count = Buffer.Buffer.vPool.GetCount();
                    for( uint32_t s = 0; s < count; ++s )
                    {
                        CShader* pPtr = Buffer.Buffer.vPool[ s ];
                        Memory::DestroyObject( &Allocator, &pPtr );
                    }
                    Buffer.Clear();
                }

                Memory::DestroyObject( &HeapAllocator, &m_pShaderTaskGroups );
                m_pCompiler->Destroy();
                Memory::DestroyObject( &HeapAllocator, &m_pCompiler );

                /*for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                {
                    m_aShaderFreeListPools[ i ].Destroy();
                }
                m_ShaderProgramFreeListPool.Destroy();*/
            }
        }

        Result CShaderManager::Create(const SShaderManagerDesc& Desc)
        {
            Result res = VKE_FAIL;
            m_Desc = Desc;
            res = VKE_OK;
            res = Memory::CreateObject( &HeapAllocator, &m_pCompiler, this );
            if( VKE_SUCCEEDED( res ) )
            {
                SShaderCompilerDesc CompilerDesc;
                res = m_pCompiler->Create( CompilerDesc );
                if( VKE_SUCCEEDED( res ) )
                {
                    m_Desc.aMaxShaderCounts[ ShaderTypes::VERTEX ] = Max( m_Desc.aMaxShaderCounts[ ShaderTypes::VERTEX ], Config::RenderSystem::Shader::MAX_VERTEX_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_HULL ] = Max( m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_HULL ], Config::RenderSystem::Shader::MAX_TESSELATION_HULL_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_DOMAIN ] = Max( m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_DOMAIN ], Config::RenderSystem::Shader::MAX_TESSELATION_DOMAIN_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::GEOMETRY ] = Max( m_Desc.aMaxShaderCounts[ ShaderTypes::GEOMETRY ], Config::RenderSystem::Shader::MAX_GEOMETRY_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::PIXEL ] = Max( m_Desc.aMaxShaderCounts[ ShaderTypes::PIXEL ], Config::RenderSystem::Shader::MAX_PIXEL_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::COMPUTE ] = Max( m_Desc.aMaxShaderCounts[ ShaderTypes::COMPUTE ], Config::RenderSystem::Shader::MAX_COMPUTE_SHADER_COUNT );
                    m_Desc.maxShaderProgramCount = Max( m_Desc.maxShaderProgramCount, Config::RenderSystem::Shader::MAX_SHADER_PROGRAM_COUNT );
                    
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
                            res = Memory::CreateObject( &HeapAllocator, &m_pShaderTaskGroups );
                            if( VKE_SUCCEEDED( res ) )
                            {
                                
                            }
                        }
                    }
                }
            }
            {
                // Default init
                SShaderManagerInitDesc Desc;
                res = Init( Desc );
            }
            return res;
        }

        Result CShaderManager::Init(const SShaderManagerInitDesc& Desc)
        {
            Result ret = VKE_OK;
            m_InitDesc = Desc;
            {
                auto& vExts = m_InitDesc.avShaderExtensions[ ShaderTypes::VERTEX ];
                if( vExts.IsEmpty() )
                {
                    vExts.PushBack( "vs" );
                }
            }
            {
                auto& vExts = m_InitDesc.avShaderExtensions[ ShaderTypes::COMPUTE ];
                if( vExts.IsEmpty() )
                {
                    vExts.PushBack( "cs" );
                }
            }
            {
                auto& vExts = m_InitDesc.avShaderExtensions[ ShaderTypes::GEOMETRY ];
                if( vExts.IsEmpty() )
                {
                    vExts.PushBack( "gs" );
                }
            }
            {
                auto& vExts = m_InitDesc.avShaderExtensions[ ShaderTypes::PIXEL ];
                if( vExts.IsEmpty() )
                {
                    vExts.PushBack( "ps" );
                }
            }
            {
                auto& vExts = m_InitDesc.avShaderExtensions[ ShaderTypes::TESS_DOMAIN ];
                if( vExts.IsEmpty() )
                {
                    vExts.PushBack( "ds" );
                }
            }
            {
                auto& vExts = m_InitDesc.avShaderExtensions[ ShaderTypes::TESS_HULL ];
                if( vExts.IsEmpty() )
                {
                    vExts.PushBack( "hs" );
                }
            }
            {
                auto& vExts = m_InitDesc.vProgramExtensions;
                if( vExts.IsEmpty() )
                {
                    vExts.PushBack( "shader" );
                    vExts.PushBack( "program" );
                }
            }
            return ret;
        }

        void CShaderManager::FreeUnusedResources()
        {
            for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            {
                auto& vpShaders = m_aShaderBuffers[ i ].Buffer.vPool;
                Threads::ScopedLock l( m_aShaderTypeSyncObjects[ i ] );
                for( uint32_t s = 0; s < vpShaders.GetCount(); ++s )
                {
                    if( vpShaders[ s ]->GetRefCount() == 1 )
                    {

                    }
                }
            }
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

        ShaderPtr CShaderManager::CreateShader(const SShaderCreateDesc& Desc)
        {
            if( Desc.Create.async )
            {
                ShaderManagerTasks::SCreateShaderTask* pTask;
                {
                    Threads::ScopedLock l( m_aTaskSyncObjects[ ShaderManagerTasks::CREATE_SHADER ] );
                    pTask = _GetTask( &m_CreateShaderTaskPool );
                }
                pTask->Desc = Desc;
                m_pCtx->GetRenderSystem()->GetEngine()->GetThreadPool()->AddTask( pTask );
                return ShaderPtr();
            }
            else
            {
                return _CreateShaderTask( Desc );
            }
        }

        SHADER_TYPE CShaderManager::FindShaderType(cstr_t pFileName)
        {
            cstr_t pExt = strrchr(pFileName, '.' );
            if( pExt )
            {
                cstr_t pFileExt = pExt + 1;
                for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                {
                    for( uint32_t j = 0; j < m_InitDesc.avShaderExtensions[ i ].GetCount(); ++j )
                    {
                        if( strcmp( pFileExt, m_InitDesc.avShaderExtensions[ i ][ j ] ) == 0 )
                        {
                            return static_cast< SHADER_TYPE >( i );
                        }
                    }
                }
            }
            return ShaderTypes::_MAX_COUNT;
        }

        ShaderPtr CShaderManager::_CreateShaderTask(const SShaderCreateDesc& Desc)
        {
            ShaderPtr pRet;
            VKE_ASSERT( Desc.Shader.type < ShaderTypes::_MAX_COUNT, "Shader type must be a enum type." );
            if( Desc.Shader.type >= ShaderTypes::_MAX_COUNT )
            {
                VKE_LOG_ERR( "Invalid shader type:" << Desc.Shader.type );
                return pRet;
            }
            auto& Allocator = m_aShaderFreeListPools[ Desc.Shader.type ];
            Threads::SyncObject& SyncObj = m_aShaderTypeSyncObjects[ Desc.Shader.type ];
            CShader* pShader = nullptr;
            {
                Threads::ScopedLock l( SyncObj );
                ShaderBuffer& Buffer = m_aShaderBuffers[ Desc.Shader.type ];
                /*if( !Buffer.vFreeElements.PopBack( &pShader ) )
                {
                    if( VKE_SUCCEEDED( Memory::CreateObject( &Allocator, &pShader, this, Desc.Shader.type ) ) )
                    {
                        Buffer.vPool.PushBack( pShader );
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to allocate memory for CShader object." );
                    }
                }*/
                ShaderBuffer::MapIterator Itr;
                hash_t hash = CShader::CalcHash( Desc.Shader );
                if( Buffer.Get( hash, &pShader, &Itr ) )
                {

                }
                else
                {
                    if( VKE_SUCCEEDED( Memory::CreateObject( &Allocator, &pShader, this, Desc.Shader.type ) ) )
                    {
                        if( Buffer.Add( pShader, hash, Itr ) )
                        {

                        }
                        else
                        {
                            VKE_LOG_ERR( "Unable to allocate memory for CShader object." );
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to allocate memory for CShader object." );
                    }
                }
            }
            if( pShader )
            {
                const uint32_t resState = pShader->GetResourceState();

                if( Desc.Create.stages & ResourceStageBits::INIT )
                {
                    pShader->Init( Desc.Shader );
                }
                pRet = ShaderPtr( pShader );
                if( Desc.Create.stages & ResourceStageBits::LOAD )
                {
                    if( VKE_SUCCEEDED( LoadShader( &pRet ) ) )
                    {
                    }
                    else
                    {
                        goto FAIL;
                    }
                }
                if( Desc.Create.stages & ResourceStageBits::PREPARE )
                {
                    if( VKE_SUCCEEDED( PrepareShader( &pRet ) ) )
                    {

                    }
                    else
                    {
                        goto FAIL;
                    }
                }
                
            }
            return pRet;
        FAIL:
            pRet.Release();
            _FreeShader( pShader );
            return pRet;
        }

        Result CShaderManager::PrepareShader(ShaderPtr* ppShader)
        {
            CShader* pShader = ( *ppShader ).Get();
            return _PrepareShaderTask( &pShader );
        }

        Result CShaderManager::LoadShader(ShaderPtr* ppShader)
        {
            CShader* pShader = ( *ppShader ).Get();
            return _LoadShaderTask( &pShader );
        }

        Result CShaderManager::_LoadShaderTask(CShader** ppShader)
        {
            Result res = VKE_FAIL;
            CShader* pShader = ( *ppShader );
            Threads::ScopedLock l( pShader->m_SyncObj );
            if( !(pShader->GetResourceState() & ResourceStates::LOADED ) )
            {
                Core::SFileCreateDesc Desc;
                Desc.File.Base = pShader->m_Desc.Base;
                FilePtr pFile = m_pFileMgr->LoadFile( Desc );
                if( pFile.IsValid() )
                {
                    VKE_ASSERT( pShader->m_pFile.IsNull(), "Current file must be released." );
                    pShader->m_pFile = ( pFile );
                    pShader->m_resourceState |= ResourceStates::LOADED;
                    res = VKE_OK;
                }
            }
            else
            {
                res = VKE_OK;
            }
            return res;
        }

        Result _PreprocessIncludes(Core::CFileManager* pFileMgr, cstr_t pBaseDirPath,
            cstr_t pCode, vke_string& strLine, vke_string* pStrOutput)
        {
            std::smatch Match;

            static const std::regex Regex("^[ ]*#[ ]*include[ ]+[\"<](.*)[\">].*");
            char pFullFilePath[ 1024 ] = { 0 };
            std::stringstream ssCode( pCode );

            while( std::getline( ssCode, strLine ) )
            {
                if( std::regex_search( strLine, Match, Regex ) )
                {
                    const std::string& strMatch = Match[ 1 ];
                    Core::SFileCreateDesc Desc;

                    uint32_t fileNameLen = vke_sprintf( pFullFilePath, sizeof( pFullFilePath ), "%s\\%s", pBaseDirPath, strMatch.c_str() );
                    
                    Desc.File.Base.pFileName = pFullFilePath;
                    Desc.File.Base.fileNameLen = fileNameLen;
                    Desc.File.Base.pName = strMatch.c_str();
                    Desc.File.Base.nameLen = strMatch.length();
                    FilePtr pFile = pFileMgr->LoadFile( Desc );
                    if( pFile.IsValid() )
                    {
                        cstr_t pTmpCode = reinterpret_cast< cstr_t >( pFile->GetData() );
                        _PreprocessIncludes( pFileMgr, pBaseDirPath, pTmpCode, strLine, pStrOutput );
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to include file: " << strMatch.c_str() );
                    }
                }
                else
                {
                    pStrOutput->append(strLine);
                }
            }
            pStrOutput->append( "\n" );
            return VKE_OK;
        }

        Result CShaderManager::_PrepareShaderTask(CShader** ppShader)
        {
            Result res = VKE_OK;
            CShader* pShader = ( *ppShader );
            Threads::ScopedLock l( pShader->m_SyncObj );
            if( !( pShader->GetResourceState() & ResourceStates::PREPARED ) )
            {
                // Add preprocessor and includes
                cstr_t pShaderData = reinterpret_cast< cstr_t >( pShader->m_pFile->GetData() );
                uint32_t shaderDataSize = pShader->m_pFile->GetDataSize();
                
                const SShaderDesc& Desc = pShader->GetDesc();
                
                vke_string strCode, strLine;
                strCode.reserve(1024 * 1024 * 1);
                char fileDir[1024];
                char* pFileDir = fileDir;
                Platform::File::GetDirectory( Desc.Base.pFileName, Desc.Base.fileNameLen, &pFileDir );

                res = _PreprocessIncludes( m_pFileMgr, pFileDir, pShaderData, strLine, &strCode );
                const char* strings[] = { "#define test 1" };
                pShader->GetCompilerData().pShader->setStrings(strings, 1);
                if( VKE_SUCCEEDED( res ) )
                {
                    pShaderData = strCode.c_str();
                    shaderDataSize = strCode.length();

                    SCompileShaderInfo Info;
                    Info.pShader = pShader->GetCompilerData().pShader;
                    Info.pProgram = pShader->GetCompilerData().pProgram;
                    Info.pBuffer = pShaderData;
                    Info.bufferSize = shaderDataSize;
                    Info.type = pShader->m_Desc.type;
                    Info.pEntryPoint = pShader->m_Desc.pEntryPoint;
                    VKE_ASSERT(Info.pBuffer, "Shader file must be loaded.");
                    SCompileShaderData Data;
                    if (VKE_SUCCEEDED((res = m_pCompiler->Compile(Info, &Data))))
                    {
                        pShader->m_resourceState |= ResourceStates::PREPARED;
                        res = _CreateShaderModule(&Data.vShaderBinary[0], Data.vShaderBinary.size(), &pShader);
                    }
                    pShader->m_pFile = FileRefPtr();
                }
            }
            return res;
        }

        Result CShaderManager::CreateShaders(const SShadersCreateDesc& Desc, ShaderVec* pvOut)
        {
            Result res = VKE_OK;
            if( Desc.taskCount > 0 )
            {
                CThreadPool* pThreadPool = m_pCtx->GetRenderSystem()->GetEngine()->GetThreadPool();
                SShaderTaskGroups::SCreateGroup* pGroup;
                m_pShaderTaskGroups->CreateTaskGroup( this, &m_pShaderTaskGroups->CreateTaskBuffer, &pGroup );
                const uint32_t shaderCount = Desc.vCreateDescs.GetCount();
                pGroup->Desc = Desc;
                std::sort( &pGroup->Desc.vCreateDescs.Front(), &pGroup->Desc.vCreateDescs.Back(),
                           [](const SShaderCreateDesc& Left, const SShaderCreateDesc& Right)
                {
                    return Left.Shader.type < Right.Shader.type;
                } );
                pGroup->vpShaders.Resize( shaderCount );
           
                uint32_t taskCount = ( Desc.taskCount == 0 ) ? pGroup->vTasks.GetCount() : Desc.taskCount;
                uint32_t groupSize = shaderCount / taskCount;
                uint32_t groupSizeTail = shaderCount % taskCount;
                ExtentU16 Range;
                Range.begin = 0;
                Range.end = static_cast< uint16_t >( groupSizeTail );
                for( uint32_t i = 0; i < taskCount; ++i )
                {
                    Range.end += static_cast< uint16_t >( groupSize );
                    auto& Task = pGroup->vTasks[ i ];
                    Task.DescRange = Range;
                    Range.begin = Range.end;
                    pThreadPool->AddTask( &Task );
                }
            }
            else
            {
                res = VKE_OK;
                auto& vpShaders = *pvOut;
                vpShaders.Resize( Desc.vCreateDescs.GetCount() );
                for( uint32_t i = 0; i < Desc.vCreateDescs.GetCount(); ++i )
                {
                    ShaderPtr pShader = _CreateShaderTask( Desc.vCreateDescs[ i ] );
                    if( pShader.IsValid() )
                    {
                        vpShaders[ i ] = pShader;
                    }
                    else
                    {
                        res = VKE_FAIL;
                        break;
                    }
                }
            }
            return res;
        }

        Result CShaderManager::Compile()
        {
            Result res = VKE_FAIL;

            SShaderCreateDesc Desc;
            Desc.Create.async = true;
            Desc.Create.pfnCallback = []( const void*, void* )
            {};
            Desc.Shader.Base.pFileName = "data\\shaders\\test.vs";
            Desc.Shader.Base.fileNameLen = strlen( Desc.Shader.Base.pFileName );
            Desc.Shader.type = FindShaderType( Desc.Shader.Base.pFileName );
            Desc.Shader.vPreprocessor.PushBack("#define TEST 1");
            Desc.Shader.vPreprocessor.PushBack("#define TEST2 2");
            ShaderPtr pShader = CreateShader( Desc );
            return res;
        }

        Result CShaderManager::Link()
        {
            Result res = VKE_FAIL;
            //SLinkShaderInfo Info;
            //res = Memory::CreateObject( &m_ShaderProgramFreeListPool, &Info.pProgram );
            //if( VKE_SUCCEEDED( res ) )
            //{
            //    //Memory::Copy< glslang::TShader*, ShaderTypes::_MAX_COUNT >( Info.apShaders, m_CurrCompilationUnit.apShaders );
            //    for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            //    {
            //        Info.apShaders[ i ] = m_CurrCompilationUnit.aInfos[ i ].pShader;
            //    }
            //    SLinkShaderData Data;
            //    res = m_pCompiler->Link( Info, &Data );
            //}
            //else
            //{
            //    VKE_LOG_ERR( "Unable to allocate memory for glslang::TProgram object." );
            //}
            return res;
        }

        void CShaderManager::_FreeShader(CShader* pShader)
        {
            //if( pShader->GetRefCount() == 0 )
            {
                SHADER_TYPE type = pShader->m_Desc.type;
                VKE_ASSERT( type < ShaderTypes::_MAX_COUNT, "Invalid shader type." );
                Threads::ScopedLock l( m_aShaderTypeSyncObjects[ type ] );
                {
                    m_pCtx->_GetDevice().DestroyObject( nullptr, &pShader->m_vkModule );
                    //m_aShaderBuffers[ type ].vFreeElements.PushBack( pShader );
                    m_aShaderBuffers[ type ].Free( pShader->GetResourceHash() );
                }
            }
        }

        void* CShaderManager::_AllocateMemory(size_t size, size_t alignment)
        {
            const size_t alignedSize = Math::Round( size, alignment );
            void* pMem = VKE_MALLOC( alignedSize );
            return pMem;
        }

        void CShaderManager::_FreeMemory(void* /*pMemory*/, size_t /*size*/, size_t /*alignment*/)
        {

        }

        /*typedef void* (VKAPI_PTR *PFN_vkAllocationFunction)(
    void*                                       pUserData,
    size_t                                      size,
    size_t                                      alignment,
    VkSystemAllocationScope                     allocationScope);*/
        void* VkAllocateCallback(void* pUser, size_t size, size_t alignment, VkSystemAllocationScope /*vkScope*/)
        {
            CShaderManager* pMgr = reinterpret_cast< CShaderManager* >( pUser );
            return pMgr->_AllocateMemory(size, alignment);
        }

        Result CShaderManager::_CreateShaderModule(const uint32_t* pBinary, size_t size, CShader** ppInOut)
        {
            Result res = VKE_FAIL;
            const uint32_t codeSize = static_cast< uint32_t >( size * sizeof( uint32_t ) );
            VKE_ASSERT( pBinary && codeSize > 0 && codeSize % 4 == 0, "Invalid shader binary." );
            {
                CShader* pShader = ( *ppInOut );
                auto& Device = m_pCtx->_GetDevice();
                
                VkShaderModuleCreateInfo ci;
                Vulkan::InitInfo( &ci, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO );
                ci.pCode = pBinary;
                ci.codeSize = codeSize;
                ci.flags = 0;
                VkShaderModule vkModule = VK_NULL_HANDLE;
                //VkResult vkRes = Device.CreateObject( &ci, nullptr, &vkModule );
                /*VkAllocationCallbacks VkCallbacks;
                VkCallbacks.pUserData = this;
                VkCallbacks.pfnAllocation = VkAllocateCallback;*/
                VkResult vkRes = Device.GetICD().vkCreateShaderModule( Device.GetHandle(), &ci, nullptr, &vkModule );
                if( vkRes == VK_SUCCESS )
                {
                    pShader->m_vkModule = vkModule;
                    res = VKE_OK;
                }
                else
                {
                    VK_ERR( vkRes );
                    VKE_LOG_ERR( "Error while creating vkShaderModule: " << vkRes );
                }
            }
            return res;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER