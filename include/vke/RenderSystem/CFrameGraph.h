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
            NativeAPI::CPUFence GPUToCPU;
            NativeAPI::GPUFence GPUToGPU;
            Threads::SyncObject CPUToCPU;
        };
        SYNC_OBJECT_TYPE type = SynchronizationObjectTypes::_MAX_COUNT;

        SSynchronizationObject() {}
        SSynchronizationObject(const NativeAPI::CPUFence& Obj) : GPUToCPU{Obj}, type{SynchronizationObjectTypes::GPU_TO_CPU} {}
        SSynchronizationObject(const NativeAPI::GPUFence& Obj) : GPUToGPU{Obj}, type{SynchronizationObjectTypes::GPU_TO_GPU} {}
        SSynchronizationObject(const Threads::SyncObject& Obj) : CPUToCPU{Obj}, type{SynchronizationObjectTypes::CPU_TO_CPU} {}
        SSynchronizationObject& operator=( const SSynchronizationObject& Obj )
        {
            VKE_ASSERT2( ( type == Obj.type ) || (type == SynchronizationObjectTypes::_MAX_COUNT),
                "The rhs.type must be the same as this->type, or this->type must not be set." );
            type = Obj.type;
            switch( Obj.type )
            {
                case SynchronizationObjectTypes::CPU_TO_CPU: CPUToCPU = Obj.CPUToCPU; break;
                case SynchronizationObjectTypes::GPU_TO_CPU: GPUToCPU = Obj.GPUToCPU; break;
                case SynchronizationObjectTypes::GPU_TO_GPU: GPUToGPU = Obj.GPUToGPU; break;
            };
            return *this;
        }
    };
    using SyncObject = SSynchronizationObject;

    struct WaitOnBits
    {
        enum BITS : uint8_t
        {
            NONE = 0x0,
            GPU = VKE_BIT( 1 ),
            CPU = VKE_BIT( 2 ),
            THREAD = VKE_BIT( 3 )
        };
    };
    using WaitOnFlags = Utils::TCBitset<uint8_t>;

    struct WaitForFrames
    {
        enum FRAME : int8_t
        {
            LAST = -1,
            CURRENT = 0,
            NEXT = 1,
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
        using NodeQueue = vke_queue<CFrameGraphNode*>;
        using NodeArray = Utils::TCDynamicArray<CFrameGraphNode*, 1>;
        using SyncObjArray = Utils::TCDynamicArray<SyncObject, 1>;
        using GPUFenceArray = Utils::TCDynamicArray<NativeAPI::GPUFence,1>;
        using CPUFencearray = Utils::TCDynamicArray<NativeAPI::CPUFence, 1>;
        using ThreadFenceArray = Utils::TCDynamicArray<Platform::ThreadFence,1>;
        using TextureArray = Utils::TCDynamicArray< TexturePtr, 8 >;
        using index_t = uint8_t;
        static constexpr auto INVALID_INDEX = UNDEFINED_U8;
        static const FrameGraphWorkload EmptyWorkload;

        struct SIndex
        {
            index_t execute = INVALID_INDEX;
            index_t commandBuffer = INVALID_INDEX;
            index_t thread = INVALID_INDEX;
            index_t gpuFence = INVALID_INDEX;
            index_t cpuFence = INVALID_INDEX;
            index_t threadFence = INVALID_INDEX;
        };

        struct SWait
        {
            GPUFenceArray vGPUFences;
            CPUFencearray vCPUFences;
            ThreadFenceArray vThreadFences;
        };

    public:
      using TaskFunc = std::function<bool( const CFrameGraphNode*, uint8_t )>;
      struct STaskResult
      {
          bool executedOnCPU : 1;
          bool executedOnGPU : 1;
          
          constexpr STaskResult() :
              executedOnCPU{0},
              executedOnGPU{0}
          {}
      };
      struct SWaitInfo
      {
          CFrameGraphNode* pNode;
          WAIT_FOR_FRAME frame = WaitForFrames::CURRENT;
          WaitOnFlags WaitOn = WaitOnBits::NONE;
      };

      using TaskResultArray = Utils::TCDynamicArray<STaskResult*, 1024>;
      using CPUFenceTaskResultMap = vke_map< NativeAPI::CPUFence, TaskResultArray >;
      using FormatArray = Utils::TCDynamicArray<FORMAT, 8>;

    protected:
        struct STaskData
        {
            STaskResult* pResult = nullptr;
            TaskFunc Func;
        };
      using WaitArray = Utils::TCDynamicArray<SWaitInfo, 1>;
      using TaskQueue = vke_queue<STaskData>;
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
        bool IsSubpassEnabled( const ResourceName& );
        void SetWorkload( FrameGraphWorkload&& Func )
        {
            m_Workload = std::move(Func);
        }

        CContextBase* GetContext() const { return m_pContext; }
        
        CommandBufferPtr GetCommandBuffer() { return m_pCommandBuffer; }
        CommandBufferPtr GetCommandBuffer( uint8_t backBufferIndex );

        bool IsEnabled() const { return m_isEnabled; }
        void IsEnabled( bool isEnabled );

        void WaitFor( const SWaitInfo& );

        void AddSynchronization( const SyncObject& Obj ) { m_vSyncObjects.PushBack( Obj ); }

        NativeAPI::GPUFence& GetGPUFence( uint32_t backBufferIndex ) const;
        NativeAPI::CPUFence& GetCPUFence( uint32_t backBufferIndex ) const;
        Platform::ThreadFence& GetThreadFence();

        void SignalThreadFence( uint32_t value );
        void IncrementThreadFence();
        Result Wait( const NativeAPI::GPUFence& );
        Result Wait( const NativeAPI::CPUFence&, uint64_t timeout );
        Result Wait( const Platform::ThreadFence&, uint32_t value, uint64_t timeout );
        Result WaitForFrame( const Platform::ThreadFence&, WAIT_FOR_FRAME frame, uint64_t timeout );
        

        Result OnWorkloadBegin(uint8_t);
        Result OnWorkloadEnd(Result);

        void AddTask( TaskFunc&&, STaskResult* );

        CFrameGraphNode* SetNext( CFrameGraphNode* );
        CFrameGraphNode* GetPrev() { return m_pPrevNode; }

        const TexturePtr GetColorRenderTarget( uint32_t index ) const;
        const TexturePtr GetDepthStencilRenderTarget() const { return m_pDepthStencilRenderTarget; }

        const FormatArray& GetColorRenderTargetFormats() const { return m_vColorRenderTargetFormats; }
        FORMAT GetDepthRenderTargetFormat() const;

        bool HasCommandBuffer() const
        {
            return !m_CommandBufferName.IsEmpty();
        }

        bool HasRenderPass() const { return m_hasRenderPass; }

        bool IsSubpass() const { return m_isSubpass; }

     protected:
        Result _Create( const SFrameGraphPassDesc& );
        void _Destroy();
        Result _Run(CFrameGraphNode* pLastNode);
        Result _WaitForThreads();
        void _SignalGPUFence();
        void _CreateBeginRenderPassInfo( const SFrameGraphNodeDesc& );

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

        CONTEXT_TYPE m_ctxType;
        FrameGraphWorkload m_Workload;
        CFrameGraph* m_pFrameGraph = nullptr;
        NodeQueue m_qSubpasses;
        CFrameGraphNode* m_pParent = nullptr;
        CFrameGraphNode* m_pNextNode = nullptr;
        CFrameGraphNode* m_pPrevNode = nullptr;
        CFrameGraphNode* m_pSubpassNode = nullptr;
        CFrameGraphExecuteNode* m_pExecuteNode = nullptr;
        //NodeArray m_vpSubpassNodes;
        WaitArray m_vWaitForNodes;
        SyncObjArray m_vSyncObjects;
        CContextBase* m_pContext = nullptr;
        CommandBufferRefPtr m_pCommandBuffer;
        SIndex m_Index;
        Platform::ThreadFence m_hFence;
        std::condition_variable m_CondVar;
        std::mutex m_CondVarMtx;
        TaskQueue m_qTasks;
        TaskSyncObj m_TaskSyncObj;
        CPUFenceTaskResultMap m_mTaskResults;
        SBeginRenderPassInfo2 m_BeginRenderPassInfo;
        TextureArray m_vpColorRenderTargets;
        TexturePtr m_pDepthStencilRenderTarget;
        FormatArray m_vColorRenderTargetFormats;
        /// <summary>
        /// if true, this node will execute command buffers.
        /// Usually that means that this node is the last one using particular ExecuteBatch
        /// This member is set in CFrameGraph::Build
        /// </summary>
        bool m_doExecute = false;
        bool m_isEnabled = true;
        bool m_finished = false;
        bool m_isAsync = false;
        bool m_isSubpass = false;
        bool m_hasRenderPass = false;

        ShortName m_Name;
        ShortName m_ThreadName;
        ShortName m_CommandBufferName;
        ShortName m_ExecuteName;
    };

    class VKE_API CFrameGraphMultiWorkloadNode final : CFrameGraphNode
    {
        using WorkloadQueue = std::deque<FrameGraphWorkload>;
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
          Result _BuildDataToExecute(uint8_t backBufferIndex);

      protected:
        NodeArray m_vpNodesToExecute;
        EXECUTE_COMMAND_BUFFER_FLAGS m_executeFlags = 0;
    };

    class VKE_API CResourceLoaddManager
    {
        friend class CFrameGraph;

        using TextureArray = Utils::TCDynamicArray<TextureRefPtr>;
        using BufferArray = Utils::TCDynamicArray<BufferRefPtr>;
        using ShaderArray = Utils::TCDynamicArray<ShaderRefPtr>;

        struct SFrameBudget
        {
            struct
            {
                /// <summary>
                /// Max number of texture loads per frame back buffer
                /// </summary>
                uint16_t textureLoads = Config::RenderSystem::FrameBudget::MAX_TEXTURE_LOAD_COUNT;
                /// <summary>
                /// Max number of buffer loads per frame back buffer
                /// </summary>
                uint16_t bufferLoads = Config::RenderSystem::FrameBudget::MAX_BUFFER_LOAD_COUNT;
                /// <summary>
                /// Max number of shader compilations per frame back buffer
                /// </summary>
                uint16_t shaderCompilations = Config::RenderSystem::FrameBudget::MAX_SHADER_COMPILATION_COUNT;
                /// <summary>
                /// Max number of pipeline creations per frame back buffer
                /// </summary>
                uint16_t pipelineCompilations = Config::RenderSystem::FrameBudget::MAX_PIPELINE_COMPILATION_COUNT;
            } Count;

            struct
            {
                /// <summary>
                /// Max memory of staging buffer for texture loads per frame back buffer
                /// </summary>
                uint32_t textureLoadStagingBuffer
                    = Config::RenderSystem::FrameBudget::MAX_TEXTURE_LOAD_STAGING_BUFFER_MEMORY_SIZE;
                /// <summary>
                /// Max memory of staging buffer for buffer loads per frame back buffer
                /// </summary>
                uint32_t bufferLoadStagingBuffer
                    = Config::RenderSystem::FrameBudget::MAX_BUFFER_LOAD_STAGING_BUFFER_MEMORY_SIZE;
            } Memory;
        };

        public:

            TextureRefPtr LoadTexture( const Core::SLoadFileInfo& );
            BufferViewRefPtr LoadBuffer( const Core::SLoadFileInfo& );
            ShaderRefPtr LoadShader( const Core::SLoadFileInfo& );

        protected:
          TextureArray m_vpTextures;
          BufferArray m_vpBuffers;
          ShaderArray m_vpShaders;
    };

    struct FrameGraphCounterTypes
    {
        enum TYPE
        {
            CPU_FPS,
            CPU_FRAME_TIME,
            GPU_FPS,
            GPU_FRAME_TIME,
            FRAME_MEMORY_UPLOAD_SIZE,
            FRAME_COUNT,
            _MAX_COUNT
        };
    };

    struct SFrameGraphCounter
    {
        enum DATA_TYPE
        {
            INT32,
            UINT32,
            FLOAT32
        };
        union Value
        {
            int32_t i32;
            uint32_t u32;
            float f32;
        };
        Value Min;
        Value Max;
        Value Avg = {};
        Value Total = {};
        uint32_t avgCount = 0;
        DATA_TYPE type = FLOAT32;

        SFrameGraphCounter()
        {
            Min.f32 = std::numeric_limits<float>::max();
            Max.f32 = std::numeric_limits<float>::min();
        }

        template<typename T> T& Get(Value& Val)
        {
            if constexpr(std::is_same_v<T, int32_t>)
            {
                return Val.i32;
            }
            else if constexpr( std::is_same_v<T, uint32_t> )
            {
                return Val.u32;
            }
            else if constexpr( std::is_same_v<T, float> )
            {
                return Val.f32;
            }
            static_assert( std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> || std::is_same_v<T, float>,
                "Only int, uint and float types are valid." );
        }

        template<typename T>
        void Update(T v)
        {
            auto& min = Get<T>( Min );
            auto& max = Get<T>( Max );
            auto& total = Get<T>( Total );
            if(v <= min)
            {
                min = v;
            }
            else if(v >= max)
            {
                max = v;
            }
            else
            {
                avgCount++;
                total += v;
            }
        }

        template<typename T> T CalcAvg()
        {
            auto& total = Get<T>( Total );
            auto& avg = Get<T>( Avg );
            avg = total / ( T )avgCount;
            return avg;
        }
    };
    using FRAME_GRAPH_COUNTER = FrameGraphCounterTypes::TYPE;

    class VKE_API CFrameGraph
    {
        friend class CFrameGraphManager;
        friend class CFrameGraphNode;
        friend class CFrameGraphExecuteNode;
        friend class CResourceLoaddManager;
        friend FrameGraphWorkload;

        using NodeMap = vke_hash_map<vke_string, CFrameGraphNode*>;
        using NodeQueue = vke_queue<CFrameGraphNode*>;
        
        using CommandBufferArray = Utils::TCDynamicArray<CommandBufferPtr>;
        using ResourceNameArray = Utils::TCDynamicArray<ResourceName>;
        using ExecuteBatchArray = Utils::TCDynamicArray<SExecuteBatch>;
        using UintQueue = std::queue<uint32_t>;
        using GPUFenceArray = Utils::TCDynamicArray<NativeAPI::GPUFence>;
        using CPUFenceArray = Utils::TCDynamicArray<NativeAPI::CPUFence>;
        using ThreadFenceArray = Utils::TCDynamicArray<Platform::ThreadFence>;
        using INDEX_TYPE = CFrameGraphNode::index_t;
        using ThreadPtrArray = Utils::TCDynamicArray<std::thread*>;
        using ThreadCVarArray = Utils::TCDynamicArray<std::condition_variable>;
        using TextureMap = vke_hash_map<ShortName, TextureRefPtr>;
        using NodePtrArray = Utils::TCDynamicArray<CFrameGraphNode*, 1>;
        static constexpr auto INVALID_INDEX = CFrameGraphNode::INVALID_INDEX;
        

        static constexpr uint8_t MAX_GRAPHICS_THREAD_COUNT = 4;
        static constexpr uint8_t MAX_BACKBUFFER_COUNT = 4;
        static constexpr uint8_t MAX_EXECUTION_PER_FRAME = 16;
        static constexpr uint8_t MAX_COMMAND_BUFFER_COUNT_PER_FRAME = 32;

        struct SCommandBufferData
        {
            CommandBufferArray vCommandBuffers;
            uint32_t usedCount = 0;
        };

        struct SFrameData
        {
            CommandBufferArray avpCommandBuffers[ ContextTypes::_MAX_COUNT ];
            ExecuteBatchArray avExecutes[ ContextTypes::_MAX_COUNT ];
            GPUFenceArray vGPUFences;
            CPUFenceArray vCPUFences;
            ThreadFenceArray vThreadFences;
            SPresentInfo PresentInfo;
            INDEX_TYPE cpuFenceIndex = INVALID_INDEX;
        };

        struct SBuildInfo
        {
            uint16_t commandBufferCount = 0; // number of required command buffers per frame
        };

        struct SGetCommandBufferInfo
        {
            CONTEXT_TYPE contextType;
            uint8_t threadIndex;
            uint8_t commandBufferIndex;
        };

        struct SCounterManager
        {
            Utils::CTimer FrameTimer;
            Utils::CTimer FPSTimer;
            SFrameGraphCounter aCounters[ FrameGraphCounterTypes::_MAX_COUNT ];
        };

        struct SThreadData
        {
            struct SWorkload
            {
                FrameGraphWorkload Func;
                CFrameGraphNode* pNode;
                uint8_t backBufferIndex;
            };
            bool needExit = false;
            std::mutex Mutex;
            std::condition_variable CondVar;
            std::deque<SWorkload> qWorkloads;
            
        };
        using ThreadDataPtrArray = Utils::TCDynamicArray<SThreadData*>;

      public:
        CFrameGraphNode* CreatePass( const SFrameGraphPassDesc& );
        CFrameGraphExecuteNode* CreateExecutePass( const SFrameGraphNodeDesc& );
        CFrameGraphNode* CreatePresentPass( const SFrameGraphNodeDesc& );
        template<class T> T* CreateCustomPass( const SFrameGraphPassDesc&, const void* );
        Result Build();

        void Run();

        Result SetupPresent( CSwapChain* const, uint8_t );

        CDeviceContext* GetDevice() { return m_Desc.pDevice; }

        
        Result EndFrame();

        CFrameGraphNode* SetRootNode( CFrameGraphNode* pNode )
        {
            m_pRootNode = pNode;
            return m_pRootNode;
        }

        const NativeAPI::CPUFence& GetFrameCPUFence(uint8_t backBufferIndex) const
        {
            return m_ahFrameCPUFences[ backBufferIndex ];
        }

        const SFrameGraphCounter& GetCounter( FRAME_GRAPH_COUNTER type ) const
        {
            return m_CounterMgr.aCounters[ type ];
        }

        void UpdateCounters();

        CFrameGraphNode* GetNode(const char* pName)
        {
            return _GetNode<CFrameGraphNode>( pName );
        }

        CFrameGraphNode* GetPass( const char* pName )
        {
            return GetNode( pName );
        }

        CResourceLoaddManager* GetLoadManager()
        {
            return m_pLoadMgr;
        }

        uint32_t GetFrameIndex() const
        {
            return m_currentFrameIndex;
        }

      protected:
        Result _Create( const SFrameGraphDesc& );
        void _Destroy();
        bool _Validate(CFrameGraphNode*);
        Result _Build( CFrameGraphNode* );

        Result _BeginFrame();

        Result _GetNextFrame();
        void _AcquireCommandBuffers();
        
        //CommandBufferPtr _GetCommandBuffer( const SGetCommandBufferInfo& );
        CommandBufferRefPtr _GetCommandBuffer( const CFrameGraphNode* const, uint8_t backBufferIdx );
        INDEX_TYPE _CreateCommandBuffer( const CFrameGraphNode* const );
        INDEX_TYPE _CreateExecute( const CFrameGraphNode* const );
        INDEX_TYPE _CreateCPUFence( const CFrameGraphNode* const );
        INDEX_TYPE _CreateGPUFence( const CFrameGraphNode* const );
        INDEX_TYPE _CreateThreadFence( const CFrameGraphNode* const );
        INDEX_TYPE _CreateThreadIndex( const std::string_view& );

        TextureRefPtr _GetTexture( const SFrameGraphRenderTargetTextureDesc& );
        Rect2DI32 _GetRenderArea( RENDER_PASS_SIZE );

        SBeginRenderPassInfo2* _CreateBeginRenderPassInfo( const SFrameGraphNodeDesc& );
        SExecuteBatch& _GetExecute( const CFrameGraphNode* const pNode, uint8_t backBufferIndex ){
            return m_aFrameData[backBufferIndex].avExecutes[ pNode->m_ctxType ][ pNode->m_Index.execute ];
        }
        Result _ExecuteBatch( SExecuteBatch* );
        Result _ExecuteBatch( CONTEXT_TYPE ctxType, uint8_t backBufferIndex, uint32_t index )
        {
            return _ExecuteBatch( &m_aFrameData[backBufferIndex].avExecutes[ ctxType ][ index ] );
        }
        Result _ExecuteBatch( CFrameGraphExecuteNode*, uint8_t backBufferIndex );
        Result _BuildDataToExecute( CFrameGraphExecuteNode* pNode, uint8_t backBufferIndex )
        {
            return pNode->_BuildDataToExecute( backBufferIndex );
        }
        void _Reset( SExecuteBatch* );

        CContextBase* _GetContext(const CFrameGraphNode* const pNode) { return m_Desc.apContexts[pNode->m_ctxType]; }

        void _ExecuteNode( CFrameGraphNode* );
        void _ExecuteSubpassNodes( CFrameGraphNode* );
        //CFrameGraphNode& _GetNode( const std::string_view& Name ) { return m_mNodes[Name]; }

        NativeAPI::GPUFence& _GetGPUFence( INDEX_TYPE index, uint32_t backBufferIndex ) const
        {
            return m_aFrameData[backBufferIndex].vGPUFences[ index ];
        }
        NativeAPI::CPUFence& _GetCPUFence( INDEX_TYPE index, uint32_t backBufferIndex ) const
        {
            return m_aFrameData[ backBufferIndex ].vCPUFences[ index ];
        }
        Platform::ThreadFence& _GetThreadFence( INDEX_TYPE index, uint32_t backBufferIndex ) const
        {
            return m_aFrameData[ backBufferIndex ].vThreadFences[ index ];
        }

        Result _OnCreateNode( const SFrameGraphNodeDesc&, CFrameGraphNode** );
        template<class T> T* _CreateNode( const SFrameGraphNodeDesc& );
        template<class T> T* _GetNode( const char* );

        uint8_t _GetBackBufferIndex( uint8_t frameIndex ) const
        {
            return frameIndex;
        }

        static void _ThreadFunc( const CFrameGraph*, uint32_t );
        SThreadData& _GetThreadData( uint32_t ) const;

        
        CFrameGraphNode* _SetNextNode( CFrameGraphNode** ppCurrNode, CFrameGraphNode* pNext );
        void _IsNodeEnabled( CFrameGraphNode** ppCurrNode, bool );

      protected:
        SFrameGraphDesc m_Desc;
        CResourceLoaddManager* m_pLoadMgr = nullptr;
        NodeMap m_mNodes;
        Threads::SyncObject m_FinishedFrameIndicesSyncObj;
        /// <summary>
        /// On frame end, frame index is pushed on the queue
        /// To indicate threads which frames are executed
        /// </summary>
        UintQueue m_qFinishedFrameIndices;
        SBuildInfo m_BuildInfo;
        uint32_t m_currentFrameIndex = 0;
        uint8_t m_backBufferIndex = MAX_BACKBUFFER_COUNT-1; // start frames from 0
        
        ResourceNameArray m_avCommandBufferNames[ ContextTypes::_MAX_COUNT ];
        ResourceNameArray m_avExecuteNames[ ContextTypes::_MAX_COUNT ];
        ResourceNameArray m_vThreadNames;
        ThreadPtrArray m_vpThreads;
        ThreadDataPtrArray m_vpThreadData;

        NativeAPI::CPUFence m_ahFrameCPUFences[ MAX_BACKBUFFER_COUNT ] = {NativeAPI::Null};

        SFrameData m_aFrameData[ MAX_BACKBUFFER_COUNT ];
        //SFrameData* m_pCurrentFrameData = &m_aFrameData[0];
        CFrameGraphNode*    m_pRootNode = nullptr;
        CFrameGraphNode*    m_pLastNode = nullptr;
        NodePtrArray        m_vpNextNodes;
        SCounterManager     m_CounterMgr;

        TextureMap m_mRenderTargets;

        bool m_isValidated = false;
        bool m_needBuild = true;
    };

    template<class T>
    T* CFrameGraph::_GetNode(const char* pName)
    {
        T* pNode = nullptr;
        auto Itr = m_mNodes.find( pName );
        if( Itr != m_mNodes.end() )
        {
            pNode = static_cast<T*>( Itr->second );
        }
        return pNode;
    }

    template<class T>
    T* CFrameGraph::_CreateNode( const SFrameGraphNodeDesc& Desc)
    {
        T* pNode = _GetNode<T>( Desc.pName );
        if( pNode == nullptr )
        {
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pNode ) ) )
            {
                pNode->m_pFrameGraph = this;
                if(VKE_SUCCEEDED(pNode->_Create(Desc)))
                {
                    m_mNodes.insert( std::pair( Desc.pName, pNode ) );
                }
                else
                {
                    Memory::DestroyObject( &HeapAllocator, &pNode );
                }
            }
        }
        return pNode;
    }

    template<class T> T* CFrameGraph::CreateCustomPass( const SFrameGraphNodeDesc& Desc, const void* pDesc )
    {
        T* pPass = _CreateNode( Desc );
        if(pPass != nullptr)
        {
            pPass->Init( pDesc );
        }
        return pPass;
    }
} // VKE