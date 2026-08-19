#pragma once

#include "RenderSystem/Common.h"
#include "Core/Math/Math.h"
#include "Core/Utils/TCBitset.h"
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/Resources/CTexture.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/Resources/CShader.h"

namespace VKE::RenderSystem
{
    struct SynchronizationObjectTypes
    {
        enum TYPE
        {
            CPU_TO_CPU,
            GPU_TO_CPU,
            GPU_TO_GPU,
            _MAX_COUNT
        };
    };

    using SYNC_OBJECT_TYPE = SynchronizationObjectTypes::TYPE;

    struct VKE_API SSynchronizationObject
    {
        union
        {
            RHI::CPUFence GPUToCPU;
            RHI::GPUFence GPUToGPU;
            Threads::SyncObject CPUToCPU;
        };

        SYNC_OBJECT_TYPE type = SynchronizationObjectTypes::_MAX_COUNT;

        SSynchronizationObject()
        {
        }

        SSynchronizationObject( const RHI::CPUFence& Obj ) :
            GPUToCPU{ Obj }, type{ SynchronizationObjectTypes::GPU_TO_CPU }
        {
        }

        SSynchronizationObject( const RHI::GPUFence& Obj ) :
            GPUToGPU{ Obj }, type{ SynchronizationObjectTypes::GPU_TO_GPU }
        {
        }

        SSynchronizationObject( const Threads::SyncObject& Obj ) :
            CPUToCPU{ Obj }, type{ SynchronizationObjectTypes::CPU_TO_CPU }
        {
        }

        SSynchronizationObject& operator=( const SSynchronizationObject& Obj )
        {
            VKE_ASSERT2( ( type == Obj.type ) || ( type == SynchronizationObjectTypes::_MAX_COUNT ),
                         "The rhs.type must be the same as this->type, or this->type must not be set." );
            type = Obj.type;
            switch( Obj.type )
            {
                case SynchronizationObjectTypes::CPU_TO_CPU:
                    CPUToCPU = Obj.CPUToCPU;
                    break;
                case SynchronizationObjectTypes::GPU_TO_CPU:
                    GPUToCPU = Obj.GPUToCPU;
                    break;
                case SynchronizationObjectTypes::GPU_TO_GPU:
                    GPUToGPU = Obj.GPUToGPU;
                    break;
            };
            return *this;
        }
    };

    using SyncObject = SSynchronizationObject;

    struct WaitOnBits
    {
        enum BITS : uint8_t
        {
            NONE              = 0x0,
            GPU_WAITS_FOR_GPU = VKE_BIT( 1 ),
            CPU_WAITS_FOR_GPU = VKE_BIT( 2 ),
            CPU_WAITS_FOR_CPU = VKE_BIT( 3 )
        };
    };

    using WaitOnFlags = Utils::TCBitset< uint8_t >;

    struct WaitForFrames
    {
        enum FRAME : int8_t
        {
            LAST    = -1,
            CURRENT = 0,
            NEXT    = 1,
            _MAX_COUNT
        };
    };

    using WAIT_FOR_FRAME = WaitForFrames::FRAME;

    class VKE_API CFrameGraphNode
    {
        friend class CFrameGraph;
        friend class CFrameGraphExecuteNode;
        friend FrameGraphWorkload;

    protected:
        using NodeQueue                               = vke_queue< CFrameGraphNode* >;
        using NodeArray                               = Utils::TCDynamicArray< CFrameGraphNode*, 1 >;
        using SyncObjArray                            = Utils::TCDynamicArray< SyncObject, 1 >;
        using GPUFenceArray                           = Utils::TCDynamicArray< RHI::GPUFence, 1 >;
        using CPUFencearray                           = Utils::TCDynamicArray< RHI::CPUFence, 1 >;
        using ThreadFenceArray                        = Utils::TCDynamicArray< Platform::ThreadFence, 1 >;
        using TextureArray                            = Utils::TCDynamicArray< TexturePtr, 8 >;
        using index_t                                 = uint8_t;
        static constexpr auto           INVALID_INDEX = UNDEFINED_U8;
        static const FrameGraphWorkload EmptyWorkload;

        struct SIndex
        {
            index_t execute       = INVALID_INDEX;
            index_t commandBuffer = INVALID_INDEX;
            index_t thread        = INVALID_INDEX;
            index_t gpuFence      = INVALID_INDEX;
            index_t cpuFence      = INVALID_INDEX;
            index_t threadFence   = INVALID_INDEX;
        };

        struct SWait
        {
            GPUFenceArray    vGPUFences;
            CPUFencearray    vCPUFences;
            ThreadFenceArray vThreadFences;
        };

    public:
        using TaskFunc = std::function< Threads::TASK_RESULT( const CFrameGraphNode*, uint8_t ) >;

        struct STaskResult
        {
            bool executedOnCPU : 1;
            bool executedOnGPU : 1;

            constexpr STaskResult() : executedOnCPU{ 0 }, executedOnGPU{ 0 }
            {
            }
        };

        struct SWaitInfo
        {
            CFrameGraphNode* pNode;
            WAIT_FOR_FRAME   frame  = WaitForFrames::CURRENT;
            WaitOnFlags      WaitOn = WaitOnBits::NONE;
        };

        using TaskResultArray       = Utils::TCDynamicArray< STaskResult*, 1024 >;
        using CPUFenceTaskResultMap = vke_map< RHI::CPUFence, TaskResultArray >;
        using FormatArray           = Utils::TCDynamicArray< FORMAT, 8 >;

    protected:
        struct STaskData
        {
            STaskResult* pResult = nullptr;
            TaskFunc     Func;
        };

        using WaitArray   = Utils::TCDynamicArray< SWaitInfo, 1 >;
        using TaskQueue   = vke_queue< STaskData >;
        using TaskSyncObj = Threads::SyncObject;

    public:
        virtual ~CFrameGraphNode()
        {
        }

        virtual void Init( const void* )
        {
        }

        CFrameGraph* GetFrameGraph() const
        {
            return m_pFrameGraph;
        }

        CFrameGraphNode* AddSubpass( CFrameGraphNode*, uint32_t index = UINT32_MAX );
        CFrameGraphNode* AddSubpass( cstr_t pName, FrameGraphWorkload&& );
        bool             IsSubpassEnabled( const ResourceName& );

        void SetWorkload( FrameGraphWorkload&& Func )
        {
            m_Workload = std::move( Func );
        }

        CContextBase* GetContext() const
        {
            return m_pContext;
        }

        CommandBufferPtr GetCommandBuffer()
        {
            return m_pCommandBuffer;
        }

        CommandBufferPtr GetCommandBuffer( uint8_t backBufferIndex ) const;

        bool IsEnabled() const
        {
            return m_isEnabled;
        }

        void IsEnabled( bool isEnabled );

        void WaitFor( const SWaitInfo& );

        void AddSynchronization( const SyncObject& Obj )
        {
            m_vSyncObjects.PushBack( Obj );
        }

        //RHI::GPUFence&   GetGPUFence( uint32_t backBufferIndex ) const;
        //RHI::CPUFence&   GetCPUFence( uint32_t backBufferIndex ) const;
        Platform::ThreadFence& GetThreadFence();

        void   SignalThreadFence( uint32_t value );
        void   IncrementThreadFence();
        Result Wait( const RHI::GPUFence& );
        Result Wait( const RHI::CPUFence&, uint64_t timeout );
        Result Wait( const Platform::ThreadFence&, uint32_t value, uint64_t timeout );
        Result WaitForFrame( const Platform::ThreadFence&, WAIT_FOR_FRAME frame, uint64_t timeout );

        Result OnWorkloadBegin( uint8_t );
        Result OnWorkloadEnd( Result );

        void AddTask( TaskFunc&&, STaskResult* );

        CFrameGraphNode* SetNext( CFrameGraphNode* );

        CFrameGraphNode* GetPrev()
        {
            return m_pPrevNode;
        }

        const TexturePtr GetColorRenderTarget( uint32_t index ) const;

        const TexturePtr GetDepthStencilRenderTarget() const
        {
            return m_pDepthStencilRenderTarget;
        }

        const FormatArray& GetColorRenderTargetFormats() const
        {
            return m_vColorRenderTargetFormats;
        }

        FORMAT GetDepthRenderTargetFormat() const;

        bool HasCommandBuffer() const
        {
            return !m_CommandBufferName.IsEmpty();
        }

        bool HasRenderPass() const
        {
            return m_hasRenderPass;
        }

        bool IsSubpass() const
        {
            return m_isSubpass;
        }

        const Rect2DI32& GetRenderArea() const
        {
            return m_RenderArea;
        }

        uint64_t GetFenceValue() const;
        uint64_t InitFenceValue(uint8_t backBufferIndex);

        RHI::RenderPass GetRHIRenderPass() const
        {
            return m_hRHIRenderPass;
        }

        Result Create(const SFrameGraphPassDesc& Desc)
        {
            return _Create( Desc );
        }

    protected:
        Result _Create( const SFrameGraphPassDesc& );
        void   _Destroy();
        Result _Run( CFrameGraphNode* pLastNode );
        Result _WaitForThreads();
        void   _SignalGPUFence();
        void   _CreateBeginRenderPassInfo( const SFrameGraphNodeDesc& );

        void _BeginRenderPass();
        void _EndRenderPass();

        struct SExecuteTaskDesc
        {
            /// <summary>
            /// Number of tasks to execute per one node iteration
            /// </summary>
            uint32_t executeTaskCount = 1;
            /// <summary>
            /// back buffer index
            /// </summary>
            uint8_t backBufferIndex;
            /// <summary>
            /// Always remove task from the queue
            /// </summary>
            bool forceRemove : 1;
        };

        void _ExecuteTasks( const SExecuteTaskDesc& );

    protected:
        ShortName m_Name;
        ShortName m_ThreadName;
        ShortName m_CommandBufferName;
        ShortName m_ExecuteName;

        CONTEXT_TYPE            m_ctxType;
        FrameGraphWorkload      m_Workload;
        CFrameGraph*            m_pFrameGraph = nullptr;
        NodeQueue               m_qSubpasses;
        CFrameGraphNode*        m_pParent      = nullptr;
        CFrameGraphNode*        m_pNextNode    = nullptr;
        CFrameGraphNode*        m_pPrevNode    = nullptr;
        CFrameGraphNode*        m_pSubpassNode = nullptr;
        CFrameGraphExecuteNode* m_pExecuteNode = nullptr;
        // NodeArray m_vpSubpassNodes;
        WaitArray               m_vWaitForNodes;
        SyncObjArray            m_vSyncObjects;
        RHI::FenceValue   m_fenceValue = 0;
        CContextBase*           m_pContext = nullptr;
        CommandBufferRefPtr     m_pCommandBuffer;
        SIndex                  m_Index;
        Platform::ThreadFence   m_hFence;
        std::condition_variable m_CondVar;
        std::mutex              m_CondVarMtx;
        TaskQueue               m_qTasks;
        TaskSyncObj             m_TaskSyncObj;
        CPUFenceTaskResultMap   m_mTaskResults;
        SBeginRenderPassInfo2   m_BeginRenderPassInfo;
        TextureArray            m_vpColorRenderTargets;
        TexturePtr              m_pDepthStencilRenderTarget;
        FormatArray             m_vColorRenderTargetFormats;
        RHI::RenderPass   m_hRHIRenderPass = RHI::Null;
        Rect2DI32               m_RenderArea;
        FrameGraphNodeFlagBits  m_Flags;
        /// <summary>
        /// if true, this node will execute command buffers.
        /// Usually that means that this node is the last one using particular ExecuteBatch
        /// This member is set in CFrameGraph::Build
        /// </summary>
        bool m_doExecute     = false;
        bool m_isEnabled     = true;
        bool m_finished      = false;
        bool m_isAsync       = false;
        bool m_isSubpass     = false;
        bool m_hasRenderPass = false;
    };

    class VKE_API CFrameGraphMultiWorkloadNode final : CFrameGraphNode
    {
        using WorkloadQueue = std::deque< FrameGraphWorkload >;

        struct WorkloadQueueTypes
        {
            enum TYPE
            {
                /// <summary>
                /// All workloads in this queue will be executed in every frame
                /// </summary>
                EXECUTE_ALL_PER_FRAME,
                /// <summary>
                /// Only one workload will be executed per frame
                /// </summary>
                EXECUTE_ONE_PER_FRAME,
                /// <summary>
                /// All workloads will be executed in a single frame then the queue is cleared.
                /// </summary>
                EXECUTE_POP_ALL_PER_FRAME,
                /// <summary>
                /// Only one workload is executed per frame, then it is front-popped.
                /// </summary>
                EXECUTE_POP_ONE_PER_FRAME,
                _MAX_COUNT
            };
        };

    protected:
        WorkloadQueue m_aqWorkloads[ WorkloadQueueTypes::_MAX_COUNT ];
    };

    class VKE_API CFrameGraphExecuteNode final : public CFrameGraphNode
    {
        friend class CFrameGraph;
        friend class CFrameGraphNode;
        friend FrameGraphWorkload;

        struct SExecuteData
        {
            Utils::TCDynamicArray< RHI::CommandBuffer > vpCommandBuffers;
            SSubmitInfo                               SubmitInfo;
        };

        // Build time
    public:
        /// <summary>
        /// Build time method.
        /// </summary>
        /// <param name=""></param>
        /// <returns></returns>
        CFrameGraphExecuteNode* AddToExecute( CFrameGraphNode* );
        CFrameGraphExecuteNode* AddToExecute( CFrameGraphExecuteNode* );

        // Runtime
    public:
    protected:
        /// <summary>
        /// Runtime method.
        /// </summary>
        SExecuteData* _BuildDataToExecute( uint8_t backBufferIndex );

    protected:
        NodeArray                    m_vpNodesToExecute;
        SExecuteData                 m_aExecutes[ 4 ];
        EXECUTE_COMMAND_BUFFER_FLAGS m_executeFlags = 0;
    };

    
} // namespace VKE::RenderSystem