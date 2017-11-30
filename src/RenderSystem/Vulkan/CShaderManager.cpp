#include "RenderSystem/Vulkan/Managers/CShaderManager.h"
#if VKE_VULKAN_RENDERER
#include "Core/VKEConfig.h"
#include "RenderSystem/Vulkan/Resources/CShader.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CVkEngine.h"
#include "Core/Threads/CThreadPool.h"
#include "Core/Managers/CFileManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        TaskState ShaderManagerTasks::SCreateShaderTask::_OnStart(uint32_t tid)
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

        TaskState ShaderManagerTasks::SCreateShadersTask::_OnStart(uint32_t tid)
        {
            TaskState state = TaskStateBits::FAIL;

            return state;
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
                        auto& vpShaders = pGroup->vpShaders;
                        if( block )
                        {
                            for( uint32_t i = DescRange.begin; i < DescRange.end; ++i )
                            {
                                auto& Desc = vDescs[ i ];
                                Desc.Create.async = false;
                                ShaderPtr pShader = pGroup->pMgr->CreateShader( Desc );
                                vpShaders[ i ] = pShader;
                                //state = TaskStateBits::OK;
                            }
                            pGroup->taskFinishedCount += ( DescRange.end - DescRange.begin );
                            if( pGroup->taskFinishedCount == pGroup->Desc.vCreateDescs.GetCount() )
                            {
                                pGroup->Desc.pfnCallback( &tid, &vpShaders );
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
            };

            template<class GroupType>
            Result InitGroup(CShaderManager* pMgr, GroupType* pInOut)
            {
                GroupType& Group = *pInOut;
                Group.pMgr = pMgr;
                auto res = Group.vTasks.Resize( Platform::Thread::GetMaxConcurrentThreadCount() - 1 );
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
                for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                {
                    ShaderBuffer& Buffer = m_aShaderBuffers[ i ];
                    auto& Allocator = m_aShaderFreeListPools[ i ];
                    const uint32_t count = Buffer.vPool.GetCount();
                    for( uint32_t s = 0; s < count; ++s )
                    {
                        Memory::DestroyObject( &Allocator, &Buffer.vPool[ s ] );
                    }
                    Buffer.vPool.Clear();
                    Buffer.vFreeElements.Clear();
                }

                Memory::DestroyObject( &HeapAllocator, &m_pShaderTaskGroups );
                m_pCompiler->Destroy();
                Memory::DestroyObject( &HeapAllocator, &m_pCompiler );

                for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                {
                    m_aShaderFreeListPools[ i ].Destroy();
                }
                m_ShaderProgramFreeListPool.Destroy();
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
                    m_Desc.aMaxShaderCounts[ ShaderTypes::VERTEX ] = max( m_Desc.aMaxShaderCounts[ ShaderTypes::VERTEX ], Config::Resource::Shader::MAX_VERTEX_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_HULL ] = max( m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_HULL ], Config::Resource::Shader::MAX_TESSELATION_HULL_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_DOMAIN ] = max( m_Desc.aMaxShaderCounts[ ShaderTypes::TESS_DOMAIN ], Config::Resource::Shader::MAX_TESSELATION_DOMAIN_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::GEOMETRY ] = max( m_Desc.aMaxShaderCounts[ ShaderTypes::GEOMETRY ], Config::Resource::Shader::MAX_GEOMETRY_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::PIXEL ] = max( m_Desc.aMaxShaderCounts[ ShaderTypes::PIXEL ], Config::Resource::Shader::MAX_PIXEL_SHADER_COUNT );
                    m_Desc.aMaxShaderCounts[ ShaderTypes::COMPUTE ] = max( m_Desc.aMaxShaderCounts[ ShaderTypes::COMPUTE ], Config::Resource::Shader::MAX_COMPUTE_SHADER_COUNT );
                    m_Desc.maxShaderProgramCount = max( m_Desc.maxShaderProgramCount, Config::Resource::Shader::MAX_SHADER_PROGRAM_COUNT );
                    
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
                    _GetTask( &m_CreateShaderTaskPool, &pTask );
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
                if( !Buffer.vFreeElements.PopBack( &pShader ) )
                {
                    if( VKE_SUCCEEDED( Memory::CreateObject( &Allocator, &pShader, Desc.Shader.type ) ) )
                    {
                        Buffer.vPool.PushBack( pShader );
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to allocate memory for CShader object." );
                    }
                }
            }
            if( pShader )
            {
                pShader->m_Desc = Desc.Shader;
                pShader->m_resourceState = ResourceStates::CREATED;
                pRet = ShaderPtr( pShader );
                if( Desc.Create.stages & ResourceStageBits::LOAD )
                {
                    if( VKE_SUCCEEDED( LoadShader( &pRet ) ) )
                    {
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
                        else
                        {
                            goto FAIL;
                        }
                    }
                    else
                    {
                        goto FAIL;
                    }
                }
                
            }
            return pRet;
        FAIL:
            {
                Threads::ScopedLock l( SyncObj );
                Memory::DestroyObject( &Allocator, &pShader );
                return ShaderPtr();
            }
        }

        Result CShaderManager::PrepareShader(ShaderPtr* ppShader)
        {
            return _PrepareShaderTask( ppShader );
        }

        Result CShaderManager::LoadShader(ShaderPtr* ppShader)
        {
            return _LoadShaderTask( ppShader );
        }

        Result CShaderManager::_LoadShaderTask(ShaderPtr* ppShader)
        {
            Result res = VKE_FAIL;
            CShader* pShader = ( *ppShader ).Get();
            SFileDesc Desc;
            Desc.Base = pShader->m_Desc.Base;
            FilePtr pFile = m_pFileMgr->Load( Desc );
            if( pFile.IsValid() )
            {
                VKE_ASSERT( pShader->m_pFile.IsNull(), "Current file must be released." );
                pShader->m_pFile = ( pFile );
                pShader->m_resourceState = ResourceStates::LOADED;
                res = VKE_OK;
            }
            return res;
        }

        Result CShaderManager::_PrepareShaderTask(ShaderPtr* ppShader)
        {
            Result res = VKE_FAIL;
            CShader* pShader = ( *ppShader ).Get();
            SCompileShaderInfo Info;
            Info.pShader = &pShader->m_Shader;
            Info.pBuffer = reinterpret_cast< cstr_t >( pShader->m_pFile->GetData() );
            Info.bufferSize = pShader->m_pFile->GetDataSize();
            Info.type = pShader->m_Desc.type;
            VKE_ASSERT( Info.pBuffer, "Shader file must be loaded." );
            SCompileShaderData Data;
            if( VKE_SUCCEEDED( m_pCompiler->Compile( Info, &Data ) ) )
            {
                res = VKE_OK;
            }
            else
            {

            }
            m_pFileMgr->FreeFile( &pShader->m_pFile );
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
                Range.end = groupSizeTail;
                for( uint32_t i = 0; i < taskCount; ++i )
                {
                    Range.end += groupSize;
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
            Desc.Create.pfnCallback = [](const void*, void*)
            {};
            Desc.Shader.Base.pFileName = "d:\\Workspace\\projects\\vke\\samples\\data\\shaders\\test.vs";
            Desc.Shader.type = FindShaderType( Desc.Shader.Base.pFileName );
            ShaderPtr pShader = CreateShader( Desc );
            return res;
        }

        Result CShaderManager::Link()
        {
            Result res = VKE_FAIL;
            SLinkShaderInfo Info;
            res = Memory::CreateObject( &m_ShaderProgramFreeListPool, &Info.pProgram );
            if( VKE_SUCCEEDED( res ) )
            {
                //Memory::Copy< glslang::TShader*, ShaderTypes::_MAX_COUNT >( Info.apShaders, m_CurrCompilationUnit.apShaders );
                for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                {
                    Info.apShaders[ i ] = m_CurrCompilationUnit.aInfos[ i ].pShader;
                }
                SLinkShaderData Data;
                res = m_pCompiler->Link( Info, &Data );
            }
            else
            {
                VKE_LOG_ERR("Unable to allocate memory for glslang::TProgram object.");
            }
            return res;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER