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
        TaskState BufferManagerTasks::SCreateBuffer::_OnStart( uint32_t )
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

                for( uint32_t i = 0; i < m_Buffers.vPool.GetCount(); ++i )
                {
                    auto& pBuffer = m_Buffers.vPool[ i ];
                    CBuffer* pBuff = pBuffer.Release();
                    _DestroyBuffer( &pBuff );
                    pBuffer = nullptr;
                }

                m_Buffers.Clear();
                
                m_MemMgr.Destroy();
            }
        }

        Result CBufferManager::Create( const SBufferManagerDesc& )
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

            if( Desc.Create.async == true )
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
            const handle_t hBuffer = pBuffer->GetHandle();
            m_Buffers.Free( static_cast< uint32_t >( hBuffer ) );
            _DestroyBuffer( &pBuffer );
        }

        Result CBufferManager::UpdateBuffer( const SUpdateMemoryInfo& Info, CContextBase* pBaseCtx, CBuffer** ppInOut )
        {
            VKE_ASSERT( ppInOut != nullptr && *ppInOut != nullptr, "" );
            Result ret = VKE_FAIL;
            CBuffer* pDstBuffer = *ppInOut;
            auto& MemMgr = m_pCtx->_GetDeviceMemoryManager();
            {
                if( pDstBuffer->m_Desc.memoryUsage & MemoryUsages::GPU_ACCESS )
                {
                    CStagingBufferManager::SBufferRequirementInfo ReqInfo;
                    ReqInfo.pCtx = m_pCtx;
                    ReqInfo.Requirements.alignment = 1;
                    ReqInfo.Requirements.size = Info.dataSize;
                    CStagingBufferManager::SBufferData* pData;
                    ret = m_pStagingBufferMgr->GetBuffer( ReqInfo, &pData );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        SUpdateMemoryInfo StagingBufferInfo;
                        StagingBufferInfo.dataSize = pData->size;
                        StagingBufferInfo.dstDataOffset = pData->offset;
                        StagingBufferInfo.pData = Info.pData;

                        if( VKE_SUCCEEDED( MemMgr.UpdateMemory( StagingBufferInfo, pData->pBuffer->m_hMemory ) ) )
                        {
                            CCommandBuffer* pCmdBuffer = pBaseCtx->GetTransferContext()->GetCommandBuffer();

                            SCopyBufferInfo CopyInfo;
                            CopyInfo.hDDISrcBuffer = pData->pBuffer->GetDDIObject();
                            CopyInfo.hDDIDstBuffer = pDstBuffer->GetDDIObject();
                            CopyInfo.Region.size = pData->size;
                            CopyInfo.Region.srcBufferOffset = pData->offset;
                            CopyInfo.Region.dstBufferOffset = Info.dstDataOffset;
                            SBufferBarrierInfo BarrierInfo;
                            BarrierInfo.hDDIBuffer = pDstBuffer->GetDDIObject();
                            BarrierInfo.size = CopyInfo.Region.size;
                            BarrierInfo.offset = Info.dstDataOffset;
                            BarrierInfo.srcMemoryAccess = MemoryAccessTypes::DATA_TRANSFER_READ;
                            BarrierInfo.dstMemoryAccess = MemoryAccessTypes::DATA_TRANSFER_WRITE;
                            pCmdBuffer->Barrier( BarrierInfo );
                            pCmdBuffer->Copy( CopyInfo );
                            pCmdBuffer->End( CommandBufferEndFlags::EXECUTE | CommandBufferEndFlags::PUSH_SIGNAL_SEMAPHORE, nullptr );
                            pData->pCommandBuffer = pCmdBuffer;

                            BarrierInfo.srcMemoryAccess = BarrierInfo.dstMemoryAccess;
                            BarrierInfo.dstMemoryAccess = MemoryAccessTypes::VERTEX_ATTRIBUTE_READ;
                            pBaseCtx->GetCommandBuffer()->Barrier( BarrierInfo );
                        }
                        else
                        {
                            m_pStagingBufferMgr->FreeBuffer( &pData );
                            ret = VKE_ENOMEMORY;
                        }
                    }
                }
                else
                {
                    ret = MemMgr.UpdateMemory( Info, pDstBuffer->m_hMemory );
                }
            }
            return ret;
        }

        void CBufferManager::_DestroyBuffer( CBuffer** ppInOut )
        {
            CBuffer* pBuffer = *ppInOut;
            
            auto& hDDIObj = pBuffer->m_hDDIObject;
            m_pCtx->_GetDDI().DestroyObject( &hDDIObj, nullptr );
            pBuffer->_Destroy();
            Memory::DestroyObject( &m_MemMgr, ppInOut );
            *ppInOut = nullptr;
        }

        CBuffer* CBufferManager::_CreateBufferTask( const SBufferDesc& Desc )
        {
            // Find this buffer in the resource buffer
            //const hash_t descHash = CBuffer::CalcHash( Desc );
            CBuffer* pBuffer = nullptr;
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_MemMgr, &pBuffer, this ) ) )
                {
                    if( VKE_SUCCEEDED( pBuffer->Init( Desc ) ) )
                    {
                        pBuffer->m_hObject = m_Buffers.Add( BufferRefPtr( pBuffer ) );
                    }
                }
                else
                {
                    VKE_LOG_ERR( "Unable to create memory for CBuffer object." );
                    goto ERR;
                }
            }

            if( pBuffer->GetDDIObject() == DDI_NULL_HANDLE )
            {
                pBuffer->m_hDDIObject = m_pCtx->_GetDDI().CreateObject( pBuffer->m_Desc, nullptr );
                if( pBuffer->m_hDDIObject != DDI_NULL_HANDLE )
                {
                    // Create memory for buffer
                    SAllocateDesc AllocDesc;
                    AllocDesc.Memory.hDDIBuffer = pBuffer->GetDDIObject();
                    AllocDesc.Memory.memoryUsages = Desc.memoryUsage;
                    AllocDesc.Memory.size = pBuffer->m_Desc.size;
                    AllocDesc.poolSize = VKE_MEGABYTES( 10 );
                    pBuffer->m_hMemory = m_pCtx->_GetDeviceMemoryManager().AllocateBuffer( AllocDesc );
                }
                else
                {
                    goto ERR;
                }
            }
            return pBuffer;
        ERR:
            _DestroyBuffer( &pBuffer );
            return pBuffer;
        }

        BufferRefPtr CBufferManager::GetBuffer( BufferHandle hBuffer )
        {
            return m_Buffers[ static_cast< uint32_t >( hBuffer.handle ) ];
        }

        BufferRefPtr CBufferManager::GetBuffer( const VertexBufferHandle& hBuffer )
        {
            return m_Buffers[ static_cast<uint16_t>( hBuffer.handle ) ];
        }

        BufferRefPtr CBufferManager::GetBuffer( const IndexBufferHandle& hBuffer )
        {
            return m_Buffers[ static_cast<uint16_t>( hBuffer.handle ) ];
        }

        Result CBufferManager::LockMemory( const uint32_t size, BufferPtr* ppBuffer, SBindMemoryInfo* )
        {
            Result ret = VKE_FAIL;
            
            return ret;
        }

        void CBufferManager::UnlockMemory( BufferPtr* )
        {

        }

        void CBufferManager::FreeUnusedAllocations()
        {
            m_pStagingBufferMgr->FreeUnusedAllocations( m_pCtx );
        }

    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER