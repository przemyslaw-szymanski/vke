#include "CBufferManager.h"
#include "RenderSystem/CTransferContext.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Vulkan/Resources/CBuffer.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"
#include "RenderSystem/Managers/CStagingBufferManager.h"

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
            if( m_pStagingBufferMgr != nullptr )
            {
                m_pStagingBufferMgr->Destroy( m_pCtx );
                Memory::DestroyObject( &HeapAllocator, &m_pStagingBufferMgr );
                m_pStagingBufferMgr = nullptr;

                for( auto& Itr : m_Buffers.Resources.Container )
                {
                    for( auto& Itr2 : Itr.second )
                    {
                        CBuffer* pBuffer = Itr2.Release();
                        pBuffer->_Destroy();
                    }
                }

                m_Buffers.Clear();
                
                m_MemMgr.Destroy();
            }
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

            ret = Memory::CreateObject( &HeapAllocator, &m_pStagingBufferMgr );
            if( VKE_FAILED( ret ) )
            {
                goto ERR;
            }

            SStagingBufferManagerDesc StagingDesc;
            /// @TODO init this value
            //StagingDesc.bufferSize
            ret = m_pStagingBufferMgr->Create( StagingDesc );
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

        void CBufferManager::DestroyBuffer( BufferPtr* pInOut )
        {
            CBuffer* pBuffer = (*pInOut).Release();
            _FreeBuffer( &pBuffer );
        }

        Result CBufferManager::UpdateBuffer( const SUpdateMemoryInfo& Info, CContextBase* pBaseCtx, CBuffer** ppInOut )
        {
            Result ret = VKE_FAIL;
            CBuffer* pBuffer = *ppInOut;
            auto& MemMgr = m_pCtx->_GetDeviceMemoryManager();
            {
                if( pBuffer->m_Desc.memoryUsage & MemoryUsages::GPU_ACCESS )
                {
                    CStagingBufferManager::SBufferRequirementInfo ReqInfo;
                    ReqInfo.pCtx = m_pCtx;
                    ReqInfo.Requirements.alignment = 1;
                    ReqInfo.Requirements.size = Info.dataSize;
                    CStagingBufferManager::SBufferData Data;
                    ret = m_pStagingBufferMgr->GetBuffer( ReqInfo, &Data );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        SMapMemoryInfo MapInfo;
                        MapInfo.hMemory = Data.pBuffer->m_BindInfo.hDDIMemory;
                        MapInfo.offset = Data.offset;
                        MapInfo.size = Data.size;
                        void* pMemory = nullptr;

                        {
                            Threads::ScopedLock l( m_MapMemSyncObj );
                            pMemory = m_pCtx->DDI().MapMemory( MapInfo );
                            if( pMemory )
                            {
                                Memory::Copy( pMemory, Data.size, Info.pData, Info.dataSize );
                                m_pCtx->DDI().UnmapMemory( MapInfo.hMemory );
                            }
                        }
                        if( pMemory )
                        {
                            CCommandBuffer* pCmdBuffer = pBaseCtx->_CreateCommandBuffer();
                            pCmdBuffer->Begin();

                            // For Uniform buffers use N-buffering mode
                            uint32_t dstOffset = 0;
                            if( pBuffer->m_Desc.backBuffering )
                            {
                                dstOffset += pBaseCtx->GetBackBufferIndex() * pBuffer->m_Desc.size;
                            }

                            SCopyBufferInfo CopyInfo;
                            CopyInfo.hDDISrcBuffer = Data.pBuffer->GetDDIObject();
                            CopyInfo.hDDIDstBuffer = pBuffer->GetDDIObject();
                            CopyInfo.Region.size = MapInfo.size;
                            CopyInfo.Region.srcBufferOffset = MapInfo.offset;
                            CopyInfo.Region.dstBufferOffset = dstOffset;
                            SBufferBarrierInfo BarrierInfo;
                            BarrierInfo.hDDIBuffer = pBuffer->GetDDIObject();
                            BarrierInfo.size = CopyInfo.Region.size;
                            BarrierInfo.offset = dstOffset;
                            BarrierInfo.srcMemoryAccess = MemoryAccessTypes::DATA_TRANSFER_READ;
                            BarrierInfo.dstMemoryAccess = MemoryAccessTypes::DATA_TRANSFER_WRITE;
                            pCmdBuffer->Barrier( BarrierInfo );
                            pCmdBuffer->Copy( CopyInfo );
                            BarrierInfo.srcMemoryAccess = BarrierInfo.dstMemoryAccess;
                            BarrierInfo.dstMemoryAccess = MemoryAccessTypes::VERTEX_ATTRIBUTE_READ;
                            pCmdBuffer->Barrier( BarrierInfo );

                            pCmdBuffer->End( CommandBufferEndFlags::EXECUTE | CommandBufferEndFlags::DONT_SIGNAL_SEMAPHORE );
                        }
                        else
                        {
                            m_pStagingBufferMgr->FreeBuffer( Data );
                            ret = VKE_ENOMEMORY;
                        }
                    }
                }
                else
                {
                    ret = MemMgr.UpdateMemory( Info, pBuffer->m_BindInfo );
                }
            }
            return ret;
        }

        void CBufferManager::_DestroyBuffer( CBuffer** ppInOut )
        {
            CBuffer* pBuffer = *ppInOut;
            const handle_t hBuffer = pBuffer->GetHandle();
            auto& hDDIObj = pBuffer->m_BindInfo.hDDIBuffer;
            m_pCtx->_GetDDI().DestroyObject( &hDDIObj, nullptr );
            Memory::DestroyObject( &m_MemMgr, &pBuffer );
        }

        void CBufferManager::_FreeBuffer( CBuffer** ppInOut )
        {
            CBuffer* pBuffer = *ppInOut;
            DDIBuffer hDDIBuffer = pBuffer->GetDDIObject();
            m_pCtx->DDI().DestroyObject( &hDDIBuffer, nullptr );
            m_Buffers.AddFree( pBuffer->m_hObject, pBuffer );
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
            const hash_t descHash = CBuffer::CalcHash( Desc );
            CBuffer* pBuffer = nullptr;
            m_Buffers.FindFree( descHash, &pBuffer );
            if( pBuffer == nullptr )
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_MemMgr, &pBuffer, this ) ) )
                {
                    if( VKE_SUCCEEDED( pBuffer->Init( Desc ) ) )
                    {
                        pBuffer->m_hObject = descHash;
                        m_Buffers.Add( descHash, BufferRefPtr( pBuffer ) );
                    }
                }
            }
            else
            {
                
            }
            if( pBuffer->GetDDIObject() == DDI_NULL_HANDLE )
            {
                uint32_t bufferCount = 1;
                if( Desc.usage & BufferUsages::UNIFORM_BUFFER ||
                    Desc.usage & BufferUsages::UNIFORM_TEXEL_BUFFER ||
                    Desc.backBuffering )
                {
                    pBuffer->m_Desc.backBuffering = true;
                    bufferCount = 2;
                }
                pBuffer->m_Desc.size *= bufferCount;

                pBuffer->m_BindInfo.hDDIBuffer = m_pCtx->_GetDDI().CreateObject( pBuffer->m_Desc, nullptr );
                if( pBuffer->m_BindInfo.hDDIBuffer != DDI_NULL_HANDLE )
                {
                    // Create memory for buffer
                    SAllocateDesc AllocDesc;
                    AllocDesc.Memory.hDDIBuffer = pBuffer->GetDDIObject();
                    AllocDesc.Memory.memoryUsages = Desc.memoryUsage;
                    AllocDesc.Memory.size = Desc.size;
                    AllocDesc.poolSize = VKE_MEGABYTES( 10 );
                    handle_t hMemory = m_pCtx->_GetDeviceMemoryManager().AllocateBuffer( AllocDesc, &pBuffer->m_BindInfo );
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

        BufferRefPtr CBufferManager::GetBuffer( BufferHandle hBuffer )
        {
            BufferRefPtr pBuffer;
            m_Buffers.Find( hBuffer.handle, &pBuffer );
            return ( pBuffer );
        }

        Result CBufferManager::LockMemory( const uint32_t size, BufferPtr* ppBuffer, SBindMemoryInfo* pOut )
        {
            Result ret = VKE_FAIL;
            CBuffer* pBuffer = (*ppBuffer).Get();
            const auto& BindInfo = pBuffer->m_BindInfo;
            CDeviceMemoryManager& MemMgr = m_pCtx->_GetDeviceMemoryManager();
            
            return ret;
        }

        void CBufferManager::UnlockMemory( BufferPtr* ppBuffer )
        {

        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER