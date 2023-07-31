#include "RenderSystem/CFrameGraph.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CSwapChain.h"
#include "RenderSystem/CGraphicsContext.h"

namespace VKE::RenderSystem
{
    Result CFrameGraph::_Create( const SFrameGraphDesc& Desc)
    {
        Result ret = VKE_OK;
        m_Desc = Desc;
        Memory::CreateObject( &HeapAllocator, &m_pLoadMgr );
        VKE_ASSERT( m_Desc.pDevice != nullptr );
        m_vThreadNames = { "Main" };
        // Add root node
        {

        }
        return ret;
    }

    void CFrameGraph::_Destroy()
    {
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
            // Find context to wait on
            for( uint32_t c = 0; c < ContextTypes::_MAX_COUNT; ++c )
            {
                for( uint32_t i = 0; i < m_pCurrentFrameData->avExecutes[ c ].GetCount(); ++i )
                {
                    if( m_aFrameData[ m_backBufferIndex ].avExecutes[ c ][ i ].hSignalCPUFence
                        == m_ahFrameCPUFences[ m_backBufferIndex ] )
                    {
                        ret = m_aFrameData[ m_backBufferIndex ].avExecutes[ c ][ i ].pContext->Wait(
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
            m_pCurrentFrameData = &m_aFrameData[ m_backBufferIndex ];
            for( uint32_t ctx = 0; ctx < ContextTypes::_MAX_COUNT; ++ctx )
            {
                // auto& vpCbs = m_pCurrentFrameData->avpCommandBuffers[ ctx ];
                auto& vExes = m_pCurrentFrameData->avExecutes[ ctx ];
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
            auto& vExecutes = m_pCurrentFrameData->avExecutes[ ContextTypes::TRANSFER ];
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
        }
        return ret;
    }

    Result CFrameGraph::_ExecuteBatch( CFrameGraphExecuteNode* pNode )
    {
        auto& Exe = _GetExecute( pNode );
        return _ExecuteBatch( &Exe );
    }

    Result CFrameGraph::_BuildDataToExecute( CFrameGraphExecuteNode* pNode)
    {
        return pNode->_BuildDataToExecute();
    }

    void CFrameGraph::_AcquireCommandBuffers()
    {

    }

    Result CFrameGraph::_OnCreateNode( CFrameGraphNode** ppNode )
    {
        Result ret = VKE_OK;
        auto pNode = *ppNode;
        // By default use parent command buffer
        // auto& ParentNode = m_mNodes[ pNode->m_Desc.ParentName ];
        //const auto& Desc = pNode->m_Desc;
        pNode->m_pContext = m_Desc.apContexts[ pNode->m_ctxType ];

        if( VKE_SUCCEEDED( ret ) )
        {
            if( !pNode->m_CommandBufferName.empty() )
            {
                pNode->m_Index.commandBuffer = _CreateCommandBuffer( pNode );
                ret = pNode->m_Index.commandBuffer != INVALID_INDEX ? VKE_OK : VKE_FAIL;
            }
            pNode->m_Index.cpuFence = _CreateCPUFence( pNode );
            pNode->m_Index.gpuFence = _CreateGPUFence( pNode );
            pNode->m_Index.threadFence = _CreateThreadFence( pNode );
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
            pPass->SetWorkload( [ this ]( CFrameGraphNode* const pPass )
            {
                CFrameGraphExecuteNode* pNode = static_cast<CFrameGraphExecuteNode*>( pPass );
                auto ret = pNode->_BuildDataToExecute();
                if( VKE_SUCCEEDED( ret ) )
                {
                    ret = this->_ExecuteBatch( pNode );
                }
                return ret;
            } );
        }
        return pPass;
    }

    CFrameGraphNode* CFrameGraph::CreatePresentPass( const SFrameGraphNodeDesc& Desc )
    {
        auto pRet = CreatePass( Desc );
        if(pRet != nullptr)
        {
            // Present executes as well but via Present api call
            pRet->m_doExecute = true;
            pRet->SetWorkload( [ & ]( CFrameGraphNode* const pPass ) {
                uint32_t frameIdx = UNDEFINED_U32;
                if( !m_qFinishedFrameIndices.empty() )
                {
                    Threads::ScopedLock l( m_FinishedFrameIndicesSyncObj );
                    frameIdx = m_qFinishedFrameIndices.front();
                    m_qFinishedFrameIndices.pop();
                }
                if( frameIdx != UNDEFINED_U32 )
                {
                    VKE_LOG( "present" );

                    auto pCtx = pPass->GetContext()->Reinterpret<CGraphicsContext>();
                    auto pSwpChain = pCtx->GetSwapChain();
                    auto hGPUFence = pPass->m_vWaitForNodes.Back().pNode->GetGPUFence();
                    return pSwpChain->Present( hGPUFence );
                }
                return VKE_OK;
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
        auto idx = m_avCommandBufferNames[ ctxType ].Find( ResourceName{ pNode->m_CommandBufferName.c_str() } );
        if(idx == INVALID_POSITION)
        {
            auto threadIndex = _GetThreadIndex(pNode->m_ThreadName);
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
                idx = (INDEX_TYPE)m_avCommandBufferNames[ pNode->m_ctxType ].PushBack( ResourceName{ pNode->m_CommandBufferName } );
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
    CommandBufferRefPtr CFrameGraph::_GetCommandBuffer(const CFrameGraphNode* const pNode)
    {
        return CommandBufferRefPtr{
            m_pCurrentFrameData->avpCommandBuffers[ pNode->m_ctxType ][ pNode->m_Index.commandBuffer ]
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
            FenceDesc.SetDebugName( pNode->m_Name.c_str() );
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
            FenceDesc.SetDebugName( pNode->m_Name.c_str() );
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

    CFrameGraph::INDEX_TYPE CFrameGraph::_GetThreadIndex( const std::string_view& ThreadName )
    {
        INDEX_TYPE ret = 0;
        auto idx = m_vThreadNames.Find( ResourceName{ ThreadName } );
        if(idx == INVALID_POSITION) // such thread name is not present
        {
            VKE_ASSERT( m_vThreadNames.GetCount() < MAX_GRAPHICS_THREAD_COUNT );
            ret = (INDEX_TYPE)( m_vThreadNames.PushBack( ResourceName{ ThreadName } ) % MAX_GRAPHICS_THREAD_COUNT );
        }
        return ret;
    }

    CFrameGraph::INDEX_TYPE CFrameGraph::_GetThreadIndex( const CFrameGraphNode* const pNode )
    {
        return _GetThreadIndex( pNode->m_ThreadName );
    }

    Result CFrameGraph::SetupPresent( CSwapChain* const pSwapChain )
    {
        Result ret = VKE_OK;
        auto& BackBuffer = pSwapChain->GetCurrentBackBuffer();
        m_pCurrentFrameData->PresentInfo.pSwapChain = pSwapChain;
        m_pCurrentFrameData->PresentInfo.imageIndex = BackBuffer.ddiBackBufferIdx;
        return ret;
    }
    
} // VKE::RenderSystem

namespace VKE::RenderSystem
{
    const FrameGraphWorkload CFrameGraphNode::EmptyWorkload = []( const CFrameGraphNode* const ) { return VKE_OK; };

    Result CFrameGraphNode::_Create( const SFrameGraphPassDesc& Desc )
    {
        Result ret = VKE_OK;
        m_ctxType = Desc.contextType;
        m_Name = Desc.pName;
        m_ThreadName = Desc.pThread;
        m_CommandBufferName = Desc.pCommandBuffer;
        m_ExecuteName = Desc.pExecute;
        m_Workload = EmptyWorkload;
        auto pThis = this;
        ret = m_pFrameGraph->_OnCreateNode( &pThis );
        return ret;
    }
    void CFrameGraphNode::_Destroy()
    {
    }

    void CFrameGraphNode::_Run()
    {
        m_vSyncObjects.Clear();
        m_pCommandBuffer = m_pFrameGraph->_GetCommandBuffer( this );
        if (m_pCommandBuffer->GetState() == CommandBufferStates::EXECUTED)
        {
            m_pCommandBuffer->Reset();
        }
        if(m_pCommandBuffer->GetState() == CommandBufferStates::RESET)
        {
            m_pCommandBuffer->Begin();
        }
        SignalThreadFence( 0 );
        Result res = m_Workload( this );
        SignalThreadFence( 1 );
        ( void )res;
    }

    Result CFrameGraphNode::Wait( const Platform::ThreadFence& hFence, uint32_t value, uint64_t timeout )
    {
        Result ret = VKE_OK;
        bool res = Platform::Thread::Wait( hFence, value, timeout );
        ret = res ? VKE_TIMEOUT : VKE_OK;
        return ret;
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

    Result CFrameGraphNode::Wait(const NativeAPI::GPUFence& hFence)
    {
        m_pFrameGraph->_GetExecute( this ).vDDIWaitGPUFences.PushBackUnique( hFence );
        return VKE_OK;
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
    
    NativeAPI::GPUFence& CFrameGraphNode::GetGPUFence() const
    {
        return m_pFrameGraph->_GetGPUFence( m_Index.gpuFence );
    }

    NativeAPI::CPUFence& CFrameGraphNode::GetCPUFence() const
    {
        return m_pFrameGraph->_GetCPUFence( m_Index.cpuFence );
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

    Platform::ThreadFence& CFrameGraphNode::GetThreadFence() const
    {
        return m_pFrameGraph->_GetThreadFence( m_Index.threadFence );
    }

    void CFrameGraphNode::SignalThreadFence( uint32_t value )
    {
        GetThreadFence().Store( value );
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

    Result CFrameGraphExecuteNode::_BuildDataToExecute()
    {
        Result ret = VKE_OK;
        auto& Exe = this->m_pFrameGraph->_GetExecute( static_cast< const CFrameGraphNode* >( this ) );
        Exe.refCount = 1;
        for( uint32_t n = 0; n < m_vWaitForNodes.GetCount(); ++n )
        {
            const auto& WaitInfo = m_vWaitForNodes[n];
            if( WaitInfo.pNode->IsEnabled() && WaitInfo.WaitOn != WaitOnBits::NONE )
            {
                if( WaitInfo.WaitOn == WaitOnBits::GPU )
                {
                    const auto& hFence = WaitInfo.pNode->GetGPUFence();
                    if( hFence != NativeAPI::Null )
                    {
                        Exe.vDDIWaitGPUFences.PushBackUnique( hFence );
                        //WaitInfo.pNode->_SignalGPUFence();
                    }
                }
                if(WaitInfo.WaitOn == WaitOnBits::CPU)
                {
                    const auto& hFence = WaitInfo.pNode->GetCPUFence();
                    if(hFence != NativeAPI::Null)
                    {
                        WaitInfo.pNode->GetContext()->Wait( hFence );
                    }
                }
                if(WaitInfo.WaitOn == WaitOnBits::THREAD)
                {
                    const auto& hFence = WaitInfo.pNode->GetThreadFence();
                    Platform::ThisThread::Wait( hFence, 1, UINT64_MAX );
                }
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
        Exe.hSignalCPUFence = GetCPUFence();
        Exe.hSignalGPUFence = GetGPUFence();
        Exe.executeFlags |= m_executeFlags;
        return ret;
    }
} // VKE::RenderSystem