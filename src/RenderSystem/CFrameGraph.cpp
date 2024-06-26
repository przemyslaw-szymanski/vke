#include "RenderSystem/CFrameGraph.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CSwapChain.h"
#include "RenderSystem/CGraphicsContext.h"

#define VKE_LOG_FRAMEGRAPH 0

namespace VKE::RenderSystem
{
    Result CFrameGraph::_Create( const SFrameGraphDesc& Desc)
    {
        Result ret = VKE_OK;
        m_Desc = Desc;
        Memory::CreateObject( &HeapAllocator, &m_pLoadMgr );
        VKE_ASSERT( m_Desc.pDevice != nullptr );

        return ret;
    }

    void CFrameGraph::_Destroy()
    {
        // Notify all threads to stop
        for(uint32_t i = 0; i < m_vpThreads.GetCount(); ++i)
        {
            auto pData = m_vpThreadData[ i ];
            pData->needExit = true;
            pData->qWorkloads.clear();
            pData->CondVar.notify_all();
        }
        // Wait for threads and destroy them
        for(uint32_t i = 0; i < m_vpThreads.GetCount(); ++i)
        {
            m_vpThreads[ i ]->join();
            Memory::DestroyObject( &HeapAllocator, &m_vpThreads[ i ] );
            Memory::DestroyObject( &HeapAllocator, &m_vpThreadData[ i ] );
        }
        m_vpThreads.Clear();
        m_vpThreadData.Clear();

        for (auto& Pair : m_mNodes)
        {
            Pair.second->_Destroy();
            Memory::DestroyObject( &HeapAllocator, &Pair.second );
        }
        m_mNodes.clear();
        Memory::DestroyObject( &HeapAllocator, &m_pLoadMgr );
    }

    Result CFrameGraph::_GetNextFrame()
    {
        Result ret = VKE_OK;
        ++m_currentFrameIndex;
        m_backBufferIndex = ( m_backBufferIndex + 1 ) % MAX_BACKBUFFER_COUNT;
        // Get first free frame
        bool needWait = true;
        for( uint32_t i = 0; i < MAX_BACKBUFFER_COUNT; ++i )
        {
            if( m_ahFrameCPUFences[ m_backBufferIndex ] == NativeAPI::Null
                || m_Desc.pDevice->IsReadyToUse( m_ahFrameCPUFences[ m_backBufferIndex ] ) )
            {
                needWait = false;
                break;
            }
            else
            {
                m_backBufferIndex = ( m_backBufferIndex + 1 ) % MAX_BACKBUFFER_COUNT;
            }
        }
        if( needWait )
        {
            // If no frame is executed, wait for first one
            m_backBufferIndex = 0;
            auto& FrameData = m_aFrameData[ m_backBufferIndex ];
            // Find context to wait on
            for( uint32_t c = 0; c < ContextTypes::_MAX_COUNT; ++c )
            {
                for( uint32_t i = 0; i < FrameData.avExecutes[ c ].GetCount(); ++i )
                {
                    if( FrameData.avExecutes[ c ][ i ].hSignalCPUFence
                        == m_ahFrameCPUFences[ m_backBufferIndex ] )
                    {
                        ret = FrameData.avExecutes[ c ][ i ].pContext->Wait(
                            m_ahFrameCPUFences[ m_backBufferIndex ] );
                        break;
                    }
                }
            }
        }
        return ret;
    }

    Result CFrameGraph::_BeginFrame()
    {
        Result res = _GetNextFrame();
        
        if( VKE_SUCCEEDED( res ) )
        {
            for( uint32_t ctx = 0; ctx < ContextTypes::_MAX_COUNT; ++ctx )
            {
                // auto& vpCbs = m_pCurrentFrameData->avpCommandBuffers[ ctx ];
                auto& vExes = m_aFrameData[m_backBufferIndex].avExecutes[ ctx ];
                for( uint32_t i = 0; i < vExes.GetCount(); ++i )
                {
                    _Reset( &vExes[ i ] );
                }
            }
        }
        return res;
    }

    void CFrameGraph::_Reset( SExecuteBatch* pBatch )
    {
        //auto pContext = pBatch->pContext;
        //auto pDevice = pContext->m_pDeviceCtx;
        //auto& API = pDevice->DDI();
        //bool signaled = API.IsSignaled( pBatch->hSignalCPUFence );
        //bool executed = pBatch->executionResult == Results::OK;
        //bool hasCmdBuffers = !pBatch->vpCommandBuffers.IsEmpty();
        pBatch->executionResult = Results::NOT_READY;
        pBatch->executeFlags = 0;
        pBatch->vDependencies.Clear();
        pBatch->refCount = 0;
        for(uint32_t c = 0; c < pBatch->vpCommandBuffers.GetCount(); ++c)
        {
            pBatch->vpCommandBuffers[ c ]->Reset();
        }
    }

    Result CFrameGraph::EndFrame()
    {
        Result ret = VKE_OK;
        {
            auto& vExecutes = m_aFrameData[ m_backBufferIndex ].avExecutes[ ContextTypes::TRANSFER ];
            for( uint32_t i = 0; i < vExecutes.GetCount(); ++i )
            {
                _ExecuteBatch( &vExecutes[ i ] );
            }
        }
        {
            Threads::ScopedLock l( m_FinishedFrameIndicesSyncObj );
            m_qFinishedFrameIndices.push( m_backBufferIndex );
        }
        return ret;
    }

    Result CFrameGraph::_ExecuteBatch( SExecuteBatch* pBatch )
    {
        Result ret = VKE_OK;
        if(pBatch->refCount > 0)
        {
            ret = pBatch->pContext->_ExecuteBatch( pBatch );
            m_ahFrameCPUFences[ m_backBufferIndex ] = pBatch->hSignalCPUFence;
#if VKE_LOG_FRAMEGRAPH
            VKE_LOG( pBatch->GetDebugName() << ", frame cpu fence bbidx: " << (uint32_t)m_backBufferIndex << " fence: " << pBatch->hSignalCPUFence );
#endif
        }
        return ret;
    }

    Result CFrameGraph::_ExecuteBatch( CFrameGraphExecuteNode* pNode, uint8_t backBufferIndex )
    {
        auto& Exe = _GetExecute( pNode, backBufferIndex );
        return _ExecuteBatch( &Exe );
    }

    void CFrameGraph::_AcquireCommandBuffers()
    {

    }

    Result CFrameGraph::_OnCreateNode( const SFrameGraphNodeDesc& Desc, CFrameGraphNode** ppNode )
    {
        Result ret = VKE_OK;
        auto pNode = *ppNode;
        // By default use parent command buffer
        // auto& ParentNode = m_mNodes[ pNode->m_Desc.ParentName ];
        //const auto& Desc = pNode->m_Desc;
        pNode->m_pContext = m_Desc.apContexts[ pNode->m_ctxType ];

        if( VKE_SUCCEEDED( ret ) )
        {
            pNode->m_Index.cpuFence = _CreateCPUFence( pNode );
            pNode->m_Index.gpuFence = _CreateGPUFence( pNode );
            pNode->m_Index.threadFence = _CreateThreadFence( pNode );
            pNode->m_Index.thread = _CreateThreadIndex( Desc.pThread );

            if( !pNode->m_CommandBufferName.empty() )
            {
                pNode->m_Index.commandBuffer = _CreateCommandBuffer( pNode );
                ret = pNode->m_Index.commandBuffer != INVALID_INDEX ? VKE_OK : VKE_FAIL;
            }
        }
        // Get prev node
        // If root skip it
   
        return ret;
    }

    CFrameGraphNode* CFrameGraph::CreatePass( const SFrameGraphPassDesc& Desc )
    {
        CFrameGraphNode* pRet = _CreateNode<CFrameGraphNode>( Desc );
        return pRet;
    }

    CFrameGraphExecuteNode* CFrameGraph::CreateExecutePass( const SFrameGraphNodeDesc& Desc )
    {
        auto idx = m_avExecuteNames[ Desc.contextType ].Find( Desc.pName );
        if(idx != INVALID_POSITION)
        {
            VKE_LOG_ERR( "FrameGraph: " << Desc.pName << ", Node: " << Desc.pName
                                        << ": Execution: " << Desc.pExecute << " already exists." );
        }
        SFrameGraphNodeDesc NewDesc = Desc;
        
        CFrameGraphExecuteNode* pPass = _CreateNode<CFrameGraphExecuteNode>( Desc );
        if( pPass != nullptr )
        {
            pPass->m_doExecute = true;
            pPass->m_Index.execute = _CreateExecute( static_cast<CFrameGraphNode*>( pPass ) );
            pPass->m_pExecuteNode = pPass;
            pPass->SetWorkload( [ this ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex )  
            {
                Result ret = pPass->OnWorkloadBegin( backBufferIndex );
                if( VKE_SUCCEEDED( ret ) )
                {
                    CFrameGraphExecuteNode* pNode = static_cast<CFrameGraphExecuteNode*>( pPass );
                    ret = pNode->_BuildDataToExecute( backBufferIndex );
                    if( VKE_SUCCEEDED( ret ) )
                    {
#if VKE_LOG_FRAMEGRAPH
                        auto& Exe = _GetExecute( pNode, backBufferIndex );
                        VKE_LOG( pPass->m_Name << ", bbidx: " << (uint32_t)this->m_backBufferIndex << " " << pPass->GetThreadFence().Load() <<  " signal gpufence: " << Exe.hSignalGPUFence );
#endif
                        ret = this->_ExecuteBatch( pNode, backBufferIndex );
                    }
                }
                ret = pPass->OnWorkloadEnd( ret );
                return ret;
            } );
        }
        return pPass;
    }

    void CFrameGraph::UpdateCounters()
    {
        ++m_CounterMgr.aCounters[ FrameGraphCounterTypes::FRAME_COUNT ].Avg.u32;
        float elapsedCPUTime = ( float )( ( m_CounterMgr.FrameTimer.GetElapsedTime() ) * 0.001f );
        float elapsedCPUTime2 = ( float )( ( m_CounterMgr.FPSTimer.GetElapsedTime() ) * 0.001f );
        auto& CPUFrameTime = m_CounterMgr.aCounters[ FrameGraphCounterTypes::CPU_FRAME_TIME ];
        CPUFrameTime.Update( elapsedCPUTime );
        auto& CPUFps = m_CounterMgr.aCounters[ FrameGraphCounterTypes::CPU_FPS ];
        CPUFps.Total.u32++;
        if( elapsedCPUTime2 >= 1000 )
        {
            CPUFps.Avg.u32 = CPUFps.Total.u32;
            CPUFps.Total.u32 = 0;
            m_CounterMgr.FPSTimer.Start();
        }
        CPUFps.Avg.f32 = 1000.0f / CPUFrameTime.CalcAvg<float>();
        m_CounterMgr.FrameTimer.Start();
    }

    CFrameGraphNode* CFrameGraph::CreatePresentPass( const SFrameGraphNodeDesc& Desc )
    {
        auto pRet = CreatePass( Desc );
        if(pRet != nullptr)
        {
            // Present executes as well but via Present api call
            pRet->m_doExecute = true;
            pRet->SetWorkload( [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex )
            {
                Result ret = VKE_OK;
                {
                    ret = pPass->OnWorkloadBegin( backBufferIndex );
                }
                if( VKE_SUCCEEDED( ret ) )
                {
                    uint32_t backBufferIdx = UNDEFINED_U32;
                    if( !m_qFinishedFrameIndices.empty() )
                    {
                        Threads::ScopedLock l( m_FinishedFrameIndicesSyncObj );
                        backBufferIdx = m_qFinishedFrameIndices.front();
                        m_qFinishedFrameIndices.pop();
                    }
                    if( backBufferIdx != UNDEFINED_U32 )
                    {
                        // VKE_LOG( "present" );
                        auto pCtx = pPass->GetContext()->Reinterpret<CGraphicsContext>();
                        auto pSwpChain = pCtx->GetSwapChain();
                        auto hGPUFence = pPass->m_vWaitForNodes.Back().pNode->GetGPUFence( backBufferIdx );
#if VKE_LOG_FRAMEGRAPH
                        VKE_LOG( "bbidx: "
                                 << backBufferIdx << " frame " << m_currentFrameIndex << " wait for thread fence "
                                           << pPass->m_vWaitForNodes.Back().pNode->GetThreadFence().Load() << " present fence " << pPass->GetThreadFence().Load()
                                           << " wait on gpufence: "
                                 << hGPUFence );
#endif
                        auto hFrameFence = m_ahFrameCPUFences[ backBufferIdx ];
                        ret = pSwpChain->Present( hGPUFence, hFrameFence );
                    }
                }
                {
                    ret = pPass->OnWorkloadEnd( ret );
                }
                return ret;
            } );
        }
        return pRet;
    }

    bool CFrameGraph::_Validate(CFrameGraphNode* pNode)
    {
        bool ret = true;
        
        return ret;
    }

    Result CFrameGraph::Build()
    {
        Result ret = VKE_OK;
        if(!m_isValidated)
        {
            m_isValidated = _Validate( m_pRootNode );
        }
        if( m_isValidated )
        {
            if( m_needBuild )
            {
                ret = _Build( m_pRootNode );
                if( VKE_SUCCEEDED( ret ) )
                {
                    //m_needBuild = false;
                }
            }
        }
        return ret;
    }

    Result CFrameGraph::_Build( CFrameGraphNode* pNode )
    {
        Result ret = VKE_OK;
        if( pNode->IsEnabled() )
        {
            
            
        }
        else
        {
            pNode->m_pCommandBuffer = nullptr;
        }
        return ret;
    }

    void CFrameGraph::Run()
    {
        if( VKE_SUCCEEDED( _BeginFrame() ) )
        {
            if( VKE_SUCCEEDED( Build() ) )
            {
                _ExecuteNode( m_pRootNode );
            }
        }
    }

    void CFrameGraph::_ExecuteNode( CFrameGraphNode* pNode )
    {
        if (pNode->m_Name == "ExecuteUpdate")
        {
            bool b = false;
            b = !b;
        }
        for(uint32_t n = 0; n < pNode->m_vpSubpassNodes.GetCount(); ++n)
        {
            auto pSubNode = pNode->m_vpSubpassNodes[ n ];
            if ( pSubNode != nullptr && pSubNode->IsEnabled())
            {
                _ExecuteNode( pSubNode );
            }
        }
        pNode->_Run();
        for( uint32_t n = 0; n < pNode->m_vpNextNodes.GetCount(); ++n )
        {
            if( pNode->m_vpNextNodes[ n ] != nullptr )
            {
                _ExecuteNode( pNode->m_vpNextNodes[ n ] );
            }
        }
    }

    static const cstr_t g_aContextNames[ ContextTypes::_MAX_COUNT ] =
    { 
        "General", //
        "Compute",
        "Transfer",
        "Sparse",
        "Present"
    };

    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateCommandBuffer( const CFrameGraphNode* const pNode )
    {
        INDEX_TYPE ret = INVALID_INDEX;
 
        // Find required command buffer
        const auto ctxType = pNode->m_ctxType;
        ResourceName CmdBufferName = std::format( "{}_{}", pNode->m_CommandBufferName, pNode->m_Index.thread ).c_str();
        auto idx = m_avCommandBufferNames[ ctxType ].Find( CmdBufferName );
        if(idx == INVALID_POSITION)
        {
            auto threadIndex = pNode->m_Index.thread;
            VKE_ASSERT( threadIndex != INVALID_INDEX );
            SCreateCommandBufferInfo CreateInfo =
            {
                .count = MAX_BACKBUFFER_COUNT,
                .threadIndex = (uint8_t)threadIndex
            };
            Utils::TCDynamicArray<CCommandBuffer*, MAX_BACKBUFFER_COUNT> vCbs(CreateInfo.count);
            VKE_ASSERT( m_Desc.apContexts[ ctxType ] != nullptr );
            Result res = m_Desc.apContexts[ ctxType ]->_CreateCommandBuffers( CreateInfo, &vCbs[ 0 ] );
            if( VKE_SUCCEEDED(res) )
            {
                ResourceName DbgName;
                for(uint32_t i = 0; i < vCbs.GetCount(); ++i)
                {
                    DbgName.Format( "%s_backBuffer%d_%s_%s", g_aContextNames[ ctxType ], i,
                                    pNode->m_ExecuteName.data(), pNode->m_CommandBufferName.data() );
                    vCbs[ i ]->SetDebugName( DbgName );
                    vCbs[ i ]->Reset();
                    auto& FrameData = m_aFrameData[ i ];
                    ret = (INDEX_TYPE)FrameData.avpCommandBuffers[ ctxType ].PushBack( CommandBufferPtr{ vCbs[ i ] } );
                }
                idx = (INDEX_TYPE)m_avCommandBufferNames[ pNode->m_ctxType ].PushBack( CmdBufferName );
                VKE_ASSERT( idx == ret );
            }
        }
        else
        {
            ret = (INDEX_TYPE)idx;
        }
        return ret;
    }

    /*CommandBufferPtr CFrameGraph::_GetCommandBuffer( const SGetCommandBufferInfo& Info )
    {
        CommandBufferPtr pRet = m_pCurrentFrameData->avpCommandBuffers[ Info.contextType ][ Info.commandBufferIndex ];
        VKE_ASSERT( pRet.IsValid() );
        return pRet;
    }*/
    CommandBufferRefPtr CFrameGraph::_GetCommandBuffer(const CFrameGraphNode* const pNode, uint8_t backBufferIdx )
    {
        return CommandBufferRefPtr{
            m_aFrameData[backBufferIdx].avpCommandBuffers[ pNode->m_ctxType ][ pNode->m_Index.commandBuffer ]
        };
    }

    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateExecute( const CFrameGraphNode* const pNode )
    {
        INDEX_TYPE ret = INVALID_INDEX;
        const auto ctxType = pNode->m_ctxType;
        auto& vNames = m_avExecuteNames[ ctxType ];
        auto idx = vNames.Find( ResourceName{ pNode->m_Name } );
        if(idx == INVALID_POSITION)
        {
            Result res = VKE_OK;
            //auto threadIndex = _GetThreadIndex( Desc.ThreadName );
            for( uint32_t backBufferIdx = 0; backBufferIdx < MAX_BACKBUFFER_COUNT; ++backBufferIdx )
            {
                auto& FrameData = m_aFrameData[ backBufferIdx ];
                SExecuteBatch Batch;
                VKE_ASSERT( m_Desc.apContexts[ ctxType ] != nullptr );
                /*res = m_Desc.apContexts[ ctxType ]->_CreateExecuteBatch(
                    backBufferIdx, m_avExecuteNames[ ctxType ].GetCount(), &Batch );*/
                Batch.pContext = m_Desc.apContexts[ ctxType ];
                Batch.hSignalCPUFence = FrameData.vCPUFences[ pNode->m_Index.cpuFence ];
                Batch.hSignalGPUFence = FrameData.vGPUFences[ pNode->m_Index.gpuFence ];
                Batch.SetDebugName( pNode->m_Name.c_str() );
                if( VKE_SUCCEEDED(res) )
                {
                    ret = ( INDEX_TYPE )FrameData.avExecutes[ ctxType ].PushBack( Batch );
                }
            }
            if( VKE_SUCCEEDED(res) )
            {
                idx = (INDEX_TYPE)m_avExecuteNames[ ctxType ].PushBack( ResourceName{ pNode->m_ExecuteName } );
                VKE_ASSERT( idx == ret );
            }
        }
        else
        {
            ret = (INDEX_TYPE)idx;
        }
        return ret;
    }

    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateCPUFence( const CFrameGraphNode* const pNode )
    {
        INDEX_TYPE ret = INVALID_INDEX;
        for(uint32_t backBufferIdex = 0; backBufferIdex < MAX_BACKBUFFER_COUNT; ++backBufferIdex)
        {
            SFenceDesc FenceDesc = { .isSignaled = true };
            FenceDesc.SetDebugName( std::format( "{}_{}", pNode->m_Name, backBufferIdex ).c_str() );
            auto hFence = m_Desc.pDevice->CreateCPUFence( FenceDesc );
            VKE_ASSERT( hFence != NativeAPI::Null );
            ret = (INDEX_TYPE)m_aFrameData[ backBufferIdex ].vCPUFences.PushBack( hFence );
        }
        return ret;
    }

    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateGPUFence( const CFrameGraphNode* const pNode )
    {
        INDEX_TYPE ret = INVALID_INDEX;
        for( uint32_t backBufferIdex = 0; backBufferIdex < MAX_BACKBUFFER_COUNT; ++backBufferIdex )
        {
            SSemaphoreDesc FenceDesc;
            FenceDesc.SetDebugName( std::format( "{}_{}", pNode->m_Name, backBufferIdex ).c_str() );
            auto hFence = m_Desc.pDevice->CreateGPUFence( FenceDesc );
            VKE_ASSERT( hFence != NativeAPI::Null );
            ret = (INDEX_TYPE)m_aFrameData[ backBufferIdex ].vGPUFences.PushBack( hFence );
        }
        return ret;
    }

    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateThreadFence( const CFrameGraphNode* const )
    {
        INDEX_TYPE ret = INVALID_INDEX;
        for( uint32_t backBufferIdex = 0; backBufferIdex < MAX_BACKBUFFER_COUNT; ++backBufferIdex )
        {
            ret = (INDEX_TYPE)m_aFrameData[ backBufferIdex ].vThreadFences.PushBack( {0} );
        }
        return ret;
    }

    CFrameGraph::SThreadData& CFrameGraph::_GetThreadData( uint32_t threadindex ) const
    {
        return *m_vpThreadData[ threadindex ];
    }

    void CFrameGraph::_ThreadFunc(const CFrameGraph* pFrameGraph, uint32_t index)
    {
        using namespace std::chrono_literals;
        CFrameGraph::SThreadData& ThreadData = pFrameGraph->_GetThreadData( index );
        while( !ThreadData.needExit )
        {
            std::unique_lock<std::mutex> l( ThreadData.Mutex );
            if( ThreadData.CondVar.wait_for( l, 2s,
                                                       [&] { return !ThreadData.qWorkloads.empty(); } ) )
            {
                if( !ThreadData.qWorkloads.empty() )
                {
                    auto Workload = ThreadData.qWorkloads.front();
                    ThreadData.qWorkloads.pop_front();
                    Workload.Func( Workload.pNode, Workload.backBufferIndex );
                }
            }
        }
    }


    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateThreadIndex( const std::string_view& ThreadName )
    {
        INDEX_TYPE ret = INVALID_INDEX;
        auto idx = m_vThreadNames.Find( ResourceName{ ThreadName } );
        if( idx == INVALID_POSITION ) // such thread name is not present
        {
            //VKE_ASSERT( m_vThreadNames.GetCount() < MAX_GRAPHICS_THREAD_COUNT );
            ret = ( INDEX_TYPE )( m_vThreadNames.PushBack( ResourceName{ ThreadName } ) );
            std::thread* pThread;
            if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pThread, CFrameGraph::_ThreadFunc, this, ret ) ) )
            {
                INDEX_TYPE ret2 = ( INDEX_TYPE )( m_vpThreads.PushBack( ( pThread ) ) );
                INDEX_TYPE ret3 = INVALID_INDEX;
                SThreadData* pData;
                if( VKE_SUCCEEDED( Memory::CreateObject( &HeapAllocator, &pData ) ) )
                {
                    ret3 = ( INDEX_TYPE )( m_vpThreadData.PushBack( pData ) );
                }
                else
                {
                    ret = INVALID_INDEX;
                }
                VKE_ASSERT( ret == ret2 && ret2 == ret3 );
            }
        }
        else
        {
            ret = ( INDEX_TYPE )idx;
        }
        return ret;
    }

    Result CFrameGraph::SetupPresent( CSwapChain* const pSwapChain, uint8_t backBufferIdx )
    {
        Result ret = VKE_OK;
        auto& BackBuffer = pSwapChain->GetCurrentBackBuffer();
        m_aFrameData[ backBufferIdx ].PresentInfo.pSwapChain = pSwapChain;
        m_aFrameData[ backBufferIdx ].PresentInfo.imageIndex = BackBuffer.ddiBackBufferIdx;
        return ret;
    }
    
} // VKE::RenderSystem

namespace VKE::RenderSystem
{
    const FrameGraphWorkload CFrameGraphNode::EmptyWorkload = []( const CFrameGraphNode* const, uint8_t ) { return VKE_OK; };

    Result CFrameGraphNode::_Create( const SFrameGraphPassDesc& Desc )
    {
        Result ret = VKE_OK;
        m_ctxType = Desc.contextType;
        m_Name = Desc.pName;
        m_ThreadName = Desc.pThread;
        m_CommandBufferName = Desc.pCommandBuffer;
        m_ExecuteName = Desc.pExecute;
        m_Workload = EmptyWorkload;
        m_isAsync = strcmp( Desc.pThread, "Main" ) != 0;
        auto pThis = this;
        ret = m_pFrameGraph->_OnCreateNode( Desc, &pThis );
        return ret;
    }
    void CFrameGraphNode::_Destroy()
    {
    }

    Result CFrameGraphNode::_WaitForThreads()
    {
        Result ret = VKE_OK;
        //auto threadFenceValue = GetThreadFence().Load();
        for(uint32_t i = 0; i < m_vWaitForNodes.GetCount(); ++i)
        {
            auto& WaitInfo = m_vWaitForNodes[ i ];
            if( WaitInfo.WaitOn == WaitOnBits::THREAD )
            {
                uint64_t timeout = 2 * 1000 * 1000; // 2 seconds
                //ret = Wait( m_vWaitForNodes[ i ].pNode->GetThreadFence(), threadFenceValue, timeout );
                ret = WaitForFrame( WaitInfo.pNode->GetThreadFence(), WaitInfo.frame, timeout );
                if(!VKE_SUCCEEDED(ret))
                {
                    VKE_LOG_ERR( "A node: '" << this->m_Name << "' reached timeout waiting for node: '"
                                             << m_vWaitForNodes[ i ].pNode->m_Name << "'"
                                             << " with value: " << WaitInfo.pNode->GetThreadFence().Load() );
                    break;
                }
            }
        }
        return ret;
    }

    Result CFrameGraphNode::OnWorkloadBegin(uint8_t backBufferIndex)
    {
        /// TODO: WaitForThreads can cause huge CPU overhead
        Result ret = _WaitForThreads();
        if( VKE_SUCCEEDED( ret ) )
        {
            m_pCommandBuffer = m_pFrameGraph->_GetCommandBuffer( this, backBufferIndex );
            if( m_pCommandBuffer->GetState() == CommandBufferStates::EXECUTED )
            {
                m_pCommandBuffer->Reset();
            }
            if( m_pCommandBuffer->GetState() == CommandBufferStates::RESET )
            {
                m_pCommandBuffer->Begin();
            }
        }
        m_vSyncObjects.Clear();
        m_finished = false;
        return ret;
    }

    Result CFrameGraphNode::OnWorkloadEnd(Result workloadResult)
    {
        IncrementThreadFence();
        m_finished = true;
        return workloadResult;
    }

    Result CFrameGraphNode::_Run()
    {
        Result res = VKE_OK;
        if(m_isAsync)
        {
            auto& ThreadData = m_pFrameGraph->_GetThreadData( m_Index.thread );
            
            ThreadData.qWorkloads.push_back( { m_Workload, this, m_pFrameGraph->m_backBufferIndex } );
            ThreadData.CondVar.notify_one();
        }
        else
        {
            res = m_Workload( this, m_pFrameGraph->m_backBufferIndex );
        }
        return res;
    }

    Result CFrameGraphNode::Wait( const Platform::ThreadFence& hFence, uint32_t value, uint64_t timeout )
    {
        Result ret = VKE_OK;
        bool res = Platform::Thread::Wait( hFence, value, timeout );
        ret = res ? VKE_TIMEOUT : VKE_OK;
        return ret;
    }

    Result CFrameGraphNode::WaitForFrame( const Platform::ThreadFence& hFence, WAIT_FOR_FRAME frame, uint64_t timeout )
    {
        auto value = GetThreadFence().Load();
        //if(value > 0)
        {
            value += frame;
        }
        return Wait( hFence, value, timeout );
    }

    Result CFrameGraphNode::Wait( const NativeAPI::CPUFence& hFence, uint64_t timeout)
    {
        Result ret = VKE_OK;
        bool res = m_pFrameGraph->m_Desc.pDevice->IsReadyToUse( hFence );
        if(!res)
        {
            ret = VKE_ENOTREADY;
            if( timeout == UINT64_MAX )
            {
                ret = m_pFrameGraph->_GetContext( this )->Wait( hFence );
            }
        }
        return ret;
    }

    CFrameGraphNode* CFrameGraphNode::AddNext( CFrameGraphNode* pNext)
    {
        m_vpNextNodes.PushBack( pNext );
        // For every execute node, try to figure out the last one
        // Last executed command buffer's fence should be frame's fence
        if(pNext->m_doExecute)
        {
            for( uint32_t i = 0; i < CFrameGraph::MAX_BACKBUFFER_COUNT; ++i )
            {
                m_pFrameGraph->m_aFrameData[ i ].cpuFenceIndex = pNext->m_Index.cpuFence;
            }
        }
        return pNext;
    }

    void CFrameGraphNode::WaitFor( const SWaitInfo& Info )
    {
        m_vWaitForNodes.PushBack( Info );
        if(Info.WaitOn == WaitOnBits::GPU)
        {
            Info.pNode->_SignalGPUFence();
        }
    }

    NativeAPI::GPUFence& CFrameGraphNode::GetGPUFence( uint32_t backBufferIndex ) const
    {
        return m_pFrameGraph->_GetGPUFence( m_Index.gpuFence, backBufferIndex );
    }
    NativeAPI::CPUFence& CFrameGraphNode::GetCPUFence( uint32_t backBufferIndex ) const
    {
        return m_pFrameGraph->_GetCPUFence( m_Index.cpuFence, backBufferIndex );
    }

    CFrameGraphNode* CFrameGraphNode::AddSubpass( CFrameGraphNode* pNode )
    {
        pNode->m_pParent = this;
        pNode->m_pPrev = this;
        if(!m_vpSubpassNodes.IsEmpty())
        {
            pNode->m_pPrev = m_vpSubpassNodes.Back();
        }
        m_vpSubpassNodes.PushBack( pNode );
        
        return this;
    }

    Platform::ThreadFence& CFrameGraphNode::GetThreadFence()
    {
        //return m_pFrameGraph->_GetThreadFence( m_Index.threadFence );
        return m_hFence;
    }

    void CFrameGraphNode::SignalThreadFence( uint32_t value )
    {
        GetThreadFence().Store( value );
    }

    void CFrameGraphNode::IncrementThreadFence()
    {
        auto& Fence = GetThreadFence();
        ++Fence;
        //VKE_LOG( m_Name << " = " << Fence.Load() );
    }

    void CFrameGraphNode::_SignalGPUFence()
    {
        if( m_pExecuteNode != nullptr )
        {
            m_pExecuteNode->m_executeFlags |= ExecuteCommandBufferFlags::SIGNAL_GPU_FENCE;
        }
    }

} // VKE::RenderSystem

namespace VKE::RenderSystem
{
    CFrameGraphExecuteNode* CFrameGraphExecuteNode::AddToExecute(CFrameGraphNode* pNode)
    {
        VKE_ASSERT( pNode->m_pExecuteNode == nullptr );
        m_vpNodesToExecute.PushBackUnique( pNode );
        pNode->m_pExecuteNode = this;
        return this;
    }

    CFrameGraphExecuteNode* CFrameGraphExecuteNode::AddToExecute( CFrameGraphExecuteNode* pNode)
    {
        return static_cast<CFrameGraphExecuteNode*>( this->AddSubpass( pNode ) );
    }

    Result CFrameGraphExecuteNode::_BuildDataToExecute(uint8_t backBufferIndex)
    {
        Result ret = VKE_OK;
        auto& Exe = this->m_pFrameGraph->_GetExecute( static_cast< const CFrameGraphNode* >( this ), backBufferIndex );
        Exe.refCount = 1;
        for( uint32_t n = 0; n < m_vWaitForNodes.GetCount(); ++n )
        {
            const auto& WaitInfo = m_vWaitForNodes[n];
            if( WaitInfo.pNode->IsEnabled() && WaitInfo.WaitOn != WaitOnBits::NONE )
            {
                if( WaitInfo.WaitOn == WaitOnBits::GPU )
                {
                    const auto& hFence = WaitInfo.pNode->GetGPUFence(backBufferIndex);
                    if( hFence != NativeAPI::Null )
                    {
                        Exe.vDDIWaitGPUFences.PushBackUnique( hFence );
                        //WaitInfo.pNode->_SignalGPUFence();
                    }
                }
                if(WaitInfo.WaitOn == WaitOnBits::CPU)
                {
                    const auto& hFence = WaitInfo.pNode->GetCPUFence(backBufferIndex);
                    if(hFence != NativeAPI::Null)
                    {
                        WaitInfo.pNode->GetContext()->Wait( hFence );
                    }
                }
                /*if(WaitInfo.WaitOn == WaitOnBits::THREAD)
                {
                    const auto& hFence = WaitInfo.pNode->GetThreadFence();
                    Platform::ThisThread::Wait( hFence, 1, UINT64_MAX );
                }*/
            }
        }
        for( uint32_t subpassIdx = 0; subpassIdx < m_vpSubpassNodes.GetCount(); ++subpassIdx )
        {
            auto& pNode = m_vpSubpassNodes[ subpassIdx ];
            if( pNode->IsEnabled() )
            {
                auto pCb = pNode->GetCommandBuffer();
                if( pCb.IsValid() && pCb->GetState() == CommandBufferStates::BEGIN )
                {
                    Exe.vpCommandBuffers.PushBackUnique( pCb.Get() );
                }
            }
        }
        Exe.hSignalCPUFence = GetCPUFence( backBufferIndex  );
        Exe.hSignalGPUFence = GetGPUFence( backBufferIndex );
        Exe.executeFlags |= m_executeFlags;
        return ret;
    }
} // VKE::RenderSystem