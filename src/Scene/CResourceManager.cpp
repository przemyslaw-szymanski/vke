#include "Scene/CResourceManager.h"

#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/CRenderSystem.h"

namespace VKE::World
{
    CResourceManager::CResourceManager(  )
    {
    }

    Result CResourceManager::_Create( const SResourceManagerDesc& Desc )
    {
        if( m_Desc.pDevice != Desc.pDevice )
        {
            m_Desc = Desc;
        }
        return VKE_OK;
    }

    CResourceManager::~CResourceManager()
    {
        _Destroy();
    }

    void CResourceManager::_Destroy()
    {
        if( m_Desc.pDevice != nullptr )
        {
            {
                Threads::ScopedLock l( m_ShaderSyncObj );
                m_qShaders.clear();
            }
            {
                Threads::ScopedLock l( m_PipelineSyncObj );
                m_qPipelines.clear();
            }
            {
                Threads::ScopedLock l( m_TextureSyncObj );
                m_qTextures.clear();
            }
            {
                Threads::ScopedLock l( m_BufferSyncObj );
                m_qBuffers.clear();
            }
            m_Desc.pDevice = nullptr;
        }
    }

    RenderSystem::ShaderRefPtr CResourceManager::LoadShader( const RenderSystem::SCreateShaderDesc& Desc )
    {
        RenderSystem::ShaderRefPtr pRet;
        const bool                 deferred =
            ( Desc.Create.flags & Core::CreateResourceFlags::ASYNC ) || ( Desc.Create.flags & Core::CreateResourceFlags::DEFERRED );
        if( deferred )
        {
            Threads::ScopedLock l( m_ShaderSyncObj );
            m_qShaders.push_back( Desc );
        }

        pRet = m_Desc.pDevice->CreateShader( Desc );

        return pRet;
    }

    Result CResourceManager::LoadDeferredShader()
    {
        Result ret = VKE_FAIL;

        auto Itr = m_qShaders.begin();
        while( Itr != m_qShaders.end() )
        {
            RenderSystem::SCreateShaderDesc& Desc = ( *Itr );
            Desc.Create.stages                    = Core::ResourceStages::FULL_LOAD;
            Desc.Create.flags                     = Core::CreateResourceFlags::DEFAULT;

            auto pPtr = m_Desc.pDevice->CreateShader( Desc );

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
            ( Desc.Create.flags & Core::CreateResourceFlags::ASYNC ) || ( Desc.Create.flags & Core::CreateResourceFlags::DEFERRED );
        if( deferred )
        {
            Threads::ScopedLock l( m_PipelineSyncObj );
            m_qPipelines.push_back( Desc );
        }
        pRet = m_Desc.pDevice->CreatePipeline( Desc );
        return pRet;
    }

    Result CResourceManager::CreateDeferredPipeline()
    {
        Result ret = VKE_FAIL;
        auto   Itr = m_qPipelines.begin();
        while( Itr != m_qPipelines.end() )
        {
            RenderSystem::SPipelineCreateDesc& Desc = ( *Itr );
            Desc.Create.stages                      = Core::ResourceStages::FULL_LOAD;
            Desc.Create.flags                       = Core::CreateResourceFlags::DEFAULT;
            auto pPtr = m_Desc.pDevice->CreatePipeline( Desc );
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