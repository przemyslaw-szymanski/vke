#include "Core/Managers/CResourceManager.h"

#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CRenderSystem.h"
#include "CVkEngine.h"

namespace VKE::Core
{
    CResourceManager::CResourceManager( CVkEngine& Engine ) : m_Engine{ Engine }
    {
    }

    RenderSystem::ShaderRefPtr CResourceManager::LoadShader( const RenderSystem::SCreateShaderDesc& Desc )
    {
        RenderSystem::ShaderRefPtr pRet;
        const bool                 deferred =
            ( Desc.Create.flags & CreateResourceFlags::ASYNC ) || ( Desc.Create.flags & CreateResourceFlags::DEFERRED );
        if( deferred )
        {
            Threads::ScopedLock l( m_ShaderSyncObj );
            m_qShaders.push_back( Desc );
        }

        pRet = m_Engine.GetRenderSystem()->GetDeviceContext()->CreateShader( Desc );

        return pRet;
    }

    Result CResourceManager::LoadDeferredShader()
    {
        Result ret = VKE_FAIL;

        auto Itr = m_qShaders.begin();
        while( Itr != m_qShaders.end() )
        {
            RenderSystem::SCreateShaderDesc& Desc = ( *Itr );
            Desc.Create.stages                    = ResourceStages::FULL_LOAD;
            Desc.Create.flags                     = CreateResourceFlags::DEFAULT;

            auto pPtr = m_Engine.GetRenderSystem()->GetDeviceContext()->CreateShader( Desc );

            if( pPtr!= nullptr && pPtr->IsResourceReady() )
            {
                if( Desc.Create.OnCreate )
                {
                    Desc.Create.OnCreate( &pPtr );
                }
                Threads::ScopedLock l( m_ShaderSyncObj );
                m_qShaders.erase( Itr );
                ret = VKE_OK;
                break;
            }
            Threads::ScopedLock l( m_ShaderSyncObj );
            ++Itr;
        }
        return ret;
    }

    RenderSystem::PipelineRefPtr CResourceManager::CreatePipeline( const RenderSystem::SPipelineCreateDesc& Desc )
    {
        RenderSystem::PipelineRefPtr pRet;
        const bool                   deferred =
            ( Desc.Create.flags & CreateResourceFlags::ASYNC ) || ( Desc.Create.flags & CreateResourceFlags::DEFERRED );
        if( deferred )
        {
            Threads::ScopedLock l( m_PipelineSyncObj );
            m_qPipelines.push_back( Desc );
        }
        pRet = m_Engine.GetRenderSystem()->GetDeviceContext()->CreatePipeline( Desc );
        return pRet;
    }

    Result CResourceManager::CreateDeferredPipeline()
    {
        Result ret = VKE_FAIL;
        auto   Itr = m_qPipelines.begin();
        while( Itr != m_qPipelines.end() )
        {
            RenderSystem::SPipelineCreateDesc& Desc = ( *Itr );
            Desc.Create.stages                      = ResourceStages::FULL_LOAD;
            Desc.Create.flags                       = CreateResourceFlags::DEFAULT;
            auto pPtr = m_Engine.GetRenderSystem()->GetDeviceContext()->CreatePipeline( Desc );
            if( pPtr!= nullptr && pPtr->IsResourceReady() )
            {
                if( Desc.Create.OnCreate )
                {
                    Desc.Create.OnCreate( &pPtr );
                }
                Threads::ScopedLock l( m_PipelineSyncObj );
                m_qPipelines.erase( Itr );
                ret = VKE_OK;
                break;
            }
            Threads::ScopedLock l( m_PipelineSyncObj );
            ++Itr;
        }
        return ret;
    }

} // namespace VKE::Core