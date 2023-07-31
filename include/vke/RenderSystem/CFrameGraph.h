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

        

        struct SWaitInfo
      {
            CFrameGraphNode* pNode;
          uint8_t frameIndex = 0; // 0 = current frame, +1 = next frame, +2 next frame of next frame
          WaitOnFlags WaitOn = WaitOnBits::NONE;
      };

        protected:
            using WaitArray = Utils::TCDynamicArray<SWaitInfo, 1>;

      public:

          virtual ~CFrameGraphNode()
        {
        }
        CFrameGraphNode* AddSubpass( CFrameGraphNode* );
        bool IsSubpassEnabled( const ResourceName& );
        void SetWorkload( FrameGraphWorkload&& Func )
        {
            m_Workload = std::move(Func);
        }

        CContextBase* GetContext() {return m_pContext;}
        
        CommandBufferPtr GetCommandBuffer() { return m_pCommandBuffer; }

        bool IsEnabled() const { return m_isEnabled; }
        void IsEnabled(bool isEnabled) { m_isEnabled = isEnabled; }

        void WaitFor( const SWaitInfo& );

        void AddSynchronization( const SyncObject& Obj ) { m_vSyncObjects.PushBack( Obj ); }

        NativeAPI::GPUFence& GetGPUFence() const;
        NativeAPI::CPUFence& GetCPUFence() const;
        Platform::ThreadFence& GetThreadFence() const;

        void SignalThreadFence( uint32_t value );
        Result Wait( const NativeAPI::GPUFence& );
        Result Wait( const NativeAPI::CPUFence&, uint64_t timeout );
        Result Wait( const Platform::ThreadFence&, uint32_t value, uint64_t timeout );

        CFrameGraphNode* AddNext( CFrameGraphNode* );

        protected:
            Result _Create( const SFrameGraphPassDesc& );
            void _Destroy();
            void _Run();

            void _SignalGPUFence();

      protected:

        CONTEXT_TYPE m_ctxType;
        FrameGraphWorkload m_Workload;
        CFrameGraph* m_pFrameGraph = nullptr;
        NodeQueue m_qSubpasses;
        CFrameGraphNode* m_pParent = nullptr;
        CFrameGraphNode* m_pPrev = nullptr;
        CFrameGraphExecuteNode* m_pExecuteNode = nullptr;
        NodeArray m_vpSubpassNodes;
        NodeArray m_vpNextNodes;
        WaitArray m_vWaitForNodes;
        SyncObjArray m_vSyncObjects;
        CContextBase* m_pContext = nullptr;
        CommandBufferRefPtr m_pCommandBuffer;
        SIndex m_Index;
        /// <summary>
        /// if true, this node will execute command buffers.
        /// Usually that means that this node is the last one using particular ExecuteBatch
        /// This member is set in CFrameGraph::Build
        /// </summary>
        bool m_doExecute = false;
        bool m_isEnabled = true;

        vke_string m_Name;
        vke_string m_ThreadName;
        vke_string m_CommandBufferName;
        vke_string m_ExecuteName;

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
          Result _BuildDataToExecute();

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

      public:
        CFrameGraphNode* CreatePass( const SFrameGraphPassDesc& );
        CFrameGraphExecuteNode* CreateExecutePass( const SFrameGraphNodeDesc& );
        CFrameGraphNode* CreatePresentPass( const SFrameGraphNodeDesc& );
        Result Build();

        void Run();

        Result SetupPresent( CSwapChain* const );

        CDeviceContext* GetDevice() { return m_Desc.pDevice; }

        
        Result EndFrame();

        CFrameGraphNode* SetRootNode( CFrameGraphNode* pNode )
        {
            m_pRootNode = pNode;
            return m_pRootNode;
        }

        const NativeAPI::CPUFence& GetFrameCPUFence() const
        {
            return m_pCurrentFrameData->vCPUFences[ m_pCurrentFrameData->cpuFenceIndex ];
        }

      protected:
        Result _Create( const SFrameGraphDesc& );
        void _Destroy();
        bool _Validate(CFrameGraphNode*);
        Result _Build( CFrameGraphNode* );

        Result _OnCreateNode( CFrameGraphNode** );

        Result _BeginFrame();

        Result _GetNextFrame();
        void _AcquireCommandBuffers();
        
        //CommandBufferPtr _GetCommandBuffer( const SGetCommandBufferInfo& );
        CommandBufferRefPtr _GetCommandBuffer( const CFrameGraphNode* const );
        INDEX_TYPE _CreateCommandBuffer( const CFrameGraphNode* const );
        INDEX_TYPE _GetThreadIndex( const std::string_view& );
        INDEX_TYPE _GetThreadIndex( const CFrameGraphNode* const pNode );
        INDEX_TYPE _CreateExecute( const CFrameGraphNode* const );
        INDEX_TYPE _CreateCPUFence( const CFrameGraphNode* const );
        INDEX_TYPE _CreateGPUFence( const CFrameGraphNode* const );
        INDEX_TYPE _CreateThreadFence( const CFrameGraphNode* const );
        SExecuteBatch& _GetExecute( const CFrameGraphNode* const pNode ){
            return m_pCurrentFrameData->avExecutes[ pNode->m_ctxType ][ pNode->m_Index.execute ];
        }
        Result _ExecuteBatch( SExecuteBatch* );
        Result _ExecuteBatch( CONTEXT_TYPE ctxType, uint32_t index ) { return _ExecuteBatch( &m_pCurrentFrameData->avExecutes[ ctxType ][ index ] ); }
        Result _ExecuteBatch( CFrameGraphExecuteNode* );
        Result _BuildDataToExecute( CFrameGraphExecuteNode* );
        void _Reset( SExecuteBatch* );

        CContextBase* _GetContext(const CFrameGraphNode* const pNode) { return m_Desc.apContexts[pNode->m_ctxType]; }

        void _ExecuteNode( CFrameGraphNode* );
        //CFrameGraphNode& _GetNode( const std::string_view& Name ) { return m_mNodes[Name]; }

        NativeAPI::GPUFence& _GetGPUFence(INDEX_TYPE index) const
        {
            return m_pCurrentFrameData->vGPUFences[ index ];
        }

        NativeAPI::CPUFence& _GetCPUFence(INDEX_TYPE index) const
        {
            return m_pCurrentFrameData->vCPUFences[ index ];
        }

        Platform::ThreadFence& _GetThreadFence(INDEX_TYPE index) const
        {
            return m_pCurrentFrameData->vThreadFences[ index ];
        }

        template<class T> T* _CreateNode( const SFrameGraphNodeDesc& );

        uint8_t _GetBackBufferIndex( uint8_t frameIndex ) const
        {
            return frameIndex;
        }

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
        uint8_t m_backBufferIndex = MAX_BACKBUFFER_COUNT-1; // start frames from 0
        
        ResourceNameArray m_avCommandBufferNames[ ContextTypes::_MAX_COUNT ];
        ResourceNameArray m_avExecuteNames[ ContextTypes::_MAX_COUNT ];
        ResourceNameArray m_vThreadNames;

        NativeAPI::CPUFence m_ahFrameCPUFences[ MAX_BACKBUFFER_COUNT ] = {NativeAPI::Null};

        SFrameData m_aFrameData[ MAX_BACKBUFFER_COUNT ];
        SFrameData* m_pCurrentFrameData = &m_aFrameData[0];
        CFrameGraphNode* m_pRootNode = nullptr;
        bool m_isValidated = false;
        bool m_needBuild = true;
    };

    template<class T>
    T* CFrameGraph::_CreateNode( const SFrameGraphNodeDesc& Desc)
    {
        T* pNode = nullptr;
        auto Itr = m_mNodes.find( Desc.pName );
        if(Itr != m_mNodes.end())
        {
            pNode = static_cast<T*>(  Itr->second );
        }
        else
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
} // VKE