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
                SPipelineLayoutDesc LayoutDesc;
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

                using PipelineBuffer = Core::TSUniqueResourceBuffer< PipelinePtr, handle_t, 2048 >;
                using PipelineLayoutBuffer = Core::TSUniqueResourceBuffer< PipelineLayoutRefPtr, handle_t, 1024 >;
                using PipelineMemoryPool = Memory::CFreeListPool;
                using PipelineLayoutMemoryPool = Memory::CFreeListPool;

                using CreatePipelineTaskPoolHelper = TaskPoolHelper< PipelineManagerTasks::SCreatePipelineTask, 1024 >;
                using CreatePipelineTaskPool = CreatePipelineTaskPoolHelper::Pool;

                using DefaultDDIPipelineMap = vke_hash_map< handle_t, DDIPipeline >;

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
                PipelineLayoutRefPtr    GetLayout( PipelineLayoutHandle hLayout );

                PipelineLayoutPtr       GetDefaultLayout() const { return m_pDefaultLayout; }

            protected:

                hash_t      _CalcHash(const SPipelineDesc&);
                hash_t      _CalcHash(const SPipelineLayoutDesc&);
                Result      _CreatePipelineTask(const SPipelineDesc&, PipelinePtr*);
                Result      _CreatePipelineTask( PipelinePtr* );

                //PipelinePtr _CreateCurrPipeline(bool createAsync);

                void        _DestroyPipeline( CPipeline** ppPipeline );
                void        _DestroyLayout( CPipelineLayout** ppLayout );

                DDIPipeline _GetDefaultPipeline( const SPipelineDesc& );

            protected:

                CDeviceContext*             m_pCtx;
                PipelineBuffer              m_Buffer;
                PipelineLayoutBuffer        m_LayoutBuffer;
                PipelineMemoryPool          m_PipelineMemMgr;
                PipelineLayoutMemoryPool    m_PipelineLayoutMemMgr;
                CreatePipelineTaskPool      m_CreatePipelineTaskPool;
                DefaultDDIPipelineMap       m_mDefaultDDIPipelines;

                Threads::SyncObject         m_CreatePipelineSyncObj;
                PipelineLayoutPtr           m_pDefaultLayout;
                hash_t                      m_currPipelineHash = 0;
                CPipeline*                  m_pCurrPipeline = nullptr;
        };

        struct DepthStencilStates
        {
            enum STATE : uint16_t
            {
                ENABLE_DEPTH_TEST       = VKE_BIT(0),
                DISABLE_DEPTH_TEST      = VKE_BIT(1),
                ENABLE_STENCIL_TEST     = VKE_BIT(2),
                DISABLE_STENCIL_TEST    = VKE_BIT(3),
                ENABLE_DEPTH_WRITE      = VKE_BIT(4),
                DISABLE_DEPTH_WRITE     = VKE_BIT(5),
                ENABLE_STENCIL_WRITE    = VKE_BIT(6),
                DISABLE_STENCIL_WRITE   = VKE_BIT(7),
                _MAX_COUNT = 8
            };
        };
        using DEPTH_STENCIL_STATE = DepthStencilStates::STATE;

        class VKE_API CPipelineBuilder
        {
            using DescSetArray = Utils::TCDynamicArray< DescriptorSetHandle >;
            using ShaderArray = Utils::TCDynamicArray< ShaderHandle >;

            public:

                const SPipelineDesc&    GetCurrent() const { return m_Desc; }

                void    SetParent( PipelinePtr );

                void    Bind( const RenderPassHandle& );
                void    Bind( RenderPassPtr );
                void    Bind( const DDIRenderPass& );
                void    Bind( const CSwapChain* );

                void    Bind( const DescriptorSetHandle&, const uint32_t offset );

                void    SetState( const ShaderHandle& );
                void    SetState( const PipelineLayoutHandle& );

                void    SetState( const SPipelineDesc::SDepthStencil& );
                void    SetState( const SPipelineDesc::SBlending& );
                void    SetState( const SPipelineDesc::SInputLayout& );
                void    SetState( const SPipelineDesc::SMultisampling& );
                void    SetState( const SPipelineDesc::SRasterization& );
                void    SetState( const SPipelineDesc::SShaders& );
                void    SetState( const SPipelineDesc::STesselation& );
                void    SetState( const SPipelineDesc::SViewport& );
                
                void    SetState( DEPTH_STENCIL_STATE );

                void    SetBlendingEnable( bool enable );

                void    SetVertexAttribute( const uint16_t& index, const SPipelineDesc::SInputLayout::SVertexAttribute& );

                void    SetCullMode( const CULL_MODE& );
                void    SetFrontFace( const FRONT_FACE& );

                PipelinePtr Build( CDeviceContext* );

            protected:

                SPipelineDesc   m_Desc;
                PipelinePtr     m_pParent;
                DescSetArray    m_vDescSets;
                ShaderArray     m_vShaders;
        };
    } // RenderSystem
} // VKE