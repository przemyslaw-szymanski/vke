#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "Core/Managers/CResourceManager.h"
#include "Core/Memory/TCFreeListManager.h"
#include "RenderSystem/Vulkan/CShaderCompiler.h"
#include "Core/Threads/ITask.h"
#include "Core/Threads/CTaskGroup.h"
#include "Core/Resources/CResource.h"
#include "RenderSystem/Resources/CShader.h"

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

        /*SShaderCreateDesc::SShaderCreateDesc(SShaderCreateDesc&& Other)
        {
            this->operator=( std::move( Other ) );
        }

        SShaderCreateDesc& SShaderCreateDesc::operator=(SShaderCreateDesc&& Other)
        {
            if( this != &Other )
            {
                Create = std::move( Other.Create );
                Shader = std::move( Other.Shader );
            }
            return *this;
        }*/

        struct SShaderProgramCreateDesc
        {
            SCreateResourceDesc Create;
            SShaderProgramDesc  Program;
        };

        struct SShadersCreateDesc
        {
            using CreateDescVec = Utils::TCDynamicArray< SCreateShaderDesc >;
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
                SCreateShaderDesc   Desc;
                ShaderPtr           pShader;

                SCreateShaderTask() {}
                SCreateShaderTask(SCreateShaderTask&&) = default;
                ~SCreateShaderTask() { Clear(); }

                SCreateShaderTask& operator=(SCreateShaderTask&&) = default;
                SCreateShaderTask& operator=(const SCreateShaderTask& Other)
                {
                    pMgr = Other.pMgr;
                    Desc = Other.Desc;
                    pShader = Other.pShader;
                    return *this;
                }

                TaskState _OnStart(uint32_t tid) override;
                void _OnGet(void**) override;

                void Clear()
                {
                    
                }
            };

            struct SCreateShadersTask : public Threads::ITask
            {
                friend class CShaderManager;
                CShaderManager*     pMgr = nullptr;
                SShadersCreateDesc  Desc;

                ~SCreateShadersTask() { Clear(); }

                TaskState   _OnStart(uint32_t tid) override;
                void _OnGet(void**) override;

                void Clear()
                {
                    Desc.vCreateDescs.ClearFull();
                }
            };

            struct SCreateProgramTask : public Threads::ITask
            {
                friend class CShaderManager;
                CShaderManager*             pMgr = nullptr;
                SShaderProgramCreateDesc    Desc;
                ShaderProgramPtr            pProgram;

                ~SCreateProgramTask() { Clear(); }

                TaskState _OnStart(uint32_t tid) override;
                void _OnGet(void**) override;

                void Clear()
                {
                    for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                    {
                        Desc.Program.apShaders[ i ] = nullptr;
                    }
                    pProgram = nullptr;
                }
            };

        }; // ShaderManagerTasks

        struct SShaderTaskGroups;
        
        class VKE_API CShaderManager
        {
            friend struct ShaderManagerTasks;
            friend class CShader;
            friend class CShaderProgram;

            friend void* VkAllocateCallback( void*, size_t, size_t, VkSystemAllocationScope );

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
                //using ShaderBuffer = Utils::TSFreePool< CShader*, CShader*, 1024 >;
                //using ShaderBuffer = Core::TSResourceBuffer< CShader*, CShader*, 1024 >;
                using ShaderBuffer = Core::TSVectorResourceBuffer< CShader*, CShader* >;
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
                ShaderRefPtr        CreateShader(SCreateShaderDesc&& Desc);
                ShaderRefPtr        CreateShader(const SCreateShaderDesc& Desc);
                Result              CreateShaders(const SShadersCreateDesc& Desc, ShaderVec* pvOut);
                Result              PrepareShader(ShaderPtr* ppInOut);
                Result              LoadShader(ShaderPtr* ppInOut);
                //void                DestroyShader(ShaderPtr* pInOut);
                //ShaderProgramPtr    CreateProgram(const SShaderProgramDesc& Desc);
                ShaderProgramPtr    CreateProgram(const SShaderProgramCreateDesc& Desc);
                ShaderRefPtr        GetShader( ShaderHandle hShader );
                ShaderRefPtr        GetShader( const hash_t& hash, SHADER_TYPE type );

                void                FreeUnusedResources();

                ShaderPtr           GetDefaultShader( SHADER_TYPE type );

                Result          Compile();
                Result          Link();

            protected:

                ShaderRefPtr        _CreateShaderTask(const SCreateShaderDesc& Desc);
                Result              _PrepareShaderTask(CShader**);
                Result              _LoadShaderTask(CShader**);
                ShaderProgramPtr    _CreateProgramTask(const SShaderProgramCreateDesc& Desc);
                //Result              _LoadProgramTask(CShaderProgram** ppInOut);
                //Result              _PrepareProgramTask(CShaderProgram** ppInOut);
                Result              _CreateShaderModule(const uint32_t* pBinary, size_t size, CShader** ppInOut);
                //void                _FreeProgram(CShaderProgram* pProgram);
                void                _FreeShader(CShader* pShader);
                //Result              _PreprocessIncludes(CShader** ppShader);

                void*               _AllocateMemory(size_t size, size_t alignment);
                void                _FreeMemory(void* pMemory, size_t size, size_t alignment);
                Result              _CreateDefaultShaders();

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
                ShaderPtr                   m_apDefaultShaders[ ShaderTypes::_MAX_COUNT ];
        };

        template<class T>
        T* CShaderManager::_GetTask(TaskPool< T >* pPool)
        {
            T* pTask = nullptr;
            if( !pPool->vFreeElements.PopBack( &pTask ) )
            {
                T Task;
                uint32_t idx = pPool->vPool.PushBack( std::move( Task ) );
                pTask = &pPool->vPool[ idx ];
            }
            pTask->pMgr = this;
            return pTask;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER