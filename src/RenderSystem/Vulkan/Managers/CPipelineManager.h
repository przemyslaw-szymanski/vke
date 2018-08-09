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

                // State
                void        SetShader( ShaderPtr pShader );

            protected:

                hash_t      _CalcHash(const SPipelineDesc&);
                hash_t      _CalcHash(const SPipelineLayoutDesc&);
                Result      _CreatePipelineTask(const SPipelineDesc&, PipelinePtr*);
                Result      _CreatePipeline(const SPipelineDesc& Desc, CPipeline::SVkCreateDesc* pOut, VkPipeline* pVkOut);
                PipelinePtr _CreateCurrPipeline();

            protected:

                CDeviceContext*             m_pCtx;
                PipelineBuffer              m_Buffer;
                PipelineLayoutBuffer        m_LayoutBuffer;
                PipelineMemoryPool          m_PipelineMemMgr;
                PipelineLayoutMemoryPool    m_PipelineLayoutMemMgr;
                CreatePipelineTaskPool      m_CreatePipelineTaskPool;

                Threads::SyncObject         m_CreatePipelineSyncObj;

                SPipelineCreateDesc         m_CurrPipelineDesc;
                PipelinePtr                 m_pCurrPipeline;
                bool                        m_CurrPipelineDirty = false;
        };
    } // RenderSystem
} // VKE