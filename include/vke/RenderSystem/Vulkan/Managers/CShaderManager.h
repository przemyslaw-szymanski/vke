#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "Core/Memory/TCFreeListManager.h"
#include "RenderSystem/Vulkan/CShaderCompiler.h"
#include "Core/Threads/ITask.h"

namespace glslang
{
    class TShader;
} // glslang

namespace VKE
{
    namespace RenderSystem
    {
        struct SShaderManagerDesc
        {
            uint32_t    aMaxShaderCounts[ ShaderTypes::_MAX_COUNT ] = { 0 };
            uint32_t    maxShaderProgramCount = 0;
        };

        class CShader;
        class CShaderProgram;
        class CShaderCompiler;
        class CShaderManager;

        struct ShaderManagerTasks
        {
            struct SCreateShaderTask : public Threads::ITask
            {
                friend class CShaderManager;
                CShaderManager* pMgr = nullptr;
                CShader* pShader = nullptr;

                TaskState _OnStart(uint32_t tid) override;
            };
        }; // ShaderManagerTasks

        
        class CShaderManager
        {
            friend struct ShaderManagerTasks;

            using ShaderTypeArray = glslang::TShader*[ ShaderTypes::_MAX_COUNT ];
            struct SCompilationUnit
            {
                //ShaderTypeArray   apShaders = { nullptr };
                SCompileShaderInfo  aInfos[ ShaderTypes::_MAX_COUNT ];
            };

            using ShaderMap = vke_hash_map< ShaderHandle, CShader* >;
            using ShaderVec = Utils::TCDynamicArray < CShader*, 1024 >;
            using ShaderMapArray = ShaderMap[ ShaderTypes::_MAX_COUNT ];
            using ShaderVecArray = ShaderVec[ ShaderTypes::_MAX_COUNT ];
            using ShaderBuffer = Utils::TSFreePool< CShader*, CShader*, 1024 >;
            using ShaderBufferArray = ShaderBuffer[ ShaderTypes::_MAX_COUNT ];
            using ProgramMap = vke_hash_map< ShaderProgramHandle, CShaderProgram* >;
            using ProgramVec = Utils::TCDynamicArray< CShaderProgram*, 1024 >;
            using ProgramBuffer = Utils::TSFreePool< CShaderProgram*, CShaderProgram*, 1024 >;
            template<class T>
            using TaskPool = Utils::TSFreePool< T, T*, 1024 >;

            using CreateShaderTaskPool = TaskPool< ShaderManagerTasks::SCreateShaderTask >;

            public:

                                    CShaderManager(CDeviceContext* pCtx);
                                    ~CShaderManager();

                Result              Create(const SShaderManagerDesc& Desc);
                void                Destroy();

                ShaderPtr           CreateShader(const SResourceCreateDesc& Desc);
                Result              PrepareShader(ShaderPtr* pInOut);
                Result              LoadShader(ShaderPtr* pInOut);
                void                FreeShader(ShaderPtr* pInOut);
                void                DestroyShader(ShaderPtr* pInOut);
                ShaderProgramPtr    CreateProgram(const SShaderProgramDesc& Desc);

                Result          Compile();
                Result          Link();

            protected:

                ShaderPtr           _CreateShaderTask(const SResourceCreateDesc& Desc);
                Result              _PrepareShaderTask(ShaderPtr*);
                Result              _LoadShaderTask(ShaderPtr*);

                template<class T>
                void                _GetTask(TaskPool< T >* pPool, T** ppTaskOut);

            protected:

                SShaderManagerDesc          m_Desc;
                Memory::CFreeListPool       m_aShaderFreeListPools[ ShaderTypes::_MAX_COUNT ];
                Memory::CFreeListPool       m_ShaderProgramFreeListPool;
                CDeviceContext*             m_pCtx;
                CShaderCompiler*            m_pCompiler = nullptr;
                ShaderBufferArray           m_ShaderBuffers;
                ShaderMapArray              m_amShaderHandles;
                ProgramBuffer               m_ProgramBuffer;
                CreateShaderTaskPool        m_CreateShaderTaskPool;
                SCompilationUnit            m_CurrCompilationUnit;
        };

        template<class T>
        void CShaderManager::_GetTask< T >(TaskPool< T >* pPool, T** ppTaskOut)
        {
            if( !pPool->vFreeElements.PopBack( *ppTaskOut ) )
            {
                T Task;
                uint32_t idx = pPool->vPool.PushBack( &Task );
                *ppTaskOut = &pPool->vPool[ idx ];
            }
            ( *ppTaskOut )->pMgr = this;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER