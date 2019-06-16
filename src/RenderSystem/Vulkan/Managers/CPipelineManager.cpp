#include "RenderSystem/Vulkan/Managers/CPipelineManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CVkEngine.h"
#include "Core/Threads/CThreadPool.h"
#include "RenderSystem/Resources/CShader.h"
#include "RenderSystem/CRenderPass.h"
#include "Core/Utils/CProfiler.h"

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
                CPipeline* pPipeline = Itr.second.Release();
                _DestroyPipeline( &pPipeline );
                Itr.second = nullptr;
            }
            m_Buffer.Clear();

            for( auto& Itr : m_LayoutBuffer.mContainer )
            {
                CPipelineLayout* pLayout = Itr.second.Release();
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
            auto& hDDIObj = pPipeline->m_hDDIObject;
            m_pCtx->DDI().DestroyObject( &hDDIObj, nullptr );
            Memory::DestroyObject( &m_PipelineMemMgr, &pPipeline );
            *ppPipeline = nullptr;
        }

        void CPipelineManager::_DestroyLayout( CPipelineLayout** ppLayout )
        {
            CPipelineLayout* pLayout = *ppLayout;
            auto& hDDIObj = pLayout->m_hDDIObject;
            m_pCtx->DDI().DestroyObject( &hDDIObj, nullptr );
            Memory::DestroyObject( &m_PipelineLayoutMemMgr, &pLayout );
            *ppLayout = nullptr;
        }

        void CPipelineManager::DestroyPipeline( PipelinePtr* pInOut )
        {
            CPipeline* pPipeline = (*pInOut).Release();
            const auto handle = static_cast< uint32_t >( pPipeline->GetHandle() );
            m_Buffer.Remove( handle );
            _DestroyPipeline( &pPipeline );
        }

        void CPipelineManager::DestroyLayout( PipelineLayoutPtr* pInOut )
        {
            CPipelineLayout* pLayout = (*pInOut).Release();
            const auto handle = static_cast< uint32_t >( pLayout->GetHandle() );
            m_LayoutBuffer.Remove( handle );
            _DestroyLayout( &pLayout );
        }

        DDIPipeline CPipelineManager::_CreatePipeline(const SPipelineDesc& Desc)
        {
            return m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
        }

        PipelineRefPtr CPipelineManager::CreatePipeline(const SPipelineCreateDesc& Desc)
        {
            PipelinePtr pPipeline;
            if (Desc.Create.async)
            {
                PipelineManagerTasks::SCreatePipelineTask* pTask;
                {
                    Threads::ScopedLock l( m_CreatePipelineSyncObj );
                    pTask = CreatePipelineTaskPoolHelper::GetTask( &m_CreatePipelineTaskPool );
                }
                pTask->pMgr = this;
                pTask->Desc = Desc;
                m_pCtx->GetRenderSystem()->GetEngine()->GetThreadPool()->AddTask( pTask );
            }
            else
            {
                _CreatePipelineTask( Desc.Pipeline, &pPipeline );
            }
            return PipelineRefPtr( pPipeline );
        }

        Result CPipelineManager::_CreatePipelineTask(const SPipelineDesc& Desc, PipelinePtr* ppOut)
        {
            Result ret = VKE_FAIL;
            hash_t hash = _CalcHash( Desc );
            CPipeline* pPipeline = nullptr;
            if( m_currPipelineHash == hash )
            {
                ret = VKE_OK;
                pPipeline = m_pCurrPipeline;
            }
            else
            {
                //VKE_SIMPLE_PROFILE();
                PipelineRefPtr pRef;
                if( !m_Buffer.Find( hash, &pRef ) )
                {
                    if( VKE_SUCCEEDED( Memory::CreateObject( &m_PipelineMemMgr, &pPipeline, this ) ) )
                    {
                        pRef = PipelineRefPtr( pPipeline );
                        if( m_Buffer.Add( hash, pRef ) )
                        {
                        }
                        else
                        {
                            VKE_LOG_ERR( "Unable to add pipeline object to the buffer." );
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to allocate memory for pipeline object." );
                    }
                }
                if( pRef.IsValid() )
                {
                    pPipeline = pRef.Get();
                    if( pPipeline->GetDDIObject() == DDI_NULL_HANDLE )
                    {
                        pPipeline->m_Desc = Desc;
                        if( Desc.hDDILayout == DDI_NULL_HANDLE )
                        {
                            if( Desc.hLayout == NULL_HANDLE )
                            {
                                VKE_ASSERT( pPipeline->m_Desc.pLayoutDesc != nullptr, "" );
                                pPipeline->m_Desc.hDDILayout = m_pCtx->CreatePipelineLayout( *pPipeline->m_Desc.pLayoutDesc )->GetDDIObject();
                            }
                            else
                            {
                                pPipeline->m_Desc.hDDILayout = m_pCtx->GetPipelineLayout( Desc.hLayout )->GetDDIObject();
                            }
                        }
                        DDIPipeline hPipeline = _CreatePipeline( Desc );
                        if( hPipeline != DDI_NULL_HANDLE && VKE_SUCCEEDED( pPipeline->Init( Desc ) ) )
                        {
                            pPipeline->m_hDDIObject = hPipeline;
                            pPipeline->m_hObject = hash;
                            m_currPipelineHash = hash;
                            m_pCurrPipeline = pPipeline;
                            ret = VKE_OK;
                        }
                        else
                        {
                            pPipeline = nullptr;
                        }
                    }

                }
            }
            VKE_ASSERT( pPipeline != nullptr, "Pipeline not created." );
            VKE_ASSERT( pPipeline->GetDDIObject() != DDI_NULL_HANDLE, "Pipeline API object not created." );
            //if( pPipeline->GetDDIObject() != DDI_NULL_HANDLE )
            {  
                *ppOut = PipelinePtr( pPipeline );
            }

            return ret;
        }

        hash_t CPipelineManager::_CalcHash(const SPipelineDesc& Desc)
        {
            //VKE_SIMPLE_PROFILE();
            hash_t hash = 0;

            Hash::Combine( &hash, Desc.hRenderPass.handle );
            Hash::Combine( &hash, Desc.hDDIRenderPass );
            Hash::Combine( &hash, Desc.hLayout.handle );
            Hash::Combine( &hash, Desc.hDDILayout );
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
                    Hash::Combine( &hash, pShader->GetHandle() );
                }
            }

#define FLOAT_TO_INT(f) (static_cast<int32_t>((f)*1000))

            {
                //blendingHash = Desc.Blending.enableLogicOperation ^ ( Desc.Blending.logicOperation << 1);
                Hash::Combine( &hash, Desc.Blending.enable );
                Hash::Combine( &hash, Desc.Blending.logicOperation );
                Hash::Combine( &hash, Desc.Blending.vBlendStates.GetCount() );

                for (uint32_t i = 0; i < Desc.Blending.vBlendStates.GetCount(); ++i)
                {
                    const auto& State = Desc.Blending.vBlendStates[i];
                    /*hash_t hash = (State.enable << 1) ^ ( State.writeMask << 1 );
                    hash ^= (State.Color.src << 1) ^ (State.Color.dst << 1) ^ (State.Color.operation << 1);
                    hash ^= (State.Alpha.src << 1) ^ (State.Alpha.dst << 1) ^ (State.Alpha.operation << 1);
                    blendingHash ^= hash << 1;*/
                    Hash::Combine( &hash, State.Alpha.dst );
                    Hash::Combine( &hash, State.Alpha.operation );
                    Hash::Combine( &hash, State.Alpha.src );
                    Hash::Combine( &hash, State.Color.dst );
                    Hash::Combine( &hash, State.Color.operation );
                    Hash::Combine( &hash, State.Color.src );
                    Hash::Combine( &hash, State.enable );
                    Hash::Combine( &hash, State.writeMask );
                }
            }
            {
                const auto& Raster = Desc.Rasterization;
                //rasterHash = FLOAT_TO_INT(Raster.Depth.biasClampFactor) ^ ( FLOAT_TO_INT(Raster.Depth.biasConstantFactor) << 1 );
                //rasterHash ^= ( FLOAT_TO_INT(Raster.Depth.biasSlopeFactor) << 1 ) ^ ( FLOAT_TO_INT(Raster.Depth.enableClamp) << 1 );
                //rasterHash ^= (Raster.Polygon.cullMode << 1) ^ (Raster.Polygon.frontFace << 1) ^ (Raster.Polygon.frontFace << 1);
                Hash::Combine( &hash, Raster.Depth.biasClampFactor );
                Hash::Combine( &hash, Raster.Depth.biasConstantFactor );
                Hash::Combine( &hash, Raster.Depth.biasClampFactor );
                Hash::Combine( &hash, Raster.Depth.enableClamp );
                Hash::Combine( &hash, Raster.Polygon.cullMode );
                Hash::Combine( &hash, Raster.Polygon.frontFace );
                Hash::Combine( &hash, Raster.Polygon.mode );
            }
            {
                //Viewport
                const auto& Viewport = Desc.Viewport;
                for( uint32_t i = 0; i < Viewport.vViewports.GetCount(); ++i )
                {
                    const auto& Curr = Viewport.vViewports[i];
                    Hash::Combine( &hash, Curr.Size.width );
                    Hash::Combine( &hash, Curr.Size.height );
                    Hash::Combine( &hash, Curr.Position.x );
                    Hash::Combine( &hash, Curr.Position.y );
                    Hash::Combine( &hash, Curr.MinMaxDepth.begin );
                    Hash::Combine( &hash, Curr.MinMaxDepth.end );
                }
                for( uint32_t i = 0; i < Viewport.vScissors.GetCount(); ++i )
                {
                    const auto& Curr = Viewport.vScissors[i];
                    Hash::Combine( &hash, Curr.Size.width );
                    Hash::Combine( &hash, Curr.Size.height );
                    Hash::Combine( &hash, Curr.Position.x );
                    Hash::Combine( &hash, Curr.Position.y );
                }
            }
            {
                // Multisampling
                const auto& MS = Desc.Multisampling;
                Hash::Combine( &hash, MS.enable );
                Hash::Combine( &hash, MS.sampleCount );
            }
            {
                const auto& DS = Desc.DepthStencil;
                {
                    const auto& Face = DS.BackFace;
                    Hash::Combine( &hash, Face.compareMask );
                    Hash::Combine( &hash, Face.compareOp );
                    Hash::Combine( &hash, Face.depthFailOp );
                    Hash::Combine( &hash, Face.failOp );
                    Hash::Combine( &hash, Face.passOp );
                    Hash::Combine( &hash, Face.reference );
                    Hash::Combine( &hash, Face.writeMask );
                }
                {
                    const auto& Face = DS.FrontFace;
                    Hash::Combine( &hash, Face.compareMask );
                    Hash::Combine( &hash, Face.compareOp );
                    Hash::Combine( &hash, Face.depthFailOp );
                    Hash::Combine( &hash, Face.failOp );
                    Hash::Combine( &hash, Face.passOp );
                    Hash::Combine( &hash, Face.reference );
                    Hash::Combine( &hash, Face.writeMask );
                }
                {
                    const auto& DB = DS.DepthBounds;
                    Hash::Combine( &hash, DB.enable );
                    Hash::Combine( &hash, DB.max );
                    Hash::Combine( &hash, DB.min );
                }
                Hash::Combine( &hash, DS.depthFunction );
                Hash::Combine( &hash, DS.enable );
                Hash::Combine( &hash, DS.enableDepthTest );
                Hash::Combine( &hash, DS.enableDepthWrite );
                Hash::Combine( &hash, DS.enableStencilTest );
                Hash::Combine( &hash, DS.enableStencilWrite );
            }
            {
                const auto& IL = Desc.InputLayout;
                for( uint32_t i = 0; i < IL.vVertexAttributes.GetCount(); ++i )
                {
                    const auto& Attr = IL.vVertexAttributes[ i ];
                    Hash::Combine( &hash, Attr.vertexBufferBindingIndex );
                    Hash::Combine( &hash, Attr.format );
                    Hash::Combine( &hash, Attr.inputRate );
                    Hash::Combine( &hash, Attr.location );
                    Hash::Combine( &hash, Attr.offset );
                    Hash::Combine( &hash, Attr.pName );
                    Hash::Combine( &hash, Attr.stride );
                }
                Hash::Combine( &hash, IL.vVertexAttributes.GetCount() );
            }

            return hash;
        }

        hash_t CPipelineManager::_CalcHash(const SPipelineLayoutDesc& Desc)
        {
            SHash Hash;
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
            CPipelineLayout* pLayout = nullptr;
            hash_t hash = _CalcHash( Desc );
            PipelineLayoutRefPtr pRef;
            if( !m_LayoutBuffer.Find( hash, &pRef ) )
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_PipelineLayoutMemMgr, &pLayout, this ) ) )
                {
                    pRef = PipelineLayoutRefPtr( pLayout );
                    if( m_LayoutBuffer.Add( hash, pRef ) )
                    {
                        
                    }
                    else
                    {
                        VKE_LOG_ERR("Unable to add CPipelineLayout object to the resource buffer.");
                        Memory::DestroyObject( &m_PipelineLayoutMemMgr, &pLayout );
                    }
                }
            }
            if( pRef.IsValid() )
            {
                pLayout = pRef.Get();
                if( pLayout->GetHandle() == NULL_HANDLE )
                {
                    DDIPipelineLayout hLayout = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
                    if( hLayout != DDI_NULL_HANDLE )
                    {
                        pLayout->Init( Desc );
                        pLayout->m_hDDIObject = hLayout;
                        pLayout->m_hObject = hash;
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to create VkPipelineLayout object." );
                        pLayout = nullptr;
                    }
                }
            }
            return PipelineLayoutRefPtr( pLayout );
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
            m_Buffer.Find( hPipeline.handle, &pRet );
            return pRet;
        }

        PipelineLayoutRefPtr CPipelineManager::GetPipelineLayout( PipelineLayoutHandle hLayout )
        {
            PipelineLayoutRefPtr pRet;
            m_LayoutBuffer.Find( hLayout.handle, &pRet );
            return pRet;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER