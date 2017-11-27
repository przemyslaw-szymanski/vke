#include "RenderSystem/Vulkan/Managers/CShaderManager.h"
#if VKE_VULKAN_RENDERER
#include "Core/VKEConfig.h"
#include "RenderSystem/Vulkan/Resources/CShader.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CVkEngine.h"
#include "Core/Threads/CThreadPool.h"

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
                        auto& vpShaders = pGroup->Desc.vpShaders;
                        /*const uint32_t currDesc = DescRange.begin++;
                        if( currDesc < DescRange.end )
                        {
                        auto& Desc = vDescs[ currDesc ];
                        Desc.Create.async = false;
                        ShaderPtr pShader = pScheduler->pMgr->CreateShader( Desc );
                        vpShaders[ currDesc ] = pShader;
                        state = TaskStateBits::OK;
                        }*/
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
                                pGroup->Desc.pfnCallback( &tid, &pGroup->Desc );
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
            m_pCtx{ pCtx }
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
            return res;
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
                m_pCtx->GetRenderSystem()->GetEngine()->GetThreadPool()->AddTask( -1, pTask );
                return ShaderPtr();
            }
            else
            {
                return _CreateShaderTask( Desc );
            }
        }

        ShaderPtr CShaderManager::_CreateShaderTask(const SShaderCreateDesc& Desc)
        {
            ShaderPtr pRet;
            auto& Allocator = m_aShaderFreeListPools[ Desc.Shader.type ];
            Threads::SyncObject& SyncObj = m_aTaskSyncObjects[ Desc.Shader.type ];
            Threads::ScopedLock l( SyncObj );
            CShader* pShader = nullptr;
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
            if( pShader )
            {
                pShader->m_Desc = Desc.Shader;
                pShader->m_resourceState = ResourceStates::CREATED;
                pRet = ShaderPtr( pShader );
                if( Desc.Create.stages & ResourceStageBits::PREPARE )
                {
                    if( VKE_SUCCEEDED( PrepareShader( &pRet ) ) )
                    {
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
            Memory::DestroyObject( &Allocator, &pShader );
            return ShaderPtr();
        }

        Result CShaderManager::PrepareShader(ShaderPtr* pShader)
        {
            Result res = VKE_OK;

            return res;
        }

        Result CShaderManager::LoadShader(ShaderPtr* pShader)
        {
            Result res = VKE_OK;

            return res;
        }

        Result CShaderManager::Compile()
        {
            Result res = VKE_FAIL;

            {
                SShaderCreateDesc Desc;
                Desc.Create.async = true;
                Desc.Shader.type = ShaderTypes::VERTEX;
                Desc.Create.pfnCallback = [&](const void* pDesc, void* pShader)
                {
                    ShaderPtr pShader2 = *reinterpret_cast< ShaderPtr* >( pShader );
                };
                CreateShader( Desc );
            }
            {
                CThreadPool* pThreadPool = m_pCtx->GetRenderSystem()->GetEngine()->GetThreadPool();
                SShadersCreateDesc Descs;
                uint32_t count = 123;
                Descs.vCreateDescs.Resize( count );
                Descs.vpShaders.Resize( count );
                Descs.pfnCallback = []( const void* pTid, void* pDescs )
                {
                    const uint32_t tid = *( const uint32_t* )pTid;
                    SShadersCreateDesc* pDs = ( SShadersCreateDesc* )pDescs;
                };
                for( uint32_t i = 0; i < count; ++i )
                {
                    SShaderCreateDesc Desc;
                    Desc.Shader.type = ShaderTypes::VERTEX;
                    Descs.vCreateDescs[ i ] = Desc;
                }
                
                SShaderTaskGroups::SCreateGroup* pGroup;
                m_pShaderTaskGroups->CreateTaskGroup( this, &m_pShaderTaskGroups->CreateTaskBuffer, &pGroup );
                auto& Group = *pGroup;
                Group.Desc = Descs;
                //m_pShaderTaskGroups->InitGroup( this, &Group );
                uint32_t taskCount = Group.vTasks.GetCount();
                uint32_t shaderPart = count / taskCount;
                uint32_t shaderPartRest = count % taskCount;
                ExtentU16 Range;
                Range.begin = 0;
                Range.end = shaderPartRest;
                for( uint32_t i = 0; i < taskCount; ++i )
                {
                    Range.end += shaderPart;
                    auto& Task = Group.vTasks[ i ];
                    Task.DescRange = Range;
                    Range.begin = Range.end;
                    pThreadPool->AddTask( (int32_t)i, &Task );
                }
                //m_pShaderTaskGroup->m_CreateGroup.Restart();

                //m_pCtx->GetRenderSystem()->GetEngine()->GetThreadPool()->AddTaskGroup( &m_pShaderTaskGroup->m_CreateGroup );
            }
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