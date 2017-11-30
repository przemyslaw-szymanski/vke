#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "Core/Memory/TCFreeListManager.h"
#include "RenderSystem/Vulkan/CShaderCompiler.h"
#include "Core/Threads/ITask.h"
#include "Core/Threads/CTaskGroup.h"
#include "Core/Resources/CResource.h"

namespace glslang
{
    class TShader;
} // glslang

namespace VKE
{
    namespace Core
    {
        class CFileManager;
    } // Core

    namespace RenderSystem
    {
        struct SShaderManagerDesc
        {
            uint32_t    aMaxShaderCounts[ ShaderTypes::_MAX_COUNT ] = { 0 };
            uint32_t    maxShaderProgramCount = 0;
        };

        struct SShaderManagerInitDesc
        {
            using StringVec = Utils::TCDynamicArray< cstr_t, 4 >;
            using StringVecArray = StringVec[ ShaderTypes::_MAX_COUNT ];
            StringVecArray  avShaderExtensions;
            StringVec       vProgramExtensions;
        };

        struct SShaderCreateDesc
        {
            SResourceCreateDesc Create;
            SShaderDesc         Shader;
        };

        struct SShaderProgramCreateDesc
        {
            SResourceCreateDesc Create;
            SShaderProgramDesc  Program;
        };

        struct SShadersCreateDesc
        {
            using CreateDescVec = Utils::TCDynamicArray< SShaderCreateDesc >;
            using ShaderVec = Utils::TCDynamicArray< ShaderPtr >;

            CreateDescVec               vCreateDescs;
            Resources::CreateCallback   pfnCallback;
            uint8_t                     taskCount = 0;
        };

        struct SShaderLoadDesc
        {

        };

        class CShader;
        class CShaderProgram;
        class CShaderCompiler;
        class CShaderManager;

        struct ShaderManagerTasks
        {
            enum
            {
                CREATE_SHADER,
                CREATE_SHADERS,
                _MAX_COUNT
            };

            struct SCreateShaderTask : public Threads::ITask
            {
                friend class CShaderManager;
                CShaderManager*     pMgr = nullptr;
                SShaderCreateDesc   Desc;
                ShaderPtr           pShader;

                TaskState _OnStart(uint32_t tid) override;
                void _OnGet(void**) override;
            };

            struct SCreateShadersTask : public Threads::ITask
            {
                friend class CShaderManager;
                CShaderManager*     pMgr = nullptr;
                SShadersCreateDesc  Desc;

                TaskState   _OnStart(uint32_t tid) override;
                void _OnGet(void**) override;
            };

            struct SCreateProgramTask : public Threads::ITask
            {
                friend class CShaderManager;
                CShaderManager*             pMgr = nullptr;
                SShaderProgramCreateDesc    Desc;
                ShaderProgramPtr            pProgram;

                TaskState _OnStart(uint32_t tid) override;
                void _OnGet(void**) override;
            };

        }; // ShaderManagerTasks

        struct SShaderTaskGroups;
        
        class VKE_API CShaderManager
        {
            friend struct ShaderManagerTasks;

            using ShaderTypeArray = glslang::TShader*[ ShaderTypes::_MAX_COUNT ];
            struct SCompilationUnit
            {
                //ShaderTypeArray   apShaders = { nullptr };
                SCompileShaderInfo  aInfos[ ShaderTypes::_MAX_COUNT ];
            };

            //using ShaderMap = vke_hash_map< ShaderHandle, CShader* >;
            public:
                using ShaderVec = Utils::TCDynamicArray < ShaderPtr, 1024 >;

            protected:
            //using ShaderMapArray = ShaderMap[ ShaderTypes::_MAX_COUNT ];
            using ShaderVecArray = ShaderVec[ ShaderTypes::_MAX_COUNT ];
            using ShaderBuffer = Utils::TSFreePool< CShader*, CShader*, 1024 >;
            using ShaderBufferArray = ShaderBuffer[ ShaderTypes::_MAX_COUNT ];
            //using ProgramMap = vke_hash_map< ShaderProgramHandle, CShaderProgram* >;
            using ProgramVec = Utils::TCDynamicArray< CShaderProgram*, 1024 >;
            using ProgramBuffer = Utils::TSFreePool< CShaderProgram*, CShaderProgram*, 1024 >;
            template<class T>
            using TaskPool = Utils::TSFreePool< T, T*, 1024 >;

            using CreateShaderTaskPool = TaskPool< ShaderManagerTasks::SCreateShaderTask >;
            using CreateProgramTaskPool = TaskPool< ShaderManagerTasks::SCreateProgramTask >;

            public:

                using ShaderCreateDescVec = Utils::TCDynamicArray< SShaderDesc >;

            public:

                                    CShaderManager(CDeviceContext* pCtx);
                                    ~CShaderManager();

                Result              Create(const SShaderManagerDesc& Desc);
                Result              Init(const SShaderManagerInitDesc& Desc);
                void                Destroy();

                SHADER_TYPE         FindShaderType(cstr_t pFileName);
                ShaderPtr           CreateShader(const SShaderCreateDesc& Desc);
                Result              CreateShaders(const SShadersCreateDesc& Desc, ShaderVec* pvOut);
                Result              PrepareShader(ShaderPtr* pInOut);
                Result              LoadShader(ShaderPtr* pInOut);
                //void                FreeShader(ShaderPtr* pInOut);
                //void                DestroyShader(ShaderPtr* pInOut);
                //ShaderProgramPtr    CreateProgram(const SShaderProgramDesc& Desc);
                ShaderProgramPtr    CreateProgram(const SShaderProgramCreateDesc& Desc);

                Result          Compile();
                Result          Link();

            protected:

                ShaderPtr           _CreateShaderTask(const SShaderCreateDesc& Desc);
                Result              _PrepareShaderTask(ShaderPtr*);
                Result              _LoadShaderTask(ShaderPtr*);
                ShaderProgramPtr    _CreateProgramTask(const SShaderProgramCreateDesc& Desc);

                template<class T>
                T*                  _GetTask(TaskPool< T >* pPool);

            protected:

                SShaderManagerDesc          m_Desc;
                SShaderManagerInitDesc      m_InitDesc;
                Memory::CFreeListPool       m_aShaderFreeListPools[ ShaderTypes::_MAX_COUNT ];
                Memory::CFreeListPool       m_ShaderProgramFreeListPool;
                CDeviceContext*             m_pCtx;
                CShaderCompiler*            m_pCompiler = nullptr;
                Core::CFileManager*         m_pFileMgr;
                ShaderBufferArray           m_aShaderBuffers;
                //ShaderMapArray              m_amShaderHandles;
                ProgramBuffer               m_ProgramBuffer;
                SShaderTaskGroups*          m_pShaderTaskGroups = nullptr;
                CreateShaderTaskPool        m_CreateShaderTaskPool;
                CreateProgramTaskPool       m_CreateProgramTaskPool;
                Threads::SyncObject         m_aTaskSyncObjects[ ShaderManagerTasks::_MAX_COUNT ];
                Threads::SyncObject         m_aShaderTypeSyncObjects[ ShaderTypes::_MAX_COUNT ];
                Threads::SyncObject         m_ShaderProgramSyncObj;
                SCompilationUnit            m_CurrCompilationUnit;
        };

        template<class T>
        T* CShaderManager::_GetTask(TaskPool< T >* pPool)
        {
            T* pTask = nullptr;
            if( !pPool->vFreeElements.PopBack( ppTaskOut ) )
            {
                T Task;
                uint32_t idx = pPool->vPool.PushBack( Task );
                pTask = &pPool->vPool[ idx ];
            }
            pTask->pMgr = this;
            return pTask;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER