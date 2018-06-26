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
            friend class CPipeline;

            protected:

                using PipelineBuffer = Core::TSResourceBuffer< CPipeline*, CPipeline*, 2048 >;
                using CreatePipelineTaskPoolHelper = TaskPoolHelper< PipelineManagerTasks::SCreatePipelineTask, 1024 >;
                using CreatePipelineTaskPool = CreatePipelineTaskPoolHelper::Pool;

            public:

                CPipelineManager(CDeviceContext* pCtx);
                ~CPipelineManager();

                Result Create(const SPipelineManagerDesc&);
                void Destroy();

                PipelinePtr CreatePipeline(const SPipelineCreateDesc&);

            protected:

                hash_t      _CalcHash(const SPipelineDesc&);
                Result      _CreatePiipelineTask(const SPipelineDesc&, PipelinePtr*);
                Result      _CreatePipeline(const SPipelineDesc& Desc, CPipeline::SVkCreateDesc* pOut, VkPipeline* pVkOut);

            protected:

                CDeviceContext*         m_pCtx;
                PipelineBuffer          m_Buffer;
                Memory::CFreeListPool   m_PipelineFreeListPool;
                CreatePipelineTaskPool  m_CreatePipelineTaskPool;
                Threads::SyncObject     m_CreatePipelineSyncObj;
        };
    } // RenderSystem
} // VKE