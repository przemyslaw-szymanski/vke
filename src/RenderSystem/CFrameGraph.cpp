#include "RenderSystem/CFrameGraph.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CSwapChain.h"
#include "RenderSystem/CGraphicsContext.h"
#include "RenderSystem/Helper.h"

#include "Scene/CScene.h"
#include "CVkEngine.h"
#include "Scene/CWorld.h"

#include "CVkEngine.h"
#include "Core/Managers/CResourceManager.h"

#define VKE_LOG_FRAMEGRAPH 0

namespace VKE::RenderSystem
{
    RENDER_TARGET_RENDER_PASS_OP FrameGraphPassToColorRenderTargetOp(RENDER_PASS_OP op)
    {
        static const RENDER_TARGET_RENDER_PASS_OP scValues[ FrameGraphPassOperations::_MAX_COUNT ] =
        {
            RenderTargetRenderPassOperations::COLOR,
            RenderTargetRenderPassOperations::COLOR_STORE,
            RenderTargetRenderPassOperations::COLOR_CLEAR_STORE,
            RenderTargetRenderPassOperations::COLOR
        };
        return scValues[ op ];
    }

    RENDER_TARGET_RENDER_PASS_OP
        FrameGraphPassToDepthRenderTargetOp(RENDER_PASS_OP op)
    {
        static const RENDER_TARGET_RENDER_PASS_OP scValues[ FrameGraphPassOperations::_MAX_COUNT ] =
        {
            RenderTargetRenderPassOperations::DEPTH_STENCIL,
            RenderTargetRenderPassOperations::DEPTH_STENCIL_STORE,
            RenderTargetRenderPassOperations::DEPTH_STENCIL_CLEAR_STORE,
            RenderTargetRenderPassOperations::DEPTH_STENCIL
        };
        return scValues[ op ];
    }

    TEXTURE_STATE FrameGraphPassOpToColorTextureState(RENDER_PASS_OP op)
    {
        static const TEXTURE_STATE scValues[ FrameGraphPassOperations::_MAX_COUNT ] =
        {
            TextureStates::SHADER_READ,
            TextureStates::COLOR_RENDER_TARGET,
            TextureStates::COLOR_RENDER_TARGET,
            TextureStates::COLOR_RENDER_TARGET
        };
        return scValues[ op ];
    }

    TEXTURE_STATE FrameGraphPassOpToDepthTextureState(RENDER_PASS_OP op)
    {
        static const TEXTURE_STATE scValues[ FrameGraphPassOperations::_MAX_COUNT ] =
        {
            TextureStates::SHADER_READ,
            TextureStates::DEPTH_RENDER_TARGET,
            TextureStates::DEPTH_RENDER_TARGET,
            TextureStates::DEPTH_RENDER_TARGET
        };
        return scValues[ op ];
    }

    Result CFrameGraph::_Create( const SFrameGraphDesc& Desc)
    {
        Result ret = VKE_FAIL;
        m_Desc = Desc;
        //VKE_ASSERT( Desc.Size != TextureSize{ 0, 0 } );
        if( Desc.Size != TextureSize{ 0, 0 } )
        {
            if( VKE_SUCCEEDED(Memory::CreateObject( &HeapAllocator, &m_pLoadMgr ) ) )
            {
                ret = _CreateSwapChainData();
                if( VKE_FAILED( ret ) )
                {
                    return ret;
                }
                VKE_ASSERT( m_Desc.pDevice != nullptr );
                if( ( Desc.flags & FrameGraphFlagBits::BASIC_MULTITHREADED ) != 0 )
                {
                    ret = _CreateDefaultFrameGraph( Desc );
                }
            }
        }
        else
        {
            VKE_LOG_ERR( "FrameGraph Size must be set" );
        }
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
        m_backBufferIndex = _CalcNextBackBufferIndex();
        //VKE_LOG( "bbidx: " << (uint32_t)m_backBufferIndex );
        // Get first free frame
        bool needWait = true;
        for( uint32_t i = 0; i < GetBackBufferCount(); ++i )
        {
            if( m_ahFrameCPUFences[ m_backBufferIndex ] == NativeAPI::Null
                || m_Desc.pDevice->IsReadyToUse( m_ahFrameCPUFences[ m_backBufferIndex ] ) )
            {
                needWait = false;
                break;
            }
            else
            {
                m_backBufferIndex = _CalcNextBackBufferIndex();
            }
        }
        if( needWait )
        {
            // If no frame is executed, wait for next back buffer
            //m_backBufferIndex = 0;
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
        //auto& API = pDevice->NativeAPI();
        //bool signaled = API.IsSignaled( pBatch->hSignalCPUFence );
        //bool executed = pBatch->executionResult == Results::OK;
        //bool hasCmdBuffers = !pBatch->vpCommandBuffers.IsEmpty();
        pBatch->executionResult = Results::NOT_READY;
        pBatch->executeFlags = 0;
        pBatch->vDependencies.Clear();
        pBatch->refCount = 0;
        
        for(uint32_t c = 0; c < pBatch->vpCommandBuffers.GetCount(); ++c)
        {
            auto pCb = pBatch->vpCommandBuffers[ c ];
            auto pDevice = pCb->GetContext()->GetDeviceContext();
            VKE_ASSERT( pDevice->IsReadyToUse( pBatch->hSignalCPUFence ) );
            while( !pDevice->IsReadyToUse( pBatch->hSignalCPUFence ) )
            {
                Platform::ThisThread::Pause();
            }
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
        //VKE_LOG( "Execute batch: " << pNode->m_Name.GetData() );
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
            //pNode->m_Index.gpuFence = _CreateGPUFence( pNode );
            pNode->m_Index.threadFence = _CreateThreadFence( pNode );
            pNode->m_Index.thread = _CreateThreadIndex( Desc.pThread );
            pNode->_CreateBeginRenderPassInfo( Desc );

            if( !pNode->m_CommandBufferName.IsEmpty() )
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
        VKE_ASSERT2( Desc.executeIndex != INVALID_POSITION, "executeIndex must be set" );
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
            VKE_ASSERT2( Desc.pFenceName != nullptr, "GPU fence must be set" );
            VKE_ASSERT2( Desc.executeIndex > 0, "executeIndex must be > 0" );
            pPass->m_GPUFenceName = Desc.pFenceName;
            pPass->m_gpuFenceValue = Desc.executeIndex;
            pPass->m_doExecute = true;
            pPass->m_pExecuteNode        = pPass;
            pPass->m_Index.execute = _CreateExecute( static_cast<CFrameGraphNode*>( pPass ) );
            pPass->m_executeIndex  = Desc.executeIndex;
            pPass->m_signalEndFrameFence = Desc.signalEndFrameFence;
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
                        std::stringstream ss, ss2;
                        for (uint32_t i = 0; i < Exe.vSignalFences.GetCount(); ++i)
                        {
                            ss << ", " << Exe.vSignalFences[ i ].hNative << " = " << Exe.vSignalFences[ i ].value;
                        }
                        for( uint32_t i = 0; i < Exe.vNativeAPIWaitGPUFences.GetCount(); ++i )
                        {
                            ss2 << ", " << Exe.vNativeAPIWaitGPUFences[ i ] << " = "
                               << Exe.vWaitFenceValues[i];
                        }
                        VKE_LOG(
                            pPass->m_Name << ", bbidx: " << ( uint32_t )this->m_backBufferIndex
                                          << " cpuFence: " << pPass->GetThreadFence().Load()
                                          << " signal gpufences: " << ss.str() << " wait gpuFence: " << ss2.str() );
#endif
                        //VKE_LOG_NO_SYNC( "Execute " << pNode->m_Name.GetData() );
                        ret = this->_ExecuteBatch( pNode, backBufferIndex );
                        //auto hGpuFence = this->_GetExecute( pNode, backBufferIndex ).hSignalGPUFence;
                        //Platform::Debug::PrintOutput( "exe %s, %d\n", pNode->m_Name.GetData(), hGpuFence );
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
        
        auto& CPUFps = m_CounterMgr.aCounters[ FrameGraphCounterTypes::CPU_FPS ];
        CPUFps.Total.u32++;
        if( elapsedCPUTime2 >= 1000 )
        {
            CPUFrameTime.Set( elapsedCPUTime );
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
            pRet->SetWorkload(
                [ & ]( CFrameGraphNode* const pPass, uint8_t backBufferIndex )
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
                            auto pCtx      = pPass->GetContext()->Reinterpret<CGraphicsContext>();
                            auto pSwpChain = pCtx->GetSwapChain();
                            auto hGPUFence = NativeAPI::Null; // pPass->m_vWaitForNodes.Back().pNode->GetGPUFence(
                                                              // (uint8_t)backBufferIdx );
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
        m_pScene = m_Desc.pDevice->GetRenderSystem()->GetEngine()->GetWorld()->GetScene().Get();
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
        if( pNode->IsEnabled() )
        {
            pNode->_Run( m_pLastNode );
            if( !pNode->IsSubpass() )
            {
                m_pLastNode = pNode;
            }
        }
        if( pNode->m_pNextNode )
        {
            _ExecuteNode( pNode->m_pNextNode );
        }
    }

    void CFrameGraph::_ExecuteSubpassNodes(CFrameGraphNode* pNode)
    {
        auto pCurrSubpass = pNode->m_pSubpassNode;
        if( pCurrSubpass )
        {
            _ExecuteNode( pCurrSubpass );
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
        ResourceName CmdBufferName = std::format( "{}_{}_{}", (int)ctxType, pNode->m_CommandBufferName.GetData(), pNode->m_Index.thread ).c_str();
        auto idx = m_avCommandBufferNames[ ctxType ].Find( CmdBufferName );
        if(idx == INVALID_POSITION)
        {
            auto threadIndex = pNode->m_Index.thread;
            VKE_ASSERT( threadIndex != INVALID_INDEX );
            SCreateCommandBufferInfo CreateInfo =
            {
                .count = GetBackBufferCount(),
                .threadIndex = (uint8_t)threadIndex
            };
            Utils::TCDynamicArray<CCommandBuffer*, GetBackBufferCount()> vCbs( CreateInfo.count );
            VKE_ASSERT( m_Desc.apContexts[ ctxType ] != nullptr );
            Result res = m_Desc.apContexts[ ctxType ]->_CreateCommandBuffers( CreateInfo, &vCbs[ 0 ] );
            if( VKE_SUCCEEDED(res) )
            {
                ResourceName DbgName;
                for(uint32_t i = 0; i < vCbs.GetCount(); ++i)
                {
                    DbgName.Format( "%s_backBuffer%d_%s_%s", g_aContextNames[ ctxType ], i,
                                    pNode->m_ExecuteName.GetData(), pNode->m_CommandBufferName.GetData() );
                    vCbs[ i ]->SetDebugName( DbgName.GetData() );
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
            VKE_ASSERT( !pNode->m_GPUFenceName.IsEmpty() );
            Result res = _CreateGPUFence( pNode->m_GPUFenceName );
            //auto threadIndex = _GetThreadIndex( Desc.ThreadName );
            const auto maxBBCount = GetBackBufferCount();
            for( uint8_t backBufferIdx = 0; backBufferIdx < maxBBCount; ++backBufferIdx )
            {
                auto& FrameData = m_aFrameData[ backBufferIdx ];
                SExecuteBatch Batch;
                VKE_ASSERT( m_Desc.apContexts[ ctxType ] != nullptr );
                /*res = m_Desc.apContexts[ ctxType ]->_CreateExecuteBatch(
                    backBufferIdx, m_avExecuteNames[ ctxType ].GetCount(), &Batch );*/
                Batch.pContext = m_Desc.apContexts[ ctxType ];
                Batch.hSignalCPUFence = FrameData.vCPUFences[ pNode->m_Index.cpuFence ];
                //Batch.hSignalGPUFence = FrameData.vGPUFences[ pNode->m_Index.gpuFence ];
                //Batch.hSignalGPUFence = pNode->GetGPUFence( backBufferIdx );
                //Batch.signalFenceValue = pNode->GetGPUFenceValue();
                Batch.vSignalFences = { pNode->GetGPUFence( backBufferIdx ) };
                Batch.SetDebugName( pNode->m_Name.GetData() );
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
        for( uint32_t backBufferIdex = 0; backBufferIdex < GetBackBufferCount(); ++backBufferIdex )
        {
            SFenceDesc FenceDesc = { .isSignaled = true };
            FenceDesc.SetDebugName( std::format( "{}_{}", pNode->m_Name.GetData(), backBufferIdex ).c_str() );
            auto hFence = m_Desc.pDevice->CreateCPUFence( FenceDesc );
            VKE_ASSERT( hFence != NativeAPI::Null );
            ret = (INDEX_TYPE)m_aFrameData[ backBufferIdex ].vCPUFences.PushBack( hFence );
        }
        return ret;
    }

    Result CFrameGraph::_CreateGPUFence( const ShortName& Name )
    {
        Result ret = VKE_OK;
        if (m_aFrameData[0].mGPUFences.find( Name ) != m_aFrameData[0].mGPUFences.end())
        {
            return ret;
        }
        for( uint32_t backBufferIdex = 0; backBufferIdex < GetBackBufferCount(); ++backBufferIdex )
        {
            SGPUFenceDesc FenceDesc;
            FenceDesc.SetDebugName( std::format( "{}_{}", Name.GetData(), backBufferIdex ).c_str() );
            auto hFence = m_Desc.pDevice->CreateGPUFence( FenceDesc );
            VKE_ASSERT( hFence != NativeAPI::Null );
            if (hFence != NativeAPI::Null)
            {
                auto h  = ( Name.GetHash() );
                ( void )h;
                m_aFrameData[ backBufferIdex ].mGPUFences[ Name ] = hFence;
            }
            else
            {
                ret = VKE_FAIL;
            }
        }
        return ret;
    }

    CFrameGraph::INDEX_TYPE CFrameGraph::_CreateThreadFence( const CFrameGraphNode* const )
    {
        INDEX_TYPE ret = INVALID_INDEX;
        for( uint32_t backBufferIdex = 0; backBufferIdex < GetBackBufferCount(); ++backBufferIdex )
        {
            ret = (INDEX_TYPE)m_aFrameData[ backBufferIdex ].vThreadFences.PushBack( {0} );
        }
        return ret;
    }

    CFrameGraph::SThreadData& CFrameGraph::_GetThreadData( uint32_t threadindex ) const
    {
        return *(m_vpThreadData[ threadindex ]);
    }

    void CFrameGraph::_ThreadFunc(const CFrameGraph* pFrameGraph, uint32_t index)
    {
        using namespace std::chrono_literals;
        CFrameGraph::SThreadData& ThreadData = pFrameGraph->_GetThreadData( index );
        while( !ThreadData.needExit )
        {
            std::unique_lock<std::mutex> l( ThreadData.Mutex );
            /*if( ThreadData.CondVar.wait_for( l, 2s,
                [&] { return !ThreadData.qWorkloads.empty(); } ) )*/
            ThreadData.CondVar.wait( l, [ & ] { return !ThreadData.qWorkloads.empty(); } );
            {
                if( !ThreadData.qWorkloads.empty() )
                {
                    auto Workload = ThreadData.qWorkloads.front();
                    ThreadData.qWorkloads.pop_front();
                    if( Workload.Func )
                    {
                        Workload.Func( Workload.pNode, Workload.backBufferIndex );
                    }
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

    Result CFrameGraph::_CreateSwapChainData()
    {
        Result ret = VKE_OK;
        for( uint32_t bbIdx = 0; bbIdx < GetBackBufferCount(); ++bbIdx )
        {
            SGPUFenceDesc Desc;
            Desc.isBinaryFence = true;
            Desc.SetDebugName( std::format( "SwapChain_{}", bbIdx ).data() );
            m_aFrameData[ bbIdx ].hSwapChainFence = m_Desc.pDevice->CreateGPUFence( Desc );
            if( m_aFrameData[bbIdx].hSwapChainFence == NativeAPI::Null ) 
            {
                ret = VKE_FAIL;
            }
        }
        return ret;
    }

    TextureRefPtr CFrameGraph::_GetTexture( const SFrameGraphRenderTargetTextureDesc& Desc )
    {
        TextureRefPtr pRet;
        auto Itr = m_mRenderTargets.find( Desc.pName );
        if( Itr == m_mRenderTargets.end() )
        {
            TEXTURE_USAGE usage = Helper::HasDepth( Desc.format )
                                      ? TextureUsages::DEPTH_STENCIL_RENDER_TARGET
                                      : TextureUsages::COLOR_RENDER_TARGET;
            SCreateTextureDesc TexDesc;
            TexDesc.Texture =
            {
                .Size = TextureSize( _GetRenderArea(Desc.size).Size ),
                .format = Desc.format,
                .usage = usage,
                .memoryUsage = MemoryUsages::GPU_ACCESS | MemoryUsages::TEXTURE,
                .Name = Desc.pName
            };
            auto hTex = m_Desc.pDevice->CreateTexture( TexDesc );
            pRet = m_Desc.pDevice->GetTexture( hTex );
        }
        else
        {
            pRet = Itr->second;
        }

        return pRet;
    }

    Rect2DI32 CFrameGraph::_GetRenderArea( RENDER_PASS_SIZE size )
    {
        Rect2DI32 Ret =
        {
            .Position = {0,0},
            .Size = ExtentU32( m_Desc.Size / TextureSize{ size, size } )
        };
        return Ret;
    }

    Result CFrameGraph::SetupPresent( CSwapChain* const pSwapChain, uint8_t backBufferIdx )
    {
        Result ret = VKE_OK;
        
        m_aFrameData[ backBufferIdx ].PresentInfo.pSwapChain = pSwapChain;
        m_aFrameData[ backBufferIdx ].PresentInfo.imageIndex = pSwapChain->GetNativeBackBufferIndex();
        return ret;
    }
    

    CFrameGraphNode* CFrameGraph::_SetNextNode( CFrameGraphNode** ppCurrNode, CFrameGraphNode* pNext )
    {
        auto pCurrNode = *ppCurrNode;
        pCurrNode->m_pNextNode = pNext;
        // For every execute node, try to figure out the last one
        // Last executed command buffer's fence should be frame's fence
        if( pNext->m_doExecute )
        {
            for( uint32_t i = 0; i < GetBackBufferCount(); ++i )
            {
                m_aFrameData[ i ].cpuFenceIndex = pNext->m_Index.cpuFence;
            }
        }
        if( pCurrNode->HasCommandBuffer() )
        {
            // Use the same command buffer if both passes use the same
            // command buffer name, context and thread
            if( pCurrNode->m_CommandBufferName == pNext->m_CommandBufferName &&
                pCurrNode->m_ThreadName == pNext->m_ThreadName &&
                pCurrNode->m_ctxType == pNext->m_ctxType )
            {
                pNext->m_Index.commandBuffer = pCurrNode->m_Index.commandBuffer;
            }
        }
        m_vpNextNodes.PushBackUnique( pNext );
        return pNext;
    }

    void CFrameGraph::_IsNodeEnabled( CFrameGraphNode** ppCurrNode, bool isEnabled )
    {
        //auto pCurrNode = *ppCurrNode;
    }
} // VKE::RenderSystem

namespace VKE::RenderSystem
{
    const FrameGraphWorkload CFrameGraphNode::EmptyWorkload =
        []( CFrameGraphNode* pPass, uint8_t backBufferIndex )
        {
            Result ret = pPass->OnWorkloadBegin( backBufferIndex );
            return pPass->OnWorkloadEnd( ret );
        };

    Result CFrameGraphNode::_Create( const SFrameGraphPassDesc& Desc )
    {
        Result ret = VKE_OK;
        m_ctxType = Desc.contextType;
        m_Name = Desc.pName;
        m_ThreadName = Desc.pThread;
        m_CommandBufferName = Desc.pCommandBufferName;
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

    Result CFrameGraphNode::OnWorkloadBegin(uint8_t backBufferIndex)
    {
        /// TODO: WaitForThreads can cause huge CPU overhead
        Result ret = _WaitForThreads();
        if( VKE_SUCCEEDED( ret ) )
        {
            if( HasCommandBuffer() )
            {
                m_pCommandBuffer = GetCommandBuffer( backBufferIndex );
                auto pDevice = GetContext()->GetDeviceContext();
                for( auto& Pair : m_mTaskResults )
                {
                    if( pDevice->IsReadyToUse( Pair.first ) )
                    {

                    }
                }
                for( uint32_t i = 0; i < m_vpColorRenderTargets.GetCount(); ++i )
                {
                    auto pTex = m_vpColorRenderTargets[ i ];
                    // TEXTURE_STATE state = FrameGraphPassOpToColorTextureState();
                    m_pCommandBuffer->SetState( m_BeginRenderPassInfo.vColorRenderTargetInfos[ i ].state, &pTex );
                }
                if( m_pDepthStencilRenderTarget.IsValid() )
                {
                    m_pCommandBuffer->SetState( m_BeginRenderPassInfo.DepthRenderTargetInfo.state,
                                                &m_pDepthStencilRenderTarget );
                }
                if( HasRenderPass() )
                {
                    m_pCommandBuffer->BeginRenderPass( m_BeginRenderPassInfo );
                }
            }
        }
        m_vWaitCPUFences.Clear();
        m_vWaitGPUFences.Clear();
        m_finished = false;
        return ret;
    }

    CommandBufferPtr CFrameGraphNode::GetCommandBuffer(uint8_t backBufferIndex) const
    {
        CommandBufferPtr pCommandBuffer;
        if( HasCommandBuffer() )
        {
            pCommandBuffer = m_pFrameGraph->_GetCommandBuffer( this, backBufferIndex );
            if( pCommandBuffer->GetState() == CommandBufferStates::EXECUTED )
            {
                pCommandBuffer->Reset();
            }
            if( pCommandBuffer->GetState() == CommandBufferStates::RESET )
            {
                pCommandBuffer->Begin();
            }
            pCommandBuffer->SetBackBufferIndex( backBufferIndex );
            //const auto& Fence = GetGPUFence( backBufferIndex );
            //pCommandBuffer->SetSubmitFence( Fence );
        }
        return pCommandBuffer;
    }

    Result CFrameGraphNode::OnWorkloadEnd(Result workloadResult)
    {
        if( VKE_SUCCEEDED( workloadResult ) )
        {
            m_pFrameGraph->_ExecuteSubpassNodes( this );

            if( HasRenderPass() )
            {
                m_pCommandBuffer->EndRenderPass();
            }
        }
        IncrementThreadFence();
        m_finished = true;
        m_CondVar.notify_all();
        return workloadResult;
    }

    Result CFrameGraphNode::_Run(CFrameGraphNode* pLastNode)
    {
        Result res = VKE_OK;
        if( pLastNode && !pLastNode->IsSubpass() )
        {
            this->m_pPrevNode = pLastNode;
        }
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

    Result CFrameGraphNode::_WaitForThreads()
    {
        Result ret = VKE_OK;
        // auto threadFenceValue = GetThreadFence().Load();
        for( uint32_t i = 0; i < m_vWaitForNodes.GetCount(); ++i )
        {
            auto& WaitInfo = m_vWaitForNodes[ i ];
            if( WaitInfo.WaitOn == WaitOnBits::THREAD )
            {
                CFrameGraphNode* pNode = WaitInfo.pNode;
                uint64_t timeout = 2 * 1000 * 1000; // 2 seconds
                ret = WaitForFrame( pNode->GetThreadFence(), WaitInfo.frame, timeout );
                if( !VKE_SUCCEEDED( ret ) )
                {
#if !defined( VKE_RENDER_SYSTEM_DEBUG )
                    VKE_LOG_ERR( "A node: '" << this->m_Name << "' reached timeout waiting for node: '"
                                             << m_vWaitForNodes[ i ].pNode->m_Name << "'"
                                             << " with value: " << WaitInfo.pNode->GetThreadFence().Load() );
#endif // VKE_RENDER_SYSTEM_DEBUG
                    break;
                }
            }
        }
        return ret;
    }

    Result CFrameGraphNode::Wait( const Platform::ThreadFence& hFence, uint32_t value, uint64_t timeout )
    {
        Result ret = VKE_OK;
        //VKE_LOG_NO_SYNC( "Fence value: " << hFence.Load() );
        bool res = Platform::Thread::Wait( hFence, value, timeout );
        ret = res ? VKE_TIMEOUT : VKE_OK;
       // VKE_LOG_NO_SYNC( "Wait for thread fence: " << value << " ret: " << ret );
        return ret;
    }

    Result CFrameGraphNode::WaitForFrame( const Platform::ThreadFence& hFence, WAIT_FOR_FRAME frame, uint64_t timeout )
    {
        auto value = m_pFrameGraph->GetFrameIndex() + frame;
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

    CFrameGraphNode* CFrameGraphNode::SetNext( CFrameGraphNode* pNext)
    {
        CFrameGraphNode* pThis = this;
        return m_pFrameGraph->_SetNextNode( &pThis, pNext );
    }

    void CFrameGraphNode::WaitFor( const SWaitInfo& Info )
    {
        m_vWaitForNodes.PushBack( Info );
        if(Info.WaitOn == WaitOnBits::GPU)
        {
            Info.pNode->_SignalGPUFence();
        }
    }

    const TexturePtr CFrameGraphNode::GetColorRenderTarget(uint32_t index) const
    {
        TexturePtr pRet;
        if( m_vpColorRenderTargets.GetCount() > index )
        {
            pRet = m_vpColorRenderTargets[ index ];
        }
        return pRet;
    }

    FORMAT CFrameGraphNode::GetDepthRenderTargetFormat() const
    {
        FORMAT ret = Formats::UNDEFINED;
        if (m_pDepthStencilRenderTarget.IsValid())
        {
            ret = m_pDepthStencilRenderTarget->GetDesc().format;
        }
        return ret;
    }
    
    /*NativeAPI::Fence& CFrameGraphNode::GetGPUFence( uint32_t backBufferIndex ) const
    {
        return m_pFrameGraph->_GetGPUFence( m_Index.gpuFence, backBufferIndex );
    }*/
    NativeAPI::CPUFence& CFrameGraphNode::GetCPUFence( uint32_t backBufferIndex ) const
    {
        return m_pFrameGraph->_GetCPUFence( m_Index.cpuFence, backBufferIndex );
    }

    CFrameGraphNode* CFrameGraphNode::AddSubpass( CFrameGraphNode* pNode, uint32_t index )
    {
        pNode->m_pParent = this;
        pNode->m_isSubpass = true;
        auto ppCurr = &m_pSubpassNode;

        for (uint32_t i = 0; i < index; ++i)
        {
            if (*ppCurr)
            {
                ppCurr = &( *ppCurr )->m_pNextNode;
            }
            else
            {
                break;
            }
        }

        pNode->m_pNextNode = *ppCurr;
        *ppCurr = pNode;

        return this;
    }

    CFrameGraphNode* CFrameGraphNode::AddSubpass(cstr_t pName,
        FrameGraphWorkload&& Wl)
    {
        SFrameGraphNodeDesc Desc;
        Desc.pName = pName;
        auto pNode = m_pFrameGraph->CreatePass( Desc );
        auto pRet = this;
        if (pNode)
        {
            pNode->SetWorkload( std::forward< FrameGraphWorkload >( Wl ) );
            pRet = AddSubpass( pNode );
        }
        return pRet;
    }

    Platform::ThreadFence& CFrameGraphNode::GetThreadFence()
    {
        //return m_pFrameGraph->_GetThreadFence( m_Index.threadFence );
        return m_hFence;
    }

    void CFrameGraphNode::SignalThreadFence( uint32_t value )
    {
        //std::unique_lock l( m_CondVarMtx );
        GetThreadFence().Store( value );
    }

    void CFrameGraphNode::IncrementThreadFence()
    {
        //std::unique_lock l( m_CondVarMtx );
        auto& Fence = GetThreadFence();
        ++Fence;
        //Platform::Debug::PrintOutput( "%s Signal fence: %d\n", m_Name.GetData(), Fence.Load() );
        //VKE_LOG_NO_SYNC( m_Name << " = " << Fence.Load() );
    }

    void CFrameGraphNode::_SignalGPUFence()
    {
        if( m_pExecuteNode != nullptr )
        {
            m_pExecuteNode->m_executeFlags |= ExecuteCommandBufferFlags::SIGNAL_GPU_FENCE;
        }
    }

    void CFrameGraphNode::AddTask( TaskFunc&& Func, CFrameGraphNode::STaskResult* pResult )
    {
        static STaskResult DummyTaskResult;
        Threads::ScopedLock l( m_TaskSyncObj );
        // Do not use nullptr in order to avoid redundant if-checks
        pResult = pResult ? pResult : &DummyTaskResult;
        m_qTasks.push_back( { pResult, std::move( Func ) } );
    }

    void CFrameGraphNode::_ExecuteTasks( const SExecuteTaskDesc& Desc )
    {
        uint32_t taskExecutedCount = 0;
        for( auto Itr = m_qTasks.begin(); Itr != m_qTasks.end(); ++Itr )
        {
            bool taskExecuted = Itr->Func( this, Desc.backBufferIndex );
            taskExecutedCount += taskExecuted;
            bool removeTask = taskExecuted || Desc.forceRemove;
            Itr->pResult->executedOnCPU = true;
            if( removeTask )
            {
                if( HasCommandBuffer() )
                {
                    const auto& hFence = GetCPUFence( Desc.backBufferIndex );
                    m_mTaskResults[ hFence ].PushBack( Itr->pResult );
                }
                Threads::ScopedLock l( m_TaskSyncObj );
                m_qTasks.erase( Itr );
            }
            if( Desc.executeTaskCount >= taskExecutedCount )
            {
                break;
            }
        }
    }

        void CFrameGraphNode::_CreateBeginRenderPassInfo( const SFrameGraphNodeDesc& Desc )
    {
        uint32_t writeCount = 0;
        for( uint32_t i = 0; i < Desc.vRenderTargets.GetCount(); ++i )
        {
            const SFrameGraphRenderTargetTextureDesc& RTDesc = Desc.vRenderTargets[ i ];
            TexturePtr pTexture = m_pFrameGraph->_GetTexture( RTDesc );
            if( pTexture.IsValid() )
            {
                writeCount += ( RTDesc.operation == FrameGraphPassOperations::WRITE
                                || RTDesc.operation == FrameGraphPassOperations::OVERWRITE
                                || RTDesc.operation == FrameGraphPassOperations::READ_WRITE );
                TextureViewPtr pView = pTexture->GetView();
                if( pTexture->IsColor() )
                {
                    SRenderTargetInfo Info =
                    {
                        .hNativeAPIView = pView->GetNativeAPIObject(),
                        .format = pView->GetDesc().format,
                        .ClearColor = SClearValue( 0, 0, 0, 0 ),
                        .state = FrameGraphPassOpToColorTextureState( RTDesc.operation ),
                        .renderPassOp = FrameGraphPassToColorRenderTargetOp( RTDesc.operation )
                    };
                    m_BeginRenderPassInfo.vColorRenderTargetInfos.PushBack( Info );
                    m_vpColorRenderTargets.PushBack( pTexture );
                    m_vColorRenderTargetFormats.PushBack( RTDesc.format );
                }
                else
                {
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.hNativeAPIView = pView->GetNativeAPIObject();
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.ClearColor = SClearValue( 1, 0 );
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.format = pView->GetDesc().format;
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.renderPassOp
                        = FrameGraphPassToDepthRenderTargetOp( RTDesc.operation );
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.state
                        = FrameGraphPassOpToDepthTextureState( RTDesc.operation );
                    m_pDepthStencilRenderTarget = pTexture;
                }
            }
        }
        m_BeginRenderPassInfo.SetDebugName( m_Name.GetData() );
        m_BeginRenderPassInfo.RenderArea = m_pFrameGraph->_GetRenderArea( Desc.size );
        m_hasRenderPass = writeCount > 0;
    }

    void CFrameGraphNode::_BeginRenderPass()
    {
        if (HasCommandBuffer())
        {

        }
    }

    Scene::ScenePtr CFrameGraphNode::GetScene()
    {
        return m_pFrameGraph->GetScene();
    }

    const SFence& CFrameGraphNode::GetGPUFence( uint8_t backBufferIndex ) const
    {
        if( m_pExecuteNode != nullptr )
        {
            return m_pExecuteNode->_GetGPUFence( backBufferIndex );
        }
        // Usually returns null handle which should be handled externally
        return m_aCustomFences[ backBufferIndex ];

    }
} // VKE::RenderSystem

namespace VKE::RenderSystem
{
    CFrameGraphExecuteNode* CFrameGraphExecuteNode::AddToExecute(CFrameGraphNode* pNode)
    {
        VKE_ASSERT( pNode->m_pExecuteNode == nullptr );
        m_vpNodesToExecute.PushBackUnique( pNode );
        pNode->m_pExecuteNode = this;
        auto bbCount          = m_pFrameGraph->GetBackBufferCount();
        for( uint8_t b = 0; b < bbCount; ++b )
        {
            auto pCb = m_pFrameGraph->_GetCommandBuffer( pNode, b );
            pCb->SetExecuteFence( m_Fence );
        }
        return this;
    }

    CFrameGraphExecuteNode* CFrameGraphExecuteNode::AddToExecute( CFrameGraphExecuteNode* pNode)
    {
        return static_cast<CFrameGraphExecuteNode*>( this->AddSubpass( pNode ) );
    }

    const SFence& CFrameGraphExecuteNode::_GetGPUFence(uint8_t backBufferIndex)
    {
        //if( m_currBackBufferIndex != backBufferIndex )
        {
            //VKE_ASSERT( m_lastBackBufferIndex != backBufferIndex );
            m_lastBackBufferIndex = m_currBackBufferIndex;
            m_currBackBufferIndex = backBufferIndex;
            m_Fence.hNative = m_pFrameGraph->_GetGPUFence( backBufferIndex, m_GPUFenceName );
            m_Fence.value   = m_gpuFenceValue + m_pFrameGraph->GetFrameIndex();
        }
        VKE_ASSERT( m_Fence.hNative != NativeAPI::Null );
        return m_Fence;
    }
    Result CFrameGraphExecuteNode::_BuildDataToExecute(uint8_t backBufferIndex)
    {
        Result ret = VKE_OK;
        auto& Exe = this->m_pFrameGraph->_GetExecute( static_cast< const CFrameGraphNode* >( this ), backBufferIndex );
        Exe.refCount = 1;
        
        Exe.vNativeAPIWaitGPUFences.Clear();
        Exe.vWaitFenceValues.Clear();
        Exe.vSignalFences.Clear();
        uint64_t frameIndex = m_pFrameGraph->GetFrameIndex();
        //std::stringstream ss;
        for( uint32_t n = 0; n < m_vWaitForNodes.GetCount(); ++n )
        {
            const auto& WaitInfo = m_vWaitForNodes[n];
            if( WaitInfo.pNode->IsEnabled() && WaitInfo.WaitOn != WaitOnBits::NONE )
            {
                if( WaitInfo.WaitOn == WaitOnBits::GPU )
                {
                    const auto& Fence = WaitInfo.pNode->GetGPUFence(backBufferIndex);
                    if( Fence.hNative != NativeAPI::Null )
                    {
                        Exe.vNativeAPIWaitGPUFences.PushBack( Fence.hNative );
                        Exe.vWaitFenceValues.PushBack( Fence.value + 0*frameIndex );
                        //ss << Fence.hNative << "=" << Fence.value << ", ";
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
        for( uint32_t nodeIdx = 0; nodeIdx < m_vpNodesToExecute.GetCount(); ++nodeIdx )
        {
            auto& pNode = m_vpNodesToExecute[ nodeIdx ];
            // Even if specific node is disabled it still could wait
            // therefore we have to add such synchronization
            const auto& vGPUDeps = pNode->GetGPUFenceDependencies();
            
            for( uint32_t waitIdx = 0; waitIdx < vGPUDeps.GetCount(); ++waitIdx )
            {
                const auto hFence = vGPUDeps[ waitIdx ].hNative;
                const auto value  = vGPUDeps[ waitIdx ].value;
                Exe.vNativeAPIWaitGPUFences.PushBack( hFence );
                Exe.vWaitFenceValues.PushBack( value );
            }
            
            if( pNode->IsEnabled() )
            {
                auto pCb = pNode->GetCommandBuffer( backBufferIndex );
                if( pCb.IsValid() && pCb->GetState() == CommandBufferStates::BEGIN )
                {
                    //VKE_LOG( "Execute batch: " << this->m_Name.GetData() << ", cb: " << pCb->GetDebugName() );
                    Exe.vpCommandBuffers.PushBackUnique( pCb.Get() );
                }
            }
        }
        
        Utils::TCDynamicArray<NativeAPI::Fence, 2> vFences;
        Utils::TCDynamicArray<NativeAPI::FenceValue, 2> vFenceValues;
        const SFence&                                   SignalFence = GetGPUFence( backBufferIndex );
        Exe.vSignalFences.PushBack( SignalFence );
        //VKE_LOG( this->m_Name.GetData() << ", bbidx: " << (uint32_t)backBufferIndex <<  ", frame: " << frameIndex << ": " << SignalFence.hNative << "=" << SignalFence.value );
        if(m_signalEndFrameFence)
        {
            auto pSwpChain = m_pContext->Reinterpret<CGraphicsContext>()->GetSwapChain();
            Exe.vSignalFences.PushBack( SFence{ pSwpChain->GetFrameEndGPUFence(), 0 } );
        }
        Exe.hSignalCPUFence = GetCPUFence( backBufferIndex  );

        Exe.executeFlags |= m_executeFlags;
        return ret;
    }
} // VKE::RenderSystem
