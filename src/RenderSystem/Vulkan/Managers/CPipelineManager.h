#pragma once

#include "RenderSystem/Common.h"
#include "RenderSystem/CPipeline.h"
#include "Core/Memory/TCFreeListManager.h"
#include "Core/Managers/CResourceManager.h"
#include "Core/Threads/ITask.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct PipelineManagerTasks
        {
            enum
            {
                CREATE_PIPELINE,
                CREATE_PIPELINES,
                _MAX_COUNT
            };

            struct SCreatePipelineTask : public Threads::ITask
            {
                friend class CPipelineManager;
                friend class CPipeline;
                CPipelineManager*   pMgr = nullptr;
                SPipelineCreateDesc Desc;
                PipelinePtr         pPipeline;

                TaskState _OnStart(uint32_t tid) override;

                void _OnGet(void**) override;
            };
        };

        class CPipelineManager
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CPipeline;

            protected:

                using PipelineBuffer = Core::TSResourceBuffer< CPipeline*, CPipeline*, 2048 >;
                using PipelineLayoutBuffer = Core::TSResourceBuffer< CPipelineLayout*, CPipelineLayout*, 1024 >;
                using PipelineMemoryPool = Memory::CFreeListPool;
                using PipelineLayoutMemoryPool = Memory::CFreeListPool;

                using CreatePipelineTaskPoolHelper = TaskPoolHelper< PipelineManagerTasks::SCreatePipelineTask, 1024 >;
                using CreatePipelineTaskPool = CreatePipelineTaskPoolHelper::Pool;

            public:

                CPipelineManager(CDeviceContext* pCtx);
                ~CPipelineManager();

                Result Create(const SPipelineManagerDesc&);
                void Destroy();

                PipelineRefPtr CreatePipeline(const SPipelineCreateDesc&);
                PipelineLayoutRefPtr CreateLayout(const SPipelineLayoutDesc& Desc);

                PipelineRefPtr          GetPipeline( PipelineHandle hPipeline );
                PipelineLayoutRefPtr    GetPipelineLayout( PipelineLayoutHandle hLayout );

            protected:

                hash_t      _CalcHash(const SPipelineDesc&);
                hash_t      _CalcHash(const SPipelineLayoutDesc&);
                Result      _CreatePipelineTask(const SPipelineDesc&, PipelinePtr*);
                DDIPipeline _CreatePipeline(const SPipelineDesc& Desc);
                //PipelinePtr _CreateCurrPipeline(bool createAsync);

            protected:

                CDeviceContext*             m_pCtx;
                PipelineBuffer              m_Buffer;
                PipelineLayoutBuffer        m_LayoutBuffer;
                PipelineMemoryPool          m_PipelineMemMgr;
                PipelineLayoutMemoryPool    m_PipelineLayoutMemMgr;
                CreatePipelineTaskPool      m_CreatePipelineTaskPool;

                Threads::SyncObject         m_CreatePipelineSyncObj;

                CPipeline*                  m_pCurrPipeline = nullptr;
        };
    } // RenderSystem
} // VKE