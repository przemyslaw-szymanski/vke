#include "RenderSystem/CFrameGraphNode.h"
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
    RENDER_TARGET_RENDER_PASS_OP FrameGraphPassToColorRenderTargetOp( RENDER_PASS_OP op )
    {
        static const RENDER_TARGET_RENDER_PASS_OP scValues[ FrameGraphPassOperations::_MAX_COUNT ] = {
            RenderTargetRenderPassOperations::COLOR,
            RenderTargetRenderPassOperations::COLOR_STORE,
            RenderTargetRenderPassOperations::COLOR_CLEAR_STORE,
            RenderTargetRenderPassOperations::COLOR
        };
        return scValues[ op ];
    }

    RENDER_TARGET_RENDER_PASS_OP
    FrameGraphPassToDepthRenderTargetOp( RENDER_PASS_OP op )
    {
        static const RENDER_TARGET_RENDER_PASS_OP scValues[ FrameGraphPassOperations::_MAX_COUNT ] = {
            RenderTargetRenderPassOperations::DEPTH_STENCIL,
            RenderTargetRenderPassOperations::DEPTH_STENCIL_STORE,
            RenderTargetRenderPassOperations::DEPTH_STENCIL_CLEAR_STORE,
            RenderTargetRenderPassOperations::DEPTH_STENCIL
        };
        return scValues[ op ];
    }

    TEXTURE_STATE FrameGraphPassOpToColorTextureState( RENDER_PASS_OP op )
    {
        static const TEXTURE_STATE scValues[ FrameGraphPassOperations::_MAX_COUNT ] = {
            TextureStates::SHADER_READ,
            TextureStates::COLOR_RENDER_TARGET,
            TextureStates::COLOR_RENDER_TARGET,
            TextureStates::COLOR_RENDER_TARGET
        };
        return scValues[ op ];
    }

    TEXTURE_STATE FrameGraphPassOpToDepthTextureState( RENDER_PASS_OP op )
    {
        static const TEXTURE_STATE scValues[ FrameGraphPassOperations::_MAX_COUNT ] = {
            TextureStates::SHADER_READ,
            TextureStates::DEPTH_RENDER_TARGET,
            TextureStates::DEPTH_RENDER_TARGET,
            TextureStates::DEPTH_RENDER_TARGET
        };
        return scValues[ op ];
    }

    RENDER_TARGET_RENDER_PASS_OP FrameGraphPassOpToColorRenderTargetOp( RENDER_PASS_OP op )
    {
        static const RENDER_TARGET_RENDER_PASS_OP scValues[ RenderTargetRenderPassOperations::USAGE::_MAX_COUNT ] = {
            RenderTargetRenderPassOperations::COLOR,             // shader read
            RenderTargetRenderPassOperations::COLOR_STORE,       // render pass write
            RenderTargetRenderPassOperations::COLOR_CLEAR_STORE, // pass overwrite
            RenderTargetRenderPassOperations::COLOR,             // shader rw
            RenderTargetRenderPassOperations::COLOR              // pass read
        };
        return scValues[ op ];
    }

    RENDER_TARGET_RENDER_PASS_OP FrameGraphPassOpToDepthRenderTargetOp( RENDER_PASS_OP op )
    {
        static const RENDER_TARGET_RENDER_PASS_OP scValues[ RenderTargetRenderPassOperations::USAGE::_MAX_COUNT ] = {
            RenderTargetRenderPassOperations::DEPTH_STENCIL,             // shader read
            RenderTargetRenderPassOperations::DEPTH_STENCIL_STORE,       // render pass write
            RenderTargetRenderPassOperations::DEPTH_STENCIL_CLEAR_STORE, // pass overwrite
            RenderTargetRenderPassOperations::DEPTH_STENCIL,             // shader rw
            RenderTargetRenderPassOperations::DEPTH_STENCIL              // pass read
        };
        return scValues[ op ];
    }

    const FrameGraphWorkload CFrameGraphNode::EmptyWorkload = []( CFrameGraphNode* pPass, uint8_t backBufferIndex ) {
        Result ret = pPass->OnWorkloadBegin( backBufferIndex );
        return pPass->OnWorkloadEnd( ret );
    };

    Result CFrameGraphNode::_Create( const SFrameGraphPassDesc& Desc )
    {
        Result ret          = VKE_OK;
        m_ctxType           = Desc.contextType;
        m_Name              = Desc.pName;
        m_ThreadName        = Desc.pThread;
        m_CommandBufferName = Desc.pCommandBuffer;
        m_ExecuteName       = Desc.pExecute;
        m_Workload          = EmptyWorkload;
        m_isAsync           = strcmp( Desc.pThread, "Main" ) != 0;
        auto pThis          = this;
        ret                 = m_pFrameGraph->_OnCreateNode( Desc, &pThis );
        return ret;
    }

    void CFrameGraphNode::_Destroy()
    {
    }

    Result CFrameGraphNode::OnWorkloadBegin( uint8_t backBufferIndex )
    {
        /// TODO: WaitForThreads can cause huge CPU overhead
        Result ret = _WaitForThreads();
        if( VKE_SUCCEEDED( ret ) )
        {
            if( HasCommandBuffer() )
            {
                m_pCommandBuffer = GetCommandBuffer( backBufferIndex );
                auto pDevice     = GetContext()->GetDeviceContext();
                for( auto& Pair: m_mTaskResults )
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
                if( m_pDepthStencilRenderTarget!= nullptr )
                {
                    m_pCommandBuffer->SetState( m_BeginRenderPassInfo.DepthRenderTargetInfo.state,
                                                &m_pDepthStencilRenderTarget );
                }
                if( HasRenderPass() )
                {
                    SBeginRenderPassInfo Info = {};
                    Info.hDDIRenderPass      = m_hRHIRenderPass;
                    Info.RenderArea          = GetRenderArea();
                    m_pCommandBuffer->BeginRenderPass( Info );
                }
            }
        }
        VKE_ASSERT( VKE_SUCCEEDED( ret ) );
        m_vSyncObjects.Clear();
        m_finished = false;
        return ret;
    }

    CommandBufferPtr CFrameGraphNode::GetCommandBuffer( uint8_t backBufferIndex ) const
    {
        CommandBufferPtr pCommandBuffer;
        if( HasCommandBuffer() )
        {
            pCommandBuffer = m_pFrameGraph->_GetCommandBuffer( this, backBufferIndex );
            const auto state = pCommandBuffer->GetState();
            if( state != CommandBufferStates::FLUSH )
            {
                if( state == CommandBufferStates::EXECUTED )
                {
                    pCommandBuffer->Reset();
                }
                if( state == CommandBufferStates::RESET )
                {
                    pCommandBuffer->Begin();
                }
                VKE_ASSERT( pCommandBuffer->GetState() == CommandBufferStates::BEGIN );
            }
            pCommandBuffer->SetBackBufferIndex( backBufferIndex );
        }
        return pCommandBuffer;
    }

    Result CFrameGraphNode::OnWorkloadEnd( Result workloadResult )
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

    Result CFrameGraphNode::_Run( CFrameGraphNode* pLastNode )
    {
        Result res = VKE_OK;
        if( pLastNode && !pLastNode->IsSubpass() )
        {
            this->m_pPrevNode = pLastNode;
        }
        if( m_isAsync )
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
            if( WaitInfo.WaitOn == WaitOnBits::CPU_WAITS_FOR_CPU )
            {
                CFrameGraphNode* pNode   = WaitInfo.pNode;
                uint64_t         timeout = 2 * 1000 * 1000; // 2 seconds
                ret                      = WaitForFrame( pNode->GetThreadFence(), WaitInfo.frame, timeout );
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

    uint64_t CFrameGraphNode::GetFenceValue() const
    {
        return (m_pFrameGraph->m_currentFrameIndex+1) * m_fenceValue;
    }

    uint64_t CFrameGraphNode::InitFenceValue( uint8_t backBufferIndex )
    {
        m_fenceValue = m_pFrameGraph->_AdvanceBackBufferFence(backBufferIndex);
        return m_fenceValue;
    }

    Result CFrameGraphNode::Wait( const Platform::ThreadFence& hFence, uint32_t value, uint64_t timeout )
    {
        Result ret = VKE_OK;
        // VKE_LOG_NO_SYNC( "Fence value: " << hFence.Load() );
        bool res = Platform::Thread::Wait( hFence, value, timeout );
        ret      = res ? VKE_TIMEOUT : VKE_OK;
        // VKE_LOG_NO_SYNC( "Wait for thread fence: " << value << " ret: " << ret );
        return ret;
    }

    Result CFrameGraphNode::WaitForFrame( const Platform::ThreadFence& hFence, WAIT_FOR_FRAME frame, uint64_t timeout )
    {
        auto fidx  = m_pFrameGraph->GetFrameIndex();
        auto value = fidx + frame;
        return Wait( hFence, value, timeout );
    }

    Result CFrameGraphNode::Wait( const RHI::CPUFence& hFence, uint64_t timeout )
    {
        Result ret = VKE_OK;
        bool   res = m_pFrameGraph->m_Desc.pDevice->IsReadyToUse( hFence );
        if( !res )
        {
            ret = VKE_ENOTREADY;
            if( timeout == UINT64_MAX )
            {
                ret = m_pFrameGraph->_GetContext( this )->Wait( hFence );
            }
        }
        return ret;
    }

    CFrameGraphNode* CFrameGraphNode::SetNext( CFrameGraphNode* pNext )
    {
        CFrameGraphNode* pThis = this;
        return m_pFrameGraph->_SetNextNode( &pThis, pNext );
    }

    void CFrameGraphNode::WaitFor( const SWaitInfo& Info )
    {
        m_vWaitForNodes.PushBack( Info );
        if( Info.WaitOn == WaitOnBits::GPU_WAITS_FOR_GPU )
        {
            Info.pNode->_SignalGPUFence();
        }
    }

    const TexturePtr CFrameGraphNode::GetColorRenderTarget( uint32_t index ) const
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
        if( m_pDepthStencilRenderTarget!= nullptr )
        {
            ret = m_pDepthStencilRenderTarget->GetDesc().format;
        }
        return ret;
    }

    

    CFrameGraphNode* CFrameGraphNode::AddSubpass( CFrameGraphNode* pNode, uint32_t index )
    {
        pNode->m_pParent   = this;
        pNode->m_isSubpass = true;
        auto ppCurr        = &m_pSubpassNode;

        for( uint32_t i = 0; i < index; ++i )
        {
            if( *ppCurr )
            {
                ppCurr = &( *ppCurr )->m_pNextNode;
            }
            else
            {
                break;
            }
        }
        pNode->m_fenceValue = m_pFrameGraph->_AcquireNodeIndex();
        pNode->m_pNextNode = *ppCurr;
        *ppCurr            = pNode;

        return this;
    }

    CFrameGraphNode* CFrameGraphNode::AddSubpass( cstr_t pName, FrameGraphWorkload&& Wl )
    {
        SFrameGraphNodeDesc Desc;
        Desc.pName = pName;
        auto pNode = m_pFrameGraph->CreatePass( Desc );
        auto pRet  = this;
        if( pNode )
        {
            pNode->m_fenceValue = m_pFrameGraph->_AcquireNodeIndex();
            pNode->SetWorkload( std::forward< FrameGraphWorkload >( Wl ) );
            pRet = AddSubpass( pNode );
        }
        return pRet;
    }

    Platform::ThreadFence& CFrameGraphNode::GetThreadFence()
    {
        // return m_pFrameGraph->_GetThreadFence( m_Index.threadFence );
        return m_hFence;
    }

    void CFrameGraphNode::SignalThreadFence( uint32_t value )
    {
        // std::unique_lock l( m_CondVarMtx );
        GetThreadFence().Store( value );
    }

    void CFrameGraphNode::IncrementThreadFence()
    {
        // std::unique_lock l( m_CondVarMtx );
        auto& Fence = GetThreadFence();
        ++Fence;
        // Platform::Debug::PrintOutput( "%s Signal fence: %d\n", m_Name.GetData(), Fence.Load() );
        // VKE_LOG_NO_SYNC( m_Name << " = " << Fence.Load() );
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
        static STaskResult  DummyTaskResult;
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
            bool taskExecuted            = Itr->Func( this, Desc.backBufferIndex );
            taskExecutedCount           += taskExecuted;
            bool removeTask              = taskExecuted || Desc.forceRemove;
            Itr->pResult->executedOnCPU  = true;
            if( removeTask )
            {
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
        /*uint32_t writeCount = 0;
        for( uint32_t i = 0; i < Desc.vRenderTargets.GetCount(); ++i )
        {
            const SFrameGraphRenderTargetTextureDesc& RTDesc   = Desc.vRenderTargets[ i ];
            TexturePtr                                pTexture = m_pFrameGraph->_GetTexture( RTDesc );
            if( pTexture!= nullptr )
            {
                writeCount           += ( RTDesc.operation == FrameGraphPassOperations::RENDER_PASS_WRITE ||
                                RTDesc.operation == FrameGraphPassOperations::RENDER_PASS_OVERWRITE ||
                                RTDesc.operation == FrameGraphPassOperations::SHADER_READ_WRITE );
                TextureViewPtr pView  = pTexture->GetView();
                if( pTexture->IsColor() )
                {
                    SRenderTargetInfo Info = { .hDDIView   = pView->GetDDIObject(),
                                               .format     = pView->GetDesc().format,
                                               .ClearColor = SClearValue( 0, 0, 0, 0 ),
                                               .state      = FrameGraphPassOpToColorTextureState( RTDesc.operation ),
                                               .renderPassOp =
                                                   FrameGraphPassToColorRenderTargetOp( RTDesc.operation ) };
                    m_BeginRenderPassInfo.vColorRenderTargetInfos.PushBack( Info );
                    m_vpColorRenderTargets.PushBack( pTexture );
                    m_vColorRenderTargetFormats.PushBack( RTDesc.format );
                }
                else
                {
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.hDDIView   = pView->GetDDIObject();
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.ClearColor = SClearValue( 1, 0 );
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.format     = pView->GetDesc().format;
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.renderPassOp =
                        FrameGraphPassToDepthRenderTargetOp( RTDesc.operation );
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.state =
                        FrameGraphPassOpToDepthTextureState( RTDesc.operation );
                    m_pDepthStencilRenderTarget = pTexture;
                }
            }
        }
        m_BeginRenderPassInfo.SetDebugName( m_Name.GetData() );
        m_BeginRenderPassInfo.RenderArea = m_pFrameGraph->_GetRenderArea( Desc.size );
        m_hasRenderPass                  = writeCount > 0;*/

        SRenderPassDesc RpDesc;
        uint32_t            writeCount = 0;
        for( uint32_t i = 0; i < Desc.vRenderTargets.GetCount(); ++i )
        {
            const SFrameGraphRenderTargetTextureDesc& RpRTDesc = Desc.vRenderTargets[ i ];
            TexturePtr                                pTexture = m_pFrameGraph->_GetTexture( RpRTDesc );
            if( pTexture != nullptr )
            {
                writeCount           += ( RpRTDesc.operation == FrameGraphPassOperations::RENDER_PASS_OVERWRITE ||
                                RpRTDesc.operation == FrameGraphPassOperations::RENDER_PASS_WRITE ||
                                RpRTDesc.operation == FrameGraphPassOperations::SHADER_READ_WRITE );
                TextureViewPtr pView  = pTexture->GetView();
                if( pTexture->IsColor() )
                {
                    SRenderTargetInfo Info = { .hDDIView   = pView->GetDDIObject(),
                                               .format     = pView->GetDesc().format,
                                               .ClearColor = SClearValue( 0, 0, 0, 0 ),
                                               .state      = FrameGraphPassOpToColorTextureState( RpRTDesc.operation ),
                                               .renderPassOp =
                                                   FrameGraphPassToColorRenderTargetOp( RpRTDesc.operation ) };
                    m_BeginRenderPassInfo.vColorRenderTargetInfos.PushBack( Info );
                    m_vpColorRenderTargets.PushBack( pTexture );
                    m_vColorRenderTargetFormats.PushBack( RpRTDesc.format );

                    SRenderPassDesc::SRenderTargetDesc RtDesc;
                    RtDesc.beginState  = Info.state;
                    RtDesc.endState    = Info.state;
                    RtDesc.ClearValue  = Info.ClearColor;
                    RtDesc.format      = Info.format;
                    RtDesc.hRHIView = pTexture->GetView()->GetDDIObject();
                    RtDesc.usage       = FrameGraphPassOpToColorRenderTargetOp( RpRTDesc.operation );
                    RtDesc.SetDebugName( RpRTDesc.pName );
                    RpDesc.vRenderTargets.PushBack( RtDesc );
                }
                else
                {
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.hDDIView   = pView->GetDDIObject();
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.ClearColor = SClearValue( 1, 0 );
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.format     = pView->GetDesc().format;
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.renderPassOp =
                        FrameGraphPassToDepthRenderTargetOp( RpRTDesc.operation );
                    m_BeginRenderPassInfo.DepthRenderTargetInfo.state =
                        FrameGraphPassOpToDepthTextureState( RpRTDesc.operation );
                    m_pDepthStencilRenderTarget = pTexture;

                    SRenderPassDesc::SRenderTargetDesc RtDesc;
                    RtDesc.beginState  = m_BeginRenderPassInfo.DepthRenderTargetInfo.state;
                    RtDesc.endState    = m_BeginRenderPassInfo.DepthRenderTargetInfo.state;
                    RtDesc.ClearValue  = m_BeginRenderPassInfo.DepthRenderTargetInfo.ClearColor;
                    RtDesc.format      = m_BeginRenderPassInfo.DepthRenderTargetInfo.format;
                    RtDesc.hRHIView = pTexture->GetView()->GetDDIObject();
                    RtDesc.usage       = FrameGraphPassOpToDepthRenderTargetOp( RpRTDesc.operation );
                    RtDesc.SetDebugName( RpRTDesc.pName );
                    RpDesc.vRenderTargets.PushBack( RtDesc );
                }
            }
        }
        if( writeCount > 0 )
        {
            RpDesc.Name = Desc.pName;
            RpDesc.SetDebugName( Desc.pName );
            RpDesc.Size           = m_pFrameGraph->_GetRenderArea( Desc.size ).Size;
            RpDesc.PositionOffset = m_pFrameGraph->_GetRenderArea( Desc.size ).Position;

            m_hRHIRenderPass = m_pContext->GetDeviceContext()->CreateRenderPass( RpDesc );
        }

        m_BeginRenderPassInfo.SetDebugName( m_Name.GetData() );
        m_BeginRenderPassInfo.RenderArea = m_pFrameGraph->_GetRenderArea( Desc.size );
        m_hasRenderPass                  = writeCount > 0;
    }

    void CFrameGraphNode::_BeginRenderPass()
    {
        if( HasCommandBuffer() )
        {
        }
    }

    Scene::ScenePtr CFrameGraphNode::GetScene()
    {
        return m_pFrameGraph->GetScene();
    }

} // namespace VKE::RenderSystem

namespace VKE::RenderSystem
{
    CFrameGraphExecuteNode* CFrameGraphExecuteNode::AddToExecute( CFrameGraphNode* pNode )
    {
        VKE_ASSERT( pNode->m_pExecuteNode == nullptr );
        m_vpNodesToExecute.PushBackUnique( pNode );
        pNode->m_pExecuteNode = this;
        return this;
    }

    CFrameGraphExecuteNode* CFrameGraphExecuteNode::AddToExecute( CFrameGraphExecuteNode* pNode )
    {
        return static_cast< CFrameGraphExecuteNode* >( this->AddSubpass( pNode ) );
    }

    CFrameGraphExecuteNode::SExecuteData* CFrameGraphExecuteNode::_BuildDataToExecute( uint8_t backBufferIndex )
    {
        auto&  Exe = m_aExecutes[ backBufferIndex ];
        Exe.vpCommandBuffers.Clear();
        for( uint32_t n = 0; n < m_vWaitForNodes.GetCount(); ++n )
        {
            const auto& WaitInfo = m_vWaitForNodes[ n ];
            if( WaitInfo.pNode->IsEnabled() && WaitInfo.WaitOn != WaitOnBits::NONE )
            {
                // GPU waits on GPU
                if( WaitInfo.WaitOn == WaitOnBits::GPU_WAITS_FOR_GPU )
                {
                    auto waitForValue = WaitInfo.pNode->GetFenceValue();
                    VKE_ASSERT( waitForValue < GetFenceValue() );
                    // Wait for highest value
                    Exe.SubmitInfo.waitForFenceValue = Math::Max( waitForValue, Exe.SubmitInfo.waitForFenceValue );
                }
                // CPU waits for GPU
                if( WaitInfo.WaitOn == WaitOnBits::CPU_WAITS_FOR_GPU )
                {
                    auto waitForValue = WaitInfo.pNode->GetFenceValue();
                    VKE_ASSERT( waitForValue < GetFenceValue() );
                    // Wait for highest value
                    Exe.SubmitInfo.waitForFenceValue = Math::Max( waitForValue, Exe.SubmitInfo.waitForFenceValue );
                }
                if( WaitInfo.WaitOn == WaitOnBits::CPU_WAITS_FOR_GPU )
                {
                    auto waitForValue = WaitInfo.pNode->GetFenceValue();
                    VKE_ASSERT( waitForValue < GetFenceValue() );
                    // Wait for highest value
                    Exe.SubmitInfo.waitForFenceValue = Math::Max( waitForValue, Exe.SubmitInfo.waitForFenceValue );
                }
            }
        }
        for( uint32_t nodeIdx = 0; nodeIdx < m_vpNodesToExecute.GetCount(); ++nodeIdx )
        {
            auto& pNode = m_vpNodesToExecute[ nodeIdx ];
            if( pNode->IsEnabled() )
            {
                auto pCb = pNode->GetCommandBuffer( backBufferIndex );
                if( pCb!= nullptr && pCb->GetState() == CommandBufferStates::BEGIN )
                {
                    if( VKE_SUCCEEDED(pCb->End()))
                    {
                        if( VKE_SUCCEEDED( pCb->Flush() ) )
                        {
                            // VKE_LOG( "Execute batch: " << this->m_Name.GetData() << ", cb: " << pCb->GetDebugName()
                            // );
                            Exe.vpCommandBuffers.PushBackUnique( pCb->GetDDIObject() );
                        }
                        else
                        {
                            VKE_LOG_ERR( "Unable to flush cmd buffer." );
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to end cmd buffer." );
                    }
                }
            }
        }
        //m_fenceValue = m_pFrameGraph->IncrementFrameFenceValue( backBufferIndex );
        //VKE_ASSERT( m_fenceValue > m_pFrameGraph->m_aFrameData[backBufferIndex].frameFenceValue );
        //m_fenceValue                      = m_pFrameGraph->m_aFrameData[ backBufferIndex ].frameFenceValue + 1;
        Exe.SubmitInfo.commandBufferCount = (uint16_t)Exe.vpCommandBuffers.GetCount();
        Exe.SubmitInfo.pDDICommandBuffers = Exe.vpCommandBuffers.GetData();
        Exe.SubmitInfo.signalFenceValue   = GetFenceValue();
        Exe.SubmitInfo.hSignalFence       = m_pFrameGraph->GetFrameFence( backBufferIndex );
        Exe.SubmitInfo.hDDIQueue          = this->GetContext()->GetNativeQueue();
     
        return &Exe;
    }
} // namespace VKE::RenderSystem
