#pragma once

#include "RenderSystem/CFrameGraphNode.h"
#include "RenderSystem/Common.h"
#include "Core/Math/Math.h"
#include "Core/Utils/TCBitset.h"
#include "RenderSystem/CCommandBuffer.h"
#include "RenderSystem/Resources/CTexture.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "RenderSystem/Resources/CShader.h"

namespace VKE::Scene
{
    class CScene;
    using ScenePtr = Utils::TCWeakPtr< class CScene >;
} // namespace VKE::Scene

namespace VKE::RenderSystem
{
    class VKE_API CResourceLoadManager
    {
        friend class CFrameGraph;
        friend class CFrameGraphExecuteNode;

        using TextureArray = Utils::TCDynamicArray< TextureRefPtr >;
        using BufferArray  = Utils::TCDynamicArray< BufferRefPtr >;
        using ShaderArray  = Utils::TCDynamicArray< ShaderRefPtr >;

        using FileQueue     = vke_queue< Core::SLoadFileInfo >;
        using TextureQueue  = FileQueue;
        using BufferQueue   = FileQueue;
        using ShaderQueue   = vke_queue< SCreateShaderDesc >;
        using PipelineQueue = vke_queue< SPipelineDesc >;

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
                uint32_t textureLoadStagingBuffer =
                    Config::RenderSystem::FrameBudget::MAX_TEXTURE_LOAD_STAGING_BUFFER_MEMORY_SIZE;
                /// <summary>
                /// Max memory of staging buffer for buffer loads per frame back buffer
                /// </summary>
                uint32_t bufferLoadStagingBuffer =
                    Config::RenderSystem::FrameBudget::MAX_BUFFER_LOAD_STAGING_BUFFER_MEMORY_SIZE;
            } Memory;
        };

    public:
        TextureRefPtr    LoadTexture( const Core::SLoadFileInfo& );
        BufferViewRefPtr LoadBuffer( const Core::SLoadFileInfo& );
        ShaderRefPtr     LoadShader( const SCreateShaderDesc& );
        PipelineRefPtr   CreatePipeline( const SPipelineDesc& );

    protected:
        Result LoadNextTexture();
        Result LoadNextBuffer();
        Result LoadNextShader();
        Result CreateNextPipeline();

    protected:
        Threads::SyncObject m_TextureSyncObj;
        TextureQueue        m_qTextures;
        Threads::SyncObject m_BufferSyncObj;
        BufferQueue         m_qBuffers;
        Threads::SyncObject m_ShaderSyncObj;
        ShaderQueue         m_qShaders;
        Threads::SyncObject m_PipelineSyncObj;
        PipelineQueue       m_qPipelines;
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
            int32_t  i32;
            uint32_t u32;
            float    f32;
        };

        Value     Min;
        Value     Max;
        Value     Avg      = {};
        Value     Total    = {};
        uint32_t  avgCount = 1;
        DATA_TYPE type     = FLOAT32;

        SFrameGraphCounter()
        {
            Min.f32 = std::numeric_limits< float >::max();
            Max.f32 = std::numeric_limits< float >::min();
        }

        template< typename T >
        T& Get( Value& Val )
        {
            if constexpr( std::is_same_v< T, int32_t > )
            {
                return Val.i32;
            }
            else if constexpr( std::is_same_v< T, uint32_t > )
            {
                return Val.u32;
            }
            else if constexpr( std::is_same_v< T, float > )
            {
                return Val.f32;
            }
            static_assert( std::is_same_v< T, int32_t > || std::is_same_v< T, uint32_t > || std::is_same_v< T, float >,
                           "Only int, uint and float types are valid." );
        }

        template< typename T >
        void UpdateAverage( T v )
        {
            auto& min   = Get< T >( Min );
            auto& max   = Get< T >( Max );
            auto& total = Get< T >( Total );
            if( v <= min )
            {
                min = v;
            }
            else if( v >= max )
            {
                max = v;
            }
            else
            {
                avgCount++;
                total += v;
            }
        }

        template< typename T >
        void Set( T v )
        {
            Get< T >( Min )   = v;
            Get< T >( Max )   = v;
            Get< T >( Total ) = v;
        }

        template< typename T >
        T CalcAvg()
        {
            auto& total = Get< T >( Total );
            auto& avg   = Get< T >( Avg );
            avg         = total / (T)avgCount;
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

        using NodeMap   = vke_hash_map< vke_string, CFrameGraphNode* >;
        using NodeQueue = vke_queue< CFrameGraphNode* >;

        using CommandBufferArray            = Utils::TCDynamicArray< CommandBufferPtr >;
        using ResourceNameArray             = Utils::TCDynamicArray< ResourceName >;
        using ExecuteBatchArray             = Utils::TCDynamicArray< SExecuteBatch >;
        using UintQueue                     = std::queue< uint32_t >;
        using GPUFenceArray                 = Utils::TCDynamicArray< NativeTypes::GPUFence >;
        using CPUFenceArray                 = Utils::TCDynamicArray< NativeTypes::CPUFence >;
        using ThreadFenceArray              = Utils::TCDynamicArray< Platform::ThreadFence >;
        using INDEX_TYPE                    = CFrameGraphNode::index_t;
        using ThreadPtrArray                = Utils::TCDynamicArray< std::thread* >;
        using ThreadCVarArray               = Utils::TCDynamicArray< std::condition_variable >;
        using TextureMap                    = vke_hash_map< ShortName, TextureRefPtr >;
        using NodePtrArray                  = Utils::TCDynamicArray< CFrameGraphNode*, 1 >;
        static constexpr auto INVALID_INDEX = CFrameGraphNode::INVALID_INDEX;

        static constexpr uint8_t MAX_GRAPHICS_THREAD_COUNT          = 4;
        static constexpr uint8_t MAX_BACKBUFFER_COUNT               = 4;
        static constexpr uint8_t MAX_EXECUTION_PER_FRAME            = 16;
        static constexpr uint8_t MAX_COMMAND_BUFFER_COUNT_PER_FRAME = 32;

        struct SCommandBufferData
        {
            CommandBufferArray vCommandBuffers;
            uint32_t           usedCount = 0;
        };

        struct SFrameData
        {
            /// <summary>
            /// Holds all command buffers. Passes just reference these command buffers.
            /// </summary>
            CommandBufferArray avpCommandBuffers[ ContextTypes::_MAX_COUNT ];
            NativeTypes::Fence   hFrameFence;
            /// <summary>
            /// Indicates at what value the fence must wait to check if a frame is finished on GPU
            /// </summary>
            std::atomic_uint64_t           frameFenceValue = 0;
            /// <summary>
            /// How many times this back buffer was used.
            /// This value is mainly use to calculate advancing monitored fence values.
            /// </summary>
            uint64_t              localUseIndex   = 1;
            //GPUFenceArray      vGPUFences;
            //CPUFenceArray      vCPUFences;
            ThreadFenceArray   vThreadFences;
            SPresentInfo       PresentInfo;
            INDEX_TYPE         cpuFenceIndex = INVALID_INDEX;
        };

        struct SBuildInfo
        {
            uint16_t commandBufferCount = 0; // number of required command buffers per frame
        };

        struct SGetCommandBufferInfo
        {
            CONTEXT_TYPE contextType;
            uint8_t      threadIndex;
            uint8_t      commandBufferIndex;
        };

        struct SCounterManager
        {
            Utils::CTimer      FrameTimer;
            Utils::CTimer      FPSTimer;
            SFrameGraphCounter aCounters[ FrameGraphCounterTypes::_MAX_COUNT ];
        };

        struct SThreadData
        {
            struct SWorkload
            {
                FrameGraphWorkload Func;
                CFrameGraphNode*   pNode;
                uint8_t            backBufferIndex;
            };

            bool                    needExit = false;
            std::mutex              Mutex;
            std::condition_variable CondVar;
            std::deque< SWorkload > qWorkloads;
        };

        using ThreadDataPtrArray = Utils::TCDynamicArray< SThreadData* >;

    public:
        CFrameGraphNode*        CreatePass( const SFrameGraphPassDesc& );
        CFrameGraphExecuteNode* CreateExecutePass( const SFrameGraphNodeDesc& );
        CFrameGraphNode*        CreatePresentPass( const SFrameGraphNodeDesc& );
        template< class T >
        T*     CreateCustomPass( const SFrameGraphPassDesc&, const void* );
        Result Build();

        Result Run();

        Result SetupPresent( CSwapChain* const, uint8_t );

        CDeviceContext* GetDevice()
        {
            return m_Desc.pDevice;
        }

        CFrameGraphNode* SetRootNode( CFrameGraphNode* pNode )
        {
            m_pRootNode = pNode;
            return m_pRootNode;
        }

        /*const NativeTypes::CPUFence& GetFrameCPUFence( uint8_t backBufferIndex ) const
        {
            return m_ahFrameCPUFences[ backBufferIndex ];
        }*/

        NativeTypes::Fence GetFrameFence( uint8_t backBufferIndex ) const
        {
            return m_aFrameData[ backBufferIndex ].hFrameFence;
        }

        const SFrameGraphCounter& GetCounter( FRAME_GRAPH_COUNTER type ) const
        {
            return m_CounterMgr.aCounters[ type ];
        }

        void UpdateCounters();

        CFrameGraphNode* GetNode( const char* pName )
        {
            auto pNode = _GetNode< CFrameGraphNode >( pName );
            VKE_ASSERT( pNode != nullptr );
            return pNode;
        }

        CFrameGraphNode* GetPass( const char* pName )
        {
            return GetNode( pName );
        }

        CResourceLoadManager* GetLoadManager()
        {
            return m_pLoadMgr;
        }

        uint32_t GetFrameIndex() const
        {
            return m_currentFrameIndex;
        }
        /// <summary>
        ///  TODO: framegraph should have nothing todo with Scene!
        /// </summary>
        /// <returns></returns>
        Scene::ScenePtr GetScene()
        {
            return m_pScene;
        }

        NativeTypes::FenceValue IncrementFrameFenceValue( uint8_t backBufferIndexx )
        {
            return ++m_aFrameData[ backBufferIndexx ].frameFenceValue;
        }

        NativeTypes::Fence GetFrameFence( uint8_t backBufferIndex )
        {
            return m_aFrameData[ backBufferIndex ].hFrameFence;
        }

        

    protected:
        Result _Create( const SFrameGraphDesc& );
        void   _Destroy();
        bool   _Validate( CFrameGraphNode* );
        Result _Build( CFrameGraphNode* );

        Result _BeginFrame();
        Result _EndFrame();

        Result _GetNextFrame();
        Result   _ResetFrameData( SFrameData* pInOut );
        void   _AcquireCommandBuffers();
        uint32_t _AcquireNodeIndex()
        {
            return ++m_currentNodeIndex;
        }

        // CommandBufferPtr _GetCommandBuffer( const SGetCommandBufferInfo& );
        CommandBufferRefPtr _GetCommandBuffer( const CFrameGraphNode* const, uint8_t backBufferIdx );
        INDEX_TYPE          _CreateCommandBuffer( const CFrameGraphNode* const );
        INDEX_TYPE          _CreateExecute( const CFrameGraphNode* const );

        INDEX_TYPE          _CreateThreadFence( const CFrameGraphNode* const );
        INDEX_TYPE          _CreateThreadIndex( const std::string_view& );

        TextureRefPtr _GetTexture( const SFrameGraphRenderTargetTextureDesc& );
        Rect2DI32     _GetRenderArea( RENDER_PASS_SIZE );

        uint64_t _AdvanceBackBufferFence( uint8_t backBufferIndex )
        {
            return m_aFrameData[ backBufferIndex ].frameFenceValue.fetch_add( 1 );
        }

        SBeginRenderPassInfo2* _CreateBeginRenderPassInfo( const SFrameGraphNodeDesc& );

        CFrameGraphExecuteNode::SExecuteData* _BuildDataToExecute( CFrameGraphExecuteNode* pNode, uint8_t backBufferIndex )
        {
            return pNode->_BuildDataToExecute( backBufferIndex );
        }

        void _Reset( SExecuteBatch* );

        CContextBase* _GetContext( const CFrameGraphNode* const pNode )
        {
            return m_Desc.apContexts[ pNode->m_ctxType ];
        }

        void _ExecuteNode( CFrameGraphNode* );
        void _ExecuteSubpassNodes( CFrameGraphNode* );

        // CFrameGraphNode& _GetNode( const std::string_view& Name ) { return m_mNodes[Name]; }

        /*NativeTypes::GPUFence& _GetGPUFence( INDEX_TYPE index, uint32_t backBufferIndex ) const
        {
            return m_aFrameData[ backBufferIndex ].vGPUFences[ index ];
        }

        NativeTypes::CPUFence& _GetCPUFence( INDEX_TYPE index, uint32_t backBufferIndex ) const
        {
            return m_aFrameData[ backBufferIndex ].vCPUFences[ index ];
        }*/

        Platform::ThreadFence& _GetThreadFence( INDEX_TYPE index, uint32_t backBufferIndex ) const
        {
            return m_aFrameData[ backBufferIndex ].vThreadFences[ index ];
        }

        Result _OnCreateNode( const SFrameGraphNodeDesc&, CFrameGraphNode** );
        template< class T >
        T* _CreateNode( const SFrameGraphNodeDesc& );
        template< class T >
        T* _GetNode( const char* );

        uint8_t _GetBackBufferIndex( uint8_t frameIndex ) const
        {
            return frameIndex;
        }

        static void  _ThreadFunc( const CFrameGraph*, uint32_t );
        SThreadData& _GetThreadData( uint32_t ) const;

        CFrameGraphNode* _SetNextNode( CFrameGraphNode** ppCurrNode, CFrameGraphNode* pNext );
        void             _IsNodeEnabled( CFrameGraphNode** ppCurrNode, bool );

    protected:
        SFrameGraphDesc       m_Desc;
        CResourceLoadManager* m_pLoadMgr = nullptr;
        NodeMap               m_mNodes;
        Threads::SyncObject   m_FinishedFrameIndicesSyncObj;
        /// <summary>
        /// On frame end, frame index is pushed on the queue
        /// To indicate threads which frames are executed
        /// </summary>
        UintQueue  m_qFinishedFrameIndices;
        SBuildInfo m_BuildInfo;
        uint32_t   m_currentFrameIndex = 1; // start from 1 in order to wait for 0 at first frame
        uint32_t   m_currentNodeIndex  = 0;
        uint8_t    m_backBufferIndex   = 0; // start frames from 0

        ResourceNameArray  m_avCommandBufferNames[ ContextTypes::_MAX_COUNT ];
        ResourceNameArray  m_avExecuteNames[ ContextTypes::_MAX_COUNT ];
        ResourceNameArray  m_vThreadNames;
        ThreadPtrArray     m_vpThreads;
        ThreadDataPtrArray m_vpThreadData;

        //NativeTypes::CPUFence m_ahFrameCPUFences[ MAX_BACKBUFFER_COUNT ] = { NativeTypes::Null };

        SFrameData m_aFrameData[ MAX_BACKBUFFER_COUNT ];
        struct
        {
            Threads::SyncObject FrameFence;
        } m_SyncObj;
        // SFrameData* m_pCurrentFrameData = &m_aFrameData[0];
        CFrameGraphNode* m_pRootNode = nullptr;
        CFrameGraphNode* m_pLastNode = nullptr;
        NodePtrArray     m_vpNextNodes;
        SCounterManager  m_CounterMgr;

        // Scene::CScene*      m_pScene = nullptr;
        Scene::ScenePtr m_pScene;

        TextureMap m_mRenderTargets;

        bool m_isValidated = false;
        bool m_needBuild   = true;
    };

    template< class T >
    T* CFrameGraph::_GetNode( const char* pName )
    {
        T*   pNode = nullptr;
        auto Itr   = m_mNodes.find( pName );
        if( Itr != m_mNodes.end() )
        {
            pNode = static_cast< T* >( Itr->second );
        }
        return pNode;
    }

    template< class T >
    T* CFrameGraph::_CreateNode( const SFrameGraphNodeDesc& Desc )
    {
        T* pNode = _GetNode< T >( Desc.pName );
        if( pNode == nullptr )
        {
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pNode ) ) )
            {
                pNode->m_pFrameGraph = this;
                if( VKE_SUCCEEDED( pNode->_Create( Desc ) ) )
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

    template< class T >
    T* CFrameGraph::CreateCustomPass( const SFrameGraphNodeDesc& Desc, const void* pDesc )
    {
        T* pPass = _CreateNode( Desc );
        if( pPass != nullptr )
        {
            pPass->Init( pDesc );
        }
        return pPass;
    }
} // namespace VKE::RenderSystem