#include "CBufferManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Resources/CBuffer.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        TaskState BufferManagerTasks::SCreateBuffer::_OnStart( uint32_t tid )
        {
            VKE_ASSERT( pMgr != nullptr, "CBufferManager is not set" );
            pBuffer = pMgr->_CreateBufferTask( Desc.Buffer );
            if( Desc.Create.pfnCallback )
            {
                Desc.Create.pfnCallback( pMgr->m_pCtx, pBuffer );
            }
            return TaskStateBits::OK;
        }

        void BufferManagerTasks::SCreateBuffer::_OnGet( void** ppOut )
        {
            VKE_ASSERT( ppOut != nullptr && *ppOut != nullptr, "Task output mut not be null." );
        }

        CBufferManager::CBufferManager(CDeviceContext* pCtx) :
            m_pCtx( pCtx )
        {}

        CBufferManager::~CBufferManager()
        {
            Destroy();
        }

        void CBufferManager::Destroy()
        {
            //m_VertexBufferMemMgr.Destroy();
            //m_IndexBufferMemMgr.Destroy();
            m_MemMgr.Destroy();
        }

        Result CBufferManager::Create( const SBufferManagerDesc& Desc )
        {
            Result ret = VKE_FAIL;
            const auto bufferSize = sizeof( CBuffer );
            ret = m_MemMgr.Create( Config::RenderSystem::Buffer::MAX_BUFFER_COUNT, bufferSize, 1 );
            if( VKE_FAILED( ret ) )
            {
                goto ERR;
            }

            return ret;
        ERR:
            Destroy();
            return ret;
        }

        BufferRefPtr CBufferManager::CreateBuffer( const SCreateBufferDesc& Desc )
        {
            BufferRefPtr pRet;

            if( Desc.Create.async )
            {
                BufferManagerTasks::SCreateBuffer* pTask;
                {
                    Threads::ScopedLock l( m_SyncObj );
                    pTask = CreateBufferTaskPoolHelper::GetTask( &m_CreateBufferTaskPool );
                }
                pTask->pMgr = this;
                pTask->Desc = Desc;
                m_pCtx->_AddTask( pTask );
            }
            else
            {
                pRet = _CreateBufferTask( Desc.Buffer );
            }
            return pRet;
        }

        /*VertexBufferRefPtr CBufferManager::CreateBuffer( const SCreateVertexBufferDesc& Desc )
        {
            BufferRefPtr pRet;

            if( Desc.Create.async )
            {
                BufferManagerTasks::SCreateBuffer* pTask;
                {
                    Threads::ScopedLock l( m_SyncObj );
                    pTask = CreateBufferTaskPoolHelper::GetTask( &m_CreateBufferTaskPool );
                }
                pTask->pMgr = this;
                pTask->Desc = Desc;
                m_pCtx->_AddTask( pTask );
            }
            else
            {
                pRet = _CreateBufferTask( Desc.Buffer );
            }
            return pRet;
        }*/

        void CBufferManager::DestroyBuffer( BufferPtr* pInOut )
        {
            CBuffer* pBuffer = (*pInOut).Release();
            _FreeBuffer( &pBuffer );
        }

        Result CBufferManager::UpdateBuffer( const SUpdateMemoryInfo& Info, CBuffer** ppInOut )
        {
            Result ret = VKE_FAIL;
            CBuffer* pBuffer = *ppInOut;
            auto& MemMgr = m_pCtx->_GetDeviceMemoryManager();
            ret = MemMgr.BindMemory( pBuffer->m_BindInfo );
            if( VKE_SUCCEEDED( ret ) )
            {
                ret = MemMgr.UpdateMemory( Info, pBuffer->m_BindInfo );
            }
            return ret;
        }

        void CBufferManager::_DestroyBuffer( CBuffer** ppInOut )
        {
            CBuffer* pBuffer = *ppInOut;
            const handle_t hBuffer = pBuffer->GetHandle();
            auto hDDIObj = pBuffer->GetDDIObject();
            m_pCtx->_GetDDI().DestroyObject( &hDDIObj, nullptr );
            Memory::DestroyObject( &m_MemMgr, &pBuffer );
        }

        void CBufferManager::_FreeBuffer( CBuffer** ppInOut )
        {

        }

        void CBufferManager::_AddBuffer( CBuffer* pBuffer )
        {

        }

        CBuffer* CBufferManager::_FindFreeBufferForReuse( const SBufferDesc& Desc )
        {
            const hash_t descHash = CBuffer::CalcHash( Desc );
            CBuffer* pBuffer = nullptr;
            m_Buffers.FindFree( descHash, &pBuffer );
            return pBuffer;
        }

        CBuffer* CBufferManager::_CreateBufferTask( const SBufferDesc& Desc )
        {
            // Find this buffer in the resource buffer
            CBuffer* pBuffer = _FindFreeBufferForReuse( Desc );
            if( pBuffer == nullptr )
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_MemMgr, &pBuffer, this ) ) )
                {
                    if( VKE_SUCCEEDED( pBuffer->Init( Desc ) ) )
                    {
                        _AddBuffer( pBuffer );
                    }
                }
            }
            else
            {
                
            }
            if( pBuffer->GetDDIObject() == DDI_NULL_HANDLE )
            {
                pBuffer->m_BindInfo.hBuffer = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
                if( pBuffer->m_BindInfo.hBuffer != DDI_NULL_HANDLE )
                {
                    // Create memory for buffer
                    handle_t hMemory = _AllocateMemory( Desc, &pBuffer->m_BindInfo );
                    if( hMemory != NULL_HANDLE )
                    {
                        pBuffer->m_hMemory = hMemory;
                    }
                    else
                    {
                        _FreeBuffer( &pBuffer );
                    }
                }
                else
                {
                    _FreeBuffer( &pBuffer );
                }
            }
            return pBuffer;
        }

        //CVertexBuffer* CBufferManager::_CreateBufferTask( const SVertexBufferDesc& Desc )
        //{
        //    // Find this buffer in the resource buffer
        //    CVertexBuffer* pBuffer = _FindFreeBufferForReuse( Desc );
        //    if( pBuffer == nullptr )
        //    {
        //        if( VKE_SUCCEEDED( Memory::CreateObject( &m_MemMgr, &pBuffer, this ) ) )
        //        {
        //            if( VKE_SUCCEEDED( pBuffer->Init( Desc ) ) )
        //            {
        //                _AddBuffer( pBuffer );
        //            }
        //        }
        //    }
        //    else
        //    {

        //    }
        //    if( pBuffer->GetDDIObject() == DDI_NULL_HANDLE )
        //    {
        //        pBuffer->m_BindInfo.hBuffer = m_pCtx->_GetDDI().CreateObject( Desc, nullptr );
        //        if( pBuffer->m_BindInfo.hBuffer != DDI_NULL_HANDLE )
        //        {
        //            // Create memory for buffer
        //            handle_t hMemory = _AllocateMemory( Desc, &pBuffer->m_BindInfo );
        //            if( hMemory != NULL_HANDLE )
        //            {
        //                pBuffer->m_hMemory = hMemory;
        //            }
        //            else
        //            {
        //                _FreeBuffer( &pBuffer );
        //            }
        //        }
        //        else
        //        {
        //            _FreeBuffer( &pBuffer );
        //        }
        //    }
        //    return pBuffer;
        //}

        BufferRefPtr CBufferManager::GetBuffer( BufferHandle hBuffer )
        {
            BufferRefPtr pBuffer;
            m_Buffers.Find( hBuffer.handle, &pBuffer );
            return ( pBuffer );
        }

        handle_t CBufferManager::_AllocateMemory( const SBufferDesc& Desc, SBindMemoryInfo* pBindInfoInOut )
        {
            handle_t ret = 0;
            handle_t hMemoryView = 0;
            CDeviceMemoryManager& MemMgr = m_pCtx->_GetDeviceMemoryManager();

            if( Desc.memoryUsage & MemoryUsages::SEPARATE_ALLOCATION == 0 )
            {
                auto Itr = m_mMemViews.find( Desc.memoryUsage );
                if( Itr != m_mMemViews.end() )
                {
                    hMemoryView = Itr->second;
                }
                else
                {
                    CDeviceMemoryManager::SCreateMemoryPoolDesc PoolDesc;
                    PoolDesc.bind = false;
                    PoolDesc.Memory.hBuffer = pBindInfoInOut->hBuffer;
                    PoolDesc.Memory.memoryUsages = Desc.memoryUsage;
                    PoolDesc.Memory.size = 1024;
                    ret = MemMgr.CreatePool< ResourceTypes::BUFFER >( PoolDesc );
                    if( ret != NULL_HANDLE )
                    {
                        CDeviceMemoryManager::SViewDesc MemViewDesc;
                        MemViewDesc.hPool = ret;
                        MemViewDesc.size = PoolDesc.Memory.size;
                        hMemoryView = MemMgr.CreateView( MemViewDesc );
                        if( hMemoryView != NULL_HANDLE )
                        {
                            m_mMemViews[Desc.memoryUsage] = hMemoryView;
                        }
                    }
                }
            }

            SAllocateDesc AllocDesc;
            AllocDesc.bind = false;
            AllocDesc.hView = hMemoryView;
            AllocDesc.Memory.hBuffer = pBindInfoInOut->hBuffer;
            AllocDesc.Memory.memoryUsages = Desc.memoryUsage;
            AllocDesc.Memory.size = Desc.size;
            ret = MemMgr.AllocateBuffer( AllocDesc, pBindInfoInOut );
            
            return ret;
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER