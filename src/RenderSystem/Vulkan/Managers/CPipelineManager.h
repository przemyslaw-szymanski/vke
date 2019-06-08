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

                using PipelineBuffer = Core::TSUniqueResourceBuffer< PipelineRefPtr, handle_t, 2048 >;
                using PipelineLayoutBuffer = Core::TSUniqueResourceBuffer< PipelineLayoutRefPtr, handle_t, 1024 >;
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

                void    DestroyPipeline( PipelinePtr* pInOut );
                void    DestroyLayout( PipelineLayoutPtr* pInOut );

                PipelineRefPtr          GetPipeline( PipelineHandle hPipeline );
                PipelineLayoutRefPtr    GetPipelineLayout( PipelineLayoutHandle hLayout );

                PipelineLayoutPtr       GetDefaultLayout() const { return m_pDefaultLayout; }

            protected:

                hash_t      _CalcHash(const SPipelineDesc&);
                hash_t      _CalcHash(const SPipelineLayoutDesc&);
                Result      _CreatePipelineTask(const SPipelineDesc&, PipelinePtr*);
                DDIPipeline _CreatePipeline(const SPipelineDesc& Desc);
                //PipelinePtr _CreateCurrPipeline(bool createAsync);

                void        _DestroyPipeline( CPipeline** ppPipeline );
                void        _DestroyLayout( CPipelineLayout** ppLayout );

            protected:

                CDeviceContext*             m_pCtx;
                PipelineBuffer              m_Buffer;
                PipelineLayoutBuffer        m_LayoutBuffer;
                PipelineMemoryPool          m_PipelineMemMgr;
                PipelineLayoutMemoryPool    m_PipelineLayoutMemMgr;
                CreatePipelineTaskPool      m_CreatePipelineTaskPool;

                Threads::SyncObject         m_CreatePipelineSyncObj;
                PipelineLayoutPtr           m_pDefaultLayout;
                hash_t                      m_currPipelineHash = 0;
                CPipeline*                  m_pCurrPipeline = nullptr;
        };
    } // RenderSystem
} // VKE