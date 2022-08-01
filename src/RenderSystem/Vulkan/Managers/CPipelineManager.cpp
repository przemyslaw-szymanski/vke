#include "RenderSystem/Vulkan/Managers/CPipelineManager.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CVkEngine.h"
#include "Core/Threads/CThreadPool.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CRenderPass.h"
#include "Core/Utils/CProfiler.h"
#include "RenderSystem/CSwapChain.h"

namespace VKE
{
    namespace RenderSystem
    {
        TaskState PipelineManagerTasks::SCreatePipelineTask::_OnStart(uint32_t)
        {
            return TaskStateBits::OK;
        }

        void PipelineManagerTasks::SCreatePipelineTask::_OnGet(void**)
        {

        }

        CPipelineManager::CPipelineManager(CDeviceContext* pCtx) :
            m_pCtx( pCtx )
        {

        }

        CPipelineManager::~CPipelineManager()
        {
            Destroy();
        }

        Result CPipelineManager::Create(const SPipelineManagerDesc& Desc)
        {
            Result res = VKE_FAIL;
            if( VKE_SUCCEEDED( m_PipelineMemMgr.Create( Desc.maxPipelineCount, sizeof( CPipeline ), 1 ) ) )
            {
                if( VKE_SUCCEEDED( m_PipelineLayoutMemMgr.Create( Desc.maxPipelineLayoutCount, sizeof( CPipelineLayout ), 1 ) ) )
                {
                    res = VKE_OK;
                }
                else
                {
                    goto ERR;
                }
            }
            {
                auto hDescSetLayout = m_pCtx->GetDefaultDescriptorSetLayout();
                SPipelineLayoutDesc LayoutDesc( hDescSetLayout );
                m_pDefaultLayout = CreateLayout( LayoutDesc );
            }
            
            return res;
ERR:
            Destroy();
            return res;
        }

        void CPipelineManager::Destroy()
        {
            m_pDefaultLayout = nullptr;
            m_pCurrPipeline = nullptr;
            for( auto& Itr : m_Buffer.mContainer )
            {
                CPipeline* pPipeline = Itr.second;
                _DestroyPipeline( &pPipeline );
                Itr.second = nullptr;
            }
            m_Buffer.Clear();

            for( auto& Itr : m_LayoutBuffer.mContainer )
            {
                CPipelineLayout* pLayout = Itr.second;
                _DestroyLayout( &pLayout );
                Itr.second = nullptr;
            }
            m_LayoutBuffer.Clear();

            m_PipelineLayoutMemMgr.Destroy();
            m_PipelineMemMgr.Destroy();
        }

        void CPipelineManager::_DestroyPipeline( CPipeline** ppPipeline )
        {
            CPipeline* pPipeline = *ppPipeline;
            pPipeline->_Destroy();
            auto& hDDIObj = pPipeline->m_hDDIObject;
            m_pCtx->DDI().DestroyPipeline( &hDDIObj, nullptr );
            Memory::DestroyObject( &m_PipelineMemMgr, &pPipeline );
            *ppPipeline = nullptr;
        }

        void CPipelineManager::_DestroyLayout( CPipelineLayout** ppLayout )
        {
            CPipelineLayout* pLayout = *ppLayout;
            auto& hDDIObj = pLayout->m_hDDIObject;
            m_pCtx->DDI().DestroyPipelineLayout( &hDDIObj, nullptr );
            Memory::DestroyObject( &m_PipelineLayoutMemMgr, &pLayout );
            *ppLayout = nullptr;
        }

        void CPipelineManager::DestroyPipeline( PipelinePtr* pInOut )
        {
            CPipeline* pPipeline = (*pInOut).Release();
            const auto handle = ( pPipeline->GetHandle().handle );
            m_Buffer.Remove( handle );
            _DestroyPipeline( &pPipeline );
        }

        void CPipelineManager::DestroyLayout( PipelineLayoutPtr* pInOut )
        {
            CPipelineLayout* pLayout = (*pInOut).Release();
            const auto handle = ( pLayout->GetHandle().handle );
            m_LayoutBuffer.Remove( handle );
            _DestroyLayout( &pLayout );
        }

        PipelineRefPtr CPipelineManager::CreatePipeline(const SPipelineCreateDesc& Desc)
        {
            hash_t hash = _CalcHash( Desc.Pipeline );
            CPipeline* pPipeline = nullptr;
            PipelineRefPtr pRet;
            if( m_currPipelineHash == hash )
            {
                pPipeline = m_pCurrPipeline;
                pRet = PipelineRefPtr{ pPipeline };
            }
            else
            {
                //VKE_SIMPLE_PROFILE();
                {
                    Threads::ScopedLock l( m_CreatePipelineSyncObj );
                    if( !m_Buffer.Find( hash, &pPipeline ) )
                    {
                        if( VKE_SUCCEEDED( Memory::CreateObject( &m_PipelineMemMgr, &pPipeline, this ) ) )
                        {
                            if( m_Buffer.Add( hash, pPipeline ) )
                            {
                                pPipeline->m_Desc = Desc.Pipeline;
                                pPipeline->m_hObject.handle = hash;
                                pPipeline->m_hDDIObject = _GetDefaultPipeline( Desc.Pipeline );
                                m_currPipelineHash = hash;
                                m_pCurrPipeline = pPipeline;

                                pRet = PipelineRefPtr( pPipeline );
                            }
                            else
                            {
                                VKE_LOG_ERR( "Unable to add pipeline object to the buffer." );
                                goto ERR;
                            }
                        }
                        else
                        {
                            VKE_LOG_ERR( "Unable to allocate memory for pipeline object." );
                            goto ERR;
                        }
                    }
                    else
                    {
                        pRet = PipelineRefPtr{ pPipeline };
                    }
                }

                if( (Desc.Create.flags & Core::CreateResourceFlags::ASYNC) == Core::CreateResourceFlags::ASYNC )
                {
                    PipelineManagerTasks::SCreatePipelineTask* pTask;
                    {
                        Threads::ScopedLock l( m_CreatePipelineSyncObj );
                        pTask = CreatePipelineTaskPoolHelper::GetTask( &m_CreatePipelineTaskPool );
                    }
                    pTask->pMgr = this;
                    pTask->pPipeline = pPipeline;
                    pTask->Func = [ & ]( Threads::ITask* pThisTask )
                    {
                        uint32_t ret = TaskStateBits::FAIL;
                        auto pTask = ( PipelineManagerTasks::SCreatePipelineTask* )pThisTask;
                        Result res = _CreatePipelineTask( &pTask->pPipeline );
                        if( VKE_SUCCEEDED( res ) )
                        {
                            ret = TaskStateBits::OK;
                        }
                        return ret;
                    };
                    m_pCtx->GetRenderSystem()->GetEngine()->GetThreadPool()->AddTask( pTask );
                }
                else
                {
                    if( VKE_FAILED( _CreatePipelineTask( Desc.Pipeline, &pPipeline ) ) )
                    {
                        goto ERR;
                    }
                }
            }

            
            return pRet;
        ERR:
            if( pRet.IsValid() )
            {
                pPipeline = pRet.Release();
                _DestroyPipeline( &pPipeline );
            }
            return pRet;
        }

        DDIPipeline CPipelineManager::_GetDefaultPipeline( const SPipelineDesc& Desc )
        {
            DDIPipeline hRet = DDI_NULL_HANDLE;
            if( Desc.pDefault.IsValid() && Desc.pDefault->IsReady() )
            {
                hRet = Desc.pDefault->GetDDIObject();
            }
            else
            if( Desc.hLayout != INVALID_HANDLE )
            {
                //auto hash = GetLayout( Desc.hLayout )->GetHandle();
                hRet = m_mDefaultDDIPipelines[ Desc.hLayout.handle ];
            }
            return hRet;
        }

        Result CPipelineManager::_CreatePipelineTask( CPipeline** ppInOut )
        {
            Result ret = VKE_FAIL;

            auto pPipeline = *ppInOut;
            if( !pPipeline->IsReady() )
            {
                auto& Desc = pPipeline->m_Desc;
                if( Desc.hDDILayout == DDI_NULL_HANDLE )
                {
                    VKE_ASSERT( Desc.hLayout != INVALID_HANDLE, "" );
                    {
                        pPipeline->m_pLayout = m_pCtx->GetPipelineLayout( Desc.hLayout );
                        pPipeline->m_Desc.hDDILayout = pPipeline->m_pLayout->GetDDIObject();
                    }
                }

                auto& Shaders = pPipeline->m_Desc.Shaders;
                // Wait for async loaded shaders
                for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
                {
                    auto& pCurr = Shaders.apShaders[ i ];
                    while( pCurr.IsValid() && !pCurr->IsReady() )
                    {
                        Platform::ThisThread::Pause();
                        if( pCurr->IsInvalid() )
                        {
                            VKE_LOG_ERR("Unable to create pipeline because shader is not compiled.");
                            goto ERR;
                        }
                    }
                }

                DDIPipeline hPipeline = m_pCtx->_GetDDI().CreatePipeline( pPipeline->m_Desc, nullptr );
                if( hPipeline != DDI_NULL_HANDLE && VKE_SUCCEEDED( pPipeline->Init( Desc ) ) )
                {
                    pPipeline->m_hDDIObject = hPipeline;
                    pPipeline->m_resourceStates |= Core::ResourceStates::PREPARED;
                    ret = VKE_OK;
                }
                else
                {
                    pPipeline = nullptr;
                }
            }

            VKE_ASSERT( pPipeline->GetDDIObject() != DDI_NULL_HANDLE, "Pipeline API object not created." );
            return ret;
        ERR:
            pPipeline->m_resourceStates |= Core::ResourceStates::INVALID;
            ret = VKE_FAIL;
            return ret;
        }

        Result CPipelineManager::_CreatePipelineTask(const SPipelineDesc& Desc, CPipeline** ppOut)
        {
            Result ret = VKE_FAIL;
            
            auto pPipeline = *ppOut;
            if( !pPipeline->IsReady() )
            {
                pPipeline->m_Desc = Desc;
                if( Desc.hDDILayout == DDI_NULL_HANDLE )
                {
                    VKE_ASSERT( Desc.hLayout != INVALID_HANDLE, "" );
                    {
                        pPipeline->m_pLayout = m_pCtx->GetPipelineLayout( Desc.hLayout );
                        pPipeline->m_Desc.hDDILayout = pPipeline->m_pLayout->GetDDIObject();
                    }
                }

                DDIPipeline hPipeline = m_pCtx->_GetDDI().CreatePipeline( pPipeline->m_Desc, nullptr );
                if( hPipeline != DDI_NULL_HANDLE && VKE_SUCCEEDED( pPipeline->Init( Desc ) ) )
                {
                    pPipeline->m_hDDIObject = hPipeline;
                    pPipeline->m_resourceStates |= Core::ResourceStates::PREPARED;
                    ret = VKE_OK;
                }
                else
                {
                    pPipeline = nullptr;
                }
            }

            VKE_ASSERT( pPipeline->GetDDIObject() != DDI_NULL_HANDLE, "Pipeline API object not created." );

            return ret;
        }

        hash_t CPipelineManager::_CalcHash(const SPipelineDesc& Desc)
        {
            //VKE_SIMPLE_PROFILE();
            hash_t hash = 0;

            Utils::Hash::Combine( &hash, Desc.hRenderPass.handle );
            Utils::Hash::Combine( &hash, Desc.hDDIRenderPass );
            Utils::Hash::Combine( &hash, Desc.hLayout.handle );
            Utils::Hash::Combine( &hash, Desc.hDDILayout );
            /*hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pComputeShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pVertexShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pTessHullShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pTessDomainShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pGeometryShader.Get() );
            hash ^= reinterpret_cast< uint64_t >( Desc.Shaders.pPpixelShader.Get() );*/
            for( uint32_t i = 0; i < ShaderTypes::_MAX_COUNT; ++i )
            {
                auto pShader = Desc.Shaders.apShaders[ i ];
                if( pShader.IsValid() )
                {
                    Utils::Hash::Combine( &hash, pShader->GetHandle().handle );
                }
            }

#define FLOAT_TO_INT(f) (static_cast<int32_t>((f)*1000))

            {
                //blendingHash = Desc.Blending.enableLogicOperation ^ ( Desc.Blending.logicOperation << 1);
                Utils::Hash::Combine( &hash, Desc.Blending.enable );
                Utils::Hash::Combine( &hash, Desc.Blending.logicOperation );
                Utils::Hash::Combine( &hash, Desc.Blending.vBlendStates.GetCount() );

                for (uint32_t i = 0; i < Desc.Blending.vBlendStates.GetCount(); ++i)
                {
                    const auto& State = Desc.Blending.vBlendStates[i];
                    /*hash_t hash = (State.enable << 1) ^ ( State.writeMask << 1 );
                    hash ^= (State.Color.src << 1) ^ (State.Color.dst << 1) ^ (State.Color.operation << 1);
                    hash ^= (State.Alpha.src << 1) ^ (State.Alpha.dst << 1) ^ (State.Alpha.operation << 1);
                    blendingHash ^= hash << 1;*/
                    Utils::Hash::Combine( &hash, State.Alpha.dst );
                    Utils::Hash::Combine( &hash, State.Alpha.operation );
                    Utils::Hash::Combine( &hash, State.Alpha.src );
                    Utils::Hash::Combine( &hash, State.Color.dst );
                    Utils::Hash::Combine( &hash, State.Color.operation );
                    Utils::Hash::Combine( &hash, State.Color.src );
                    Utils::Hash::Combine( &hash, State.enable );
                    Utils::Hash::Combine( &hash, State.writeMask );
                }
            }
            {
                const auto& Raster = Desc.Rasterization;
                //rasterHash = FLOAT_TO_INT(Raster.Depth.biasClampFactor) ^ ( FLOAT_TO_INT(Raster.Depth.biasConstantFactor) << 1 );
                //rasterHash ^= ( FLOAT_TO_INT(Raster.Depth.biasSlopeFactor) << 1 ) ^ ( FLOAT_TO_INT(Raster.Depth.enableClamp) << 1 );
                //rasterHash ^= (Raster.Polygon.cullMode << 1) ^ (Raster.Polygon.frontFace << 1) ^ (Raster.Polygon.frontFace << 1);
                Utils::Hash::Combine( &hash, Raster.Depth.biasClampFactor );
                Utils::Hash::Combine( &hash, Raster.Depth.biasConstantFactor );
                Utils::Hash::Combine( &hash, Raster.Depth.biasClampFactor );
                Utils::Hash::Combine( &hash, Raster.Depth.enableClamp );
                Utils::Hash::Combine( &hash, Raster.Polygon.cullMode );
                Utils::Hash::Combine( &hash, Raster.Polygon.frontFace );
                Utils::Hash::Combine( &hash, Raster.Polygon.mode );
            }
            {
                //Viewport
                const auto& Viewport = Desc.Viewport;
                for( uint32_t i = 0; i < Viewport.vViewports.GetCount(); ++i )
                {
                    const auto& Curr = Viewport.vViewports[i];
                    Utils::Hash::Combine( &hash, Curr.Size.width );
                    Utils::Hash::Combine( &hash, Curr.Size.height );
                    Utils::Hash::Combine( &hash, Curr.Position.x );
                    Utils::Hash::Combine( &hash, Curr.Position.y );
                    Utils::Hash::Combine( &hash, Curr.MinMaxDepth.begin );
                    Utils::Hash::Combine( &hash, Curr.MinMaxDepth.end );
                }
                for( uint32_t i = 0; i < Viewport.vScissors.GetCount(); ++i )
                {
                    const auto& Curr = Viewport.vScissors[i];
                    Utils::Hash::Combine( &hash, Curr.Size.width );
                    Utils::Hash::Combine( &hash, Curr.Size.height );
                    Utils::Hash::Combine( &hash, Curr.Position.x );
                    Utils::Hash::Combine( &hash, Curr.Position.y );
                }
            }
            {
                // Multisampling
                const auto& MS = Desc.Multisampling;
                Utils::Hash::Combine( &hash, MS.enable );
                Utils::Hash::Combine( &hash, MS.sampleCount );
            }
            {
                const auto& DS = Desc.DepthStencil;
                {
                    const auto& Face = DS.Stencil.BackFace;
                    Utils::Hash::Combine( &hash, Face.compareMask );
                    Utils::Hash::Combine( &hash, Face.compareFunc );
                    Utils::Hash::Combine( &hash, Face.depthFailFunc );
                    Utils::Hash::Combine( &hash, Face.failFunc );
                    Utils::Hash::Combine( &hash, Face.passFunc );
                    Utils::Hash::Combine( &hash, Face.reference );
                    Utils::Hash::Combine( &hash, Face.writeMask );
                }
                {
                    const auto& Face = DS.Stencil.FrontFace;
                    Utils::Hash::Combine( &hash, Face.compareMask );
                    Utils::Hash::Combine( &hash, Face.compareFunc );
                    Utils::Hash::Combine( &hash, Face.depthFailFunc );
                    Utils::Hash::Combine( &hash, Face.failFunc );
                    Utils::Hash::Combine( &hash, Face.passFunc );
                    Utils::Hash::Combine( &hash, Face.reference );
                    Utils::Hash::Combine( &hash, Face.writeMask );
                }
                {
                    const auto& DB = DS.Depth.Bounds;
                    Utils::Hash::Combine( &hash, DB.enable );
                    Utils::Hash::Combine( &hash, DB.max );
                    Utils::Hash::Combine( &hash, DB.min );
                }
                Utils::Hash::Combine( &hash, DS.Depth.compareFunc );
                Utils::Hash::Combine( &hash, DS.Depth.enable );
                Utils::Hash::Combine( &hash, DS.Depth.test );
                Utils::Hash::Combine( &hash, DS.Depth.write );
                Utils::Hash::Combine( &hash, DS.Stencil.test );
                Utils::Hash::Combine( &hash, DS.Stencil.enable );
            }
            {
                const auto& IL = Desc.InputLayout;
                for( uint32_t i = 0; i < IL.vVertexAttributes.GetCount(); ++i )
                {
                    const auto& Attr = IL.vVertexAttributes[ i ];
                    Utils::Hash::Combine( &hash, Attr.vertexBufferBindingIndex );
                    Utils::Hash::Combine( &hash, Attr.format );
                    Utils::Hash::Combine( &hash, Attr.inputRate );
                    Utils::Hash::Combine( &hash, Attr.location );
                    Utils::Hash::Combine( &hash, Attr.offset );
                    Utils::Hash::Combine( &hash, Attr.stride );
                }
                Utils::Hash::Combine( &hash, IL.vVertexAttributes.GetCount() );
                Utils::Hash::Combine( &hash, IL.enablePrimitiveRestart );
                Utils::Hash::Combine( &hash, IL.topology );
            }

            return hash;
        }

        hash_t CPipelineManager::_CalcHash(const SPipelineLayoutDesc& Desc)
        {
            Utils::SHash Hash;
            Hash += Desc.vDescriptorSetLayouts.GetCount();
            for( uint32_t i = 0; i < Desc.vDescriptorSetLayouts.GetCount(); ++i )
            {
                //hash ^= ( reinterpret_cast< uint64_t >( Desc.vDescriptorSetLayouts[ i ].handle ) << 1 );
                Hash += Desc.vDescriptorSetLayouts[ i ].handle;
            }
            return Hash.value;
        }

        PipelineLayoutRefPtr CPipelineManager::CreateLayout(const SPipelineLayoutDesc& Desc)
        {
            hash_t hash = _CalcHash( Desc );
            PipelineLayoutRefPtr pRet;
            Threads::ScopedLock l( m_LayoutSyncObj );
            {
                CPipelineLayout* pLayout = nullptr;
                if( !m_LayoutBuffer.Find( hash, &pLayout ) )
                {
                    if( VKE_SUCCEEDED( Memory::CreateObject( &m_PipelineLayoutMemMgr, &pLayout, this ) ) )
                    {
                        if( m_LayoutBuffer.Add( hash, { pLayout } ) )
                        {
                            pRet = PipelineLayoutRefPtr{ pLayout };
                        }
                        else
                        {
                            VKE_LOG_ERR( "Unable to add CPipelineLayout object to the resource buffer." );
                            goto ERR;
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to allocate memory for CPipelineLayout." );
                        goto ERR;
                    }
                }
                else
                {
                    pRet = PipelineLayoutRefPtr{ pLayout };
                }
            }
            if( pRet.IsValid() )
            {
                CPipelineLayout* pLayout = pRet.Get();
                if( pLayout->GetDDIObject() == DDI_NULL_HANDLE )
                {
                    DDIPipelineLayout hLayout = m_pCtx->_GetDDI().CreatePipelineLayout( Desc, nullptr );
                    if( hLayout != DDI_NULL_HANDLE )
                    {
                        pLayout->Init( Desc );
                        pLayout->m_hDDIObject = hLayout;
                        pLayout->m_hObject.handle = hash;
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to create VkPipelineLayout object." );
                        goto ERR;
                    }
                }
            }
            return pRet;
        ERR:
            if( pRet.IsValid() )
            {
                CPipelineLayout* pLayout = pRet.Release();
                _DestroyLayout( &pLayout );
            }
            return pRet;
        }

        /*PipelinePtr CPipelineManager::_CreateCurrPipeline(bool createAsync)
        {
            if( m_isCurrPipelineDirty )
            {
                m_CurrPipelineDesc.Create.async = createAsync;
                m_pCurrPipeline = CreatePipeline( m_CurrPipelineDesc );
                m_isCurrPipelineDirty = false;
            }
            return m_pCurrPipeline;
        }*/

        PipelineRefPtr CPipelineManager::GetPipeline( PipelineHandle hPipeline )
        {
            PipelineRefPtr pRet;
            CPipeline* pTmp;
            m_Buffer.Find( hPipeline.handle, &pTmp );
            pRet = PipelineRefPtr{ pTmp };
            return pRet;
        }

        PipelineLayoutRefPtr CPipelineManager::GetLayout( PipelineLayoutHandle hLayout )
        {
            PipelineLayoutRefPtr pRet;
            CPipelineLayout* pTmp;
            if( m_LayoutBuffer.Find( hLayout.handle, &pTmp ) )
            {
                pRet = PipelineLayoutRefPtr{ pTmp };
            }
            return pRet;
        }

        PipelineRefPtr CPipelineManager::GetLastCreatedPipeline() const
        {
            return PipelineRefPtr{ m_pCurrPipeline };
        }

    } // RenderSystem

    namespace RenderSystem
    {
        void CPipelineBuilder::SetParent( PipelinePtr pParent )
        {
            m_pParent = pParent;
            m_Desc = m_pParent->GetDesc();
            m_Desc.hDDIParent = m_pParent->GetDDIObject();
        }

        void CPipelineBuilder::Bind( const RenderPassHandle& hPass )
        {
            m_Desc.hRenderPass = hPass;
        }

        void CPipelineBuilder::Bind( RenderPassPtr pPass )
        {
            m_Desc.hDDIRenderPass = pPass->GetDDIObject();
        }

        void CPipelineBuilder::Bind( const DDIRenderPass& hDDIPass )
        {
            m_Desc.hDDIRenderPass = hDDIPass;
        }

        void CPipelineBuilder::Bind( const CSwapChain* pSwpChain )
        {
            m_Desc.hDDIRenderPass = pSwpChain->GetDDIRenderPass();
        }

        void CPipelineBuilder::Bind( const DescriptorSetHandle& hSet, const uint32_t)
        {
            m_vDescSets.PushBack( hSet );
        }

        void CPipelineBuilder::SetState( const ShaderHandle& hShader )
        {
            m_vShaders.PushBack( hShader );
        }

        void CPipelineBuilder::SetState( const PipelineLayoutHandle& hLayout )
        {
            m_Desc.hLayout = hLayout;
        }
        
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM