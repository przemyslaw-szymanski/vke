#include "CBufferManager.h"
#include "RenderSystem/CTransferContext.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/Vulkan/Resources/CBuffer.h"
#include "RenderSystem/CDeviceContext.h"
#include "RenderSystem/Vulkan/Managers/CDeviceMemoryManager.h"
#include "RenderSystem/Managers/CStagingBufferManager.h"

#include "Scene/Terrain/CTerrainVertexFetchRenderer.h"

#define VKE_LOG_BUFFER_MANAGER 0

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

                for( uint32_t i = 1; i < m_Buffers.vPool.GetCount(); ++i )
                {
                    auto& pBuffer = m_Buffers.vPool[ i ];
                    _DestroyBuffer( &pBuffer );
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
            SStagingBufferManagerDesc StagingDesc;
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

            
            /// @TODO init this value
            //StagingDesc.bufferSize
            ret = m_pStagingBufferMgr->Create( StagingDesc );
            if( VKE_FAILED( ret ) )
            {
                goto ERR;
            }

            // Add null handle
            m_Buffers.Add( nullptr );

            return ret;
        ERR:
            Destroy();
            return ret;
        }

        BufferHandle CBufferManager::CreateBuffer( const SCreateBufferDesc& Desc )
        {
            BufferHandle hRet = INVALID_HANDLE;
            BufferRefPtr pRet;

            if( (Desc.Create.flags & Core::CreateResourceFlags::ASYNC) == Core::CreateResourceFlags::ASYNC )
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
                hRet= pRet->GetHandle();
            }
            return hRet;
        }

        BufferRefPtr CBufferManager::GetBuffer( const BufferHandle& hBuffer )
        {
            return BufferRefPtr{ m_Buffers[hBuffer.handle] };
        }

        BufferRefPtr CBufferManager::GetBuffer( const VertexBufferHandle& hBuffer )
        {
            return BufferRefPtr{ m_Buffers[(hBuffer.handle)] };
        }

        BufferRefPtr CBufferManager::GetBuffer( const IndexBufferHandle& hBuffer )
        {
            return BufferRefPtr{ m_Buffers[(hBuffer.handle)] };
        }

        void CBufferManager::DestroyBuffer( BufferHandle* phInOut )
        {
            auto& hBuff = *phInOut;
            BufferPtr pBuffer = GetBuffer( hBuff );
            DestroyBuffer( &pBuffer );
            hBuff = INVALID_HANDLE;
        }

        void CBufferManager::DestroyBuffer( BufferPtr* pInOut )
        {
            CBuffer* pBuffer = (*pInOut).Release();
            const auto hBuffer = pBuffer->GetHandle().handle;
            m_Buffers.Free( ( hBuffer ) );
            _DestroyBuffer( &pBuffer );
        }

        Result CBufferManager::_GetStagingBuffer( const SUpdateMemoryInfo& Info, const CDeviceContext* pDevice,
                                                  handle_t* phInOut, SStagingBufferInfo* pOut,
            CCommandBuffer** ppTransferCmdBufferOut)
        {
            Result ret = VKE_ENOMEMORY;

            CStagingBufferManager::SBufferRequirementInfo ReqInfo;
            ReqInfo.pCtx = m_pCtx;
            ReqInfo.Requirements.alignment = 1;
            ReqInfo.Requirements.size = Info.dataSize;

            //Threads::ScopedLock l( m_SyncObj );
            auto pTransferCtx = pDevice->GetTransferContext();
            //pTransferCtx->Lock();
            //m_StagingBuffSyncObj.Lock();
            auto pTransferCmdBuffer = pTransferCtx->GetCommandBuffer();
            VKE_ASSERT( pTransferCmdBuffer->GetState() != CCommandBuffer::States::END, "" );
            handle_t hStagingBuffer = pTransferCmdBuffer->GetLastUsedStagingBufferAllocation();
            ret = m_pStagingBufferMgr->GetBuffer(ReqInfo, Info.flags, &hStagingBuffer, pOut);
            //m_StagingBuffSyncObj.Unlock();
            //pTransferCtx->Unlock();
            if (ret == VKE_ENOMEMORY && (Info.flags == StagingBufferFlags::OUT_OF_SPACE_FLUSH_AND_WAIT ))
            {
                VKE_LOG_WARN("No memory in staging buffer. Requested size: " << VKE_LOG_MEM_SIZE(Info.dataSize));
                pTransferCtx->Execute<ExecuteCommandBufferFlags::WAIT | ExecuteCommandBufferFlags::DONT_SIGNAL_SEMAPHORE>(false);
                ret = _GetStagingBuffer( Info, pDevice, phInOut, pOut, ppTransferCmdBufferOut );
            }
            else
            {
                //pTransferCtx->Lock();
                //m_StagingBuffSyncObj.Lock();
                pTransferCmdBuffer->UpdateStagingBufferAllocation( hStagingBuffer );
                //m_StagingBuffSyncObj.Unlock();
                *ppTransferCmdBufferOut = pTransferCmdBuffer.Get();
                VKE_ASSERT( pTransferCmdBuffer->GetState() != CCommandBuffer::States::END, "" );
                *phInOut = hStagingBuffer;
                //pTransferCtx->Unlock();
#if( VKE_LOG_BUFFER_MANAGER )
                VKE_LOG( "Allocation for cmd buffer: " << pTransferCmdBuffer );
#endif
            }
            
            return ret;
        }

        Result CBufferManager::UploadMemoryToStagingBuffer( const SUpdateMemoryInfo& Info, SStagingBufferInfo* pOut )
        {
            Result ret = VKE_FAIL;
            handle_t hStagingBuffer;
            CCommandBuffer* pTransferCmdBuffer;
            //auto pDeviceCtx = pCtx->GetDeviceContext();
            ret = _GetStagingBuffer(Info, m_pCtx, &hStagingBuffer, pOut, &pTransferCmdBuffer);
            if (VKE_SUCCEEDED(ret))
            {
                //pTransferCmdBuffer->AddStagingBufferAllocation(hStagingBuffer);

                SUpdateMemoryInfo StagingBufferInfo;
                StagingBufferInfo.dataSize = Info.dataSize;
                StagingBufferInfo.dstDataOffset = pOut->offset;
                StagingBufferInfo.pData = Info.pData;

                auto& MemMgr = m_pCtx->_GetDeviceMemoryManager();
                ret = MemMgr.UpdateMemory(StagingBufferInfo, pOut->hMemory);
            }
            return ret;
        }

        Result CBufferManager::UpdateBuffer( CommandBufferPtr pCmdbuffer, const SUpdateMemoryInfo& Info,
                                             CBuffer** ppInOut )
        {
            VKE_ASSERT( ppInOut != nullptr && *ppInOut != nullptr, "" );
            Result ret = VKE_FAIL;
            CBuffer* pDstBuffer = *ppInOut;
            auto& MemMgr = m_pCtx->_GetDeviceMemoryManager();
            {
                if( (pDstBuffer->m_Desc.memoryUsage & MemoryUsages::GPU_ACCESS) == MemoryUsages::GPU_ACCESS )
                {
                    CStagingBufferManager::SBufferRequirementInfo ReqInfo;
                    ReqInfo.pCtx = m_pCtx;
                    ReqInfo.Requirements.alignment = 1;
                    ReqInfo.Requirements.size = Info.dataSize;
                    //CStagingBufferManager::SBufferData* pData;
                    /*CCommandBuffer* pTransferCmdBuffer = pBaseCtx->GetTransferContext()->GetCommandBuffer();
                    handle_t hStagingBuffer = pTransferCmdBuffer->GetLastUsedStagingBufferAllocation();
                    SStagingBufferInfo Data;
                    ret = m_pStagingBufferMgr->GetBuffer( ReqInfo, Info.flags, &hStagingBuffer, &Data );*/
                    handle_t hStagingBuffer;
                    CCommandBuffer* pTransferCmdBuffer;
                    SStagingBufferInfo Data;
                    m_pCtx->GetTransferContext()->Lock();
                    ret = _GetStagingBuffer( Info, m_pCtx, &hStagingBuffer, &Data, &pTransferCmdBuffer );
                    if( VKE_SUCCEEDED( ret ) )
                    {
                        SUpdateMemoryInfo StagingBufferInfo;
                        StagingBufferInfo.dataSize = Info.dataSize;
                        StagingBufferInfo.dstDataOffset = Data.offset;
                        StagingBufferInfo.pData = Info.pData;
                        if( VKE_SUCCEEDED( MemMgr.UpdateMemory( StagingBufferInfo, Data.hMemory ) ) )
                        {
                            VKE_RENDER_SYSTEM_BEGIN_DEBUG_INFO( pTransferCmdBuffer, Info );
                            SCopyBufferInfo CopyInfo;
                            CopyInfo.hDDISrcBuffer = Data.hDDIBuffer;
                            CopyInfo.hDDIDstBuffer = pDstBuffer->GetDDIObject();
                            CopyInfo.Region.size = Info.dataSize;
                            CopyInfo.Region.srcBufferOffset = Data.offset;
                            CopyInfo.Region.dstBufferOffset = Info.dstDataOffset;
                            SBufferBarrierInfo BarrierInfo;
                            BarrierInfo.hDDIBuffer = pDstBuffer->GetDDIObject();
                            BarrierInfo.size = CopyInfo.Region.size;
                            BarrierInfo.offset = Info.dstDataOffset;
                            BarrierInfo.srcMemoryAccess = MemoryAccessTypes::DATA_TRANSFER_READ;
                            BarrierInfo.dstMemoryAccess = MemoryAccessTypes::DATA_TRANSFER_WRITE;
                            pTransferCmdBuffer->Barrier( BarrierInfo );
                            pTransferCmdBuffer->Copy( CopyInfo );
                            //pCmdBuffer->End( ExecuteCommandBufferFlags::EXECUTE | ExecuteCommandBufferFlags::PUSH_SIGNAL_SEMAPHORE, nullptr );
                            //pData->pCommandBuffer = pTransferCmdBuffer;

                            BarrierInfo.srcMemoryAccess = BarrierInfo.dstMemoryAccess;
                            //BarrierInfo.dstMemoryAccess = MemoryAccessTypes::VERTEX_ATTRIBUTE_READ;
                            BarrierInfo.dstMemoryAccess = MemoryAccessTypes::GPU_MEMORY_READ;
                            pTransferCmdBuffer->Barrier( BarrierInfo );
                            VKE_RENDER_SYSTEM_END_DEBUG_INFO( pTransferCmdBuffer );
                        }
                        else
                        {
                            //m_pStagingBufferMgr->FreeBuffer( &pData );
                            ret = VKE_ENOMEMORY;
                        }
                    }
                    m_pCtx->GetTransferContext()->Unlock();
                }
                else
                {
                    ret = MemMgr.UpdateMemory( Info, pDstBuffer->m_hMemory );
                }
            }
            return ret;
        }

        uint32_t CBufferManager::LockStagingBuffer(const uint32_t maxSize)
        {
            uint32_t ret = INVALID_HANDLE;
            auto pTransferCtx = m_pCtx->GetTransferContext();
            //VKE_ASSERT( pTransferCtx->IsLocked() == false, "" );
            pTransferCtx->Lock();
            auto pTransferCmdBuffer = pTransferCtx->GetCommandBuffer();

            handle_t hStagingBuffer = pTransferCmdBuffer->GetLastUsedStagingBufferAllocation();
            SStagingBufferInfo Data;
            CStagingBufferManager::SBufferRequirementInfo ReqInfo;
            ReqInfo.pCtx = m_pCtx;
            ReqInfo.Requirements.alignment = 1;
            ReqInfo.Requirements.size = maxSize;

            m_pStagingBufferMgr->GetBuffer( ReqInfo, 0, &hStagingBuffer, &Data );
            {
                pTransferCmdBuffer->AddStagingBufferAllocation( hStagingBuffer );
            }
            SUpdateMemoryInfo StagingBufferInfo;
            StagingBufferInfo.dataSize = maxSize;
            StagingBufferInfo.dstDataOffset = Data.offset;
            StagingBufferInfo.pData = nullptr;

            auto& MemMgr = m_pCtx->_GetDeviceMemoryManager();
            void* pMem = MemMgr.MapMemory( StagingBufferInfo, Data.hMemory );
            if( pMem != nullptr )
            {
                memset(pMem, 1, maxSize);
                Threads::ScopedLock l(m_vUpdateBufferInfoSyncObj);
                SUpdateBufferInfo Info;
                Info.hStagingBuffer = hStagingBuffer;
                Info.pDeviceMemory = (uint8_t*)pMem;
                Info.size = Data.alignedSize;
                Info.offset = Data.offset;
                Info.hDDIBuffer = Data.hDDIBuffer;
                Info.hMemory = Data.hMemory;
                ret = m_vUpdateBufferInfos.PushBack(Info);
            }
            //pTransferCtx->Unlock();
            return ret;
        }

        Result CBufferManager::UpdateStagingBufferMemory( const SUpdateStagingBufferInfo& UpdateInfo )
        {
            Result ret = VKE_ENOMEMORY;
            // Check if there is a free space in current chunk
            auto& Info = m_vUpdateBufferInfos[ UpdateInfo.hLockedStagingBuffer ];
            const bool canUpdate = Info.sizeUsed + UpdateInfo.dataAlignedSize <= Info.size - UpdateInfo.dataAlignedSize;
            if( canUpdate )
            {
                void* pDst = Info.pDeviceMemory + UpdateInfo.stagingBufferOffset;
                Info.sizeUsed += UpdateInfo.dataAlignedSize;

                Memory::Copy( pDst, Info.size - Info.sizeUsed, UpdateInfo.pSrcData, UpdateInfo.dataSize );
                ret = VKE_OK;
            }
            return ret;
        }

        Result CBufferManager::UnlockStagingBuffer(CContextBase* pCtx, const SUnlockBufferInfo& UnlockInfo)
        {
            Result ret = VKE_OK;
            auto& Info = m_vUpdateBufferInfos[UnlockInfo.hUpdateInfo ];
            auto pTransferCtx = m_pCtx->GetTransferContext();
            VKE_ASSERT( pTransferCtx->IsLocked(), "" );
            //pTransferCtx->Lock();
            auto pTransferCmdBuffer = pTransferCtx->GetCommandBuffer();
            VKE_RENDER_SYSTEM_BEGIN_DEBUG_INFO( pTransferCmdBuffer, UnlockInfo);
            uint32_t sizeUsed = Math::Max( UnlockInfo.totalSize, Info.sizeUsed );
            m_pStagingBufferMgr->_UpdateBufferInfo(Info.hStagingBuffer, sizeUsed);

            auto& MemMgr = m_pCtx->_GetDeviceMemoryManager();
            MemMgr.UnmapMemory(Info.hMemory);
            /*{
                const auto* p = (Scene::CTerrainVertexFetchRenderer::SPerDrawConstantBufferData*)Info.pDeviceMemory;
                p = p;
            }*/
            VKE_ASSERT(UnlockInfo.pDstBuffer != nullptr, "");
            const auto& hDDIDstBuffer = UnlockInfo.pDstBuffer->GetDDIObject();
            SCopyBufferInfo CopyInfo;
            CopyInfo.hDDISrcBuffer = Info.hDDIBuffer;
            CopyInfo.hDDIDstBuffer = hDDIDstBuffer;
            CopyInfo.Region.size = sizeUsed;
            CopyInfo.Region.srcBufferOffset = Info.offset;
            CopyInfo.Region.dstBufferOffset = UnlockInfo.dstBufferOffset;
            SBufferBarrierInfo BarrierInfo;
            BarrierInfo.hDDIBuffer = hDDIDstBuffer;
            BarrierInfo.size = CopyInfo.Region.size;
            BarrierInfo.offset = UnlockInfo.dstBufferOffset;
            BarrierInfo.srcMemoryAccess = MemoryAccessTypes::DATA_TRANSFER_READ;
            BarrierInfo.dstMemoryAccess = MemoryAccessTypes::DATA_TRANSFER_WRITE;
            pTransferCmdBuffer->Barrier( BarrierInfo );
            pTransferCmdBuffer->Copy( CopyInfo );

            VKE_RENDER_SYSTEM_END_DEBUG_INFO(pTransferCmdBuffer);

            auto pCmdBuffer = pCtx->GetCommandBuffer();
            VKE_RENDER_SYSTEM_BEGIN_DEBUG_INFO( pCmdBuffer, UnlockInfo );
            BarrierInfo.srcMemoryAccess = BarrierInfo.dstMemoryAccess;
            BarrierInfo.dstMemoryAccess = MemoryAccessTypes::VERTEX_ATTRIBUTE_READ;
            pCmdBuffer->Barrier(BarrierInfo);
            VKE_RENDER_SYSTEM_END_DEBUG_INFO( pTransferCmdBuffer );

            m_vUpdateBufferInfos.RemoveFast( UnlockInfo.hUpdateInfo );
            pTransferCtx->Unlock();
            return ret;
        }

        void CBufferManager::_DestroyBuffer( CBuffer** ppInOut )
        {
            CBuffer* pBuffer = *ppInOut;
            auto& hDDIObj = pBuffer->m_hDDIObject;
            m_pCtx->_GetDDI().DestroyBuffer( &hDDIObj, nullptr );
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
                    pBuffer->m_hObject.handle = m_Buffers.Add( pBuffer );
                    if( pBuffer->m_hObject != INVALID_HANDLE )
                    {
                        if( VKE_FAILED( pBuffer->Init( Desc ) ) )
                        {
                            goto ERR;
                        }
                    }
                    else
                    {
                        VKE_LOG_ERR( "Unable to add CBuffer to the Buffer pool." );
                        goto ERR;
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
                pBuffer->m_hDDIObject = m_pCtx->_GetDDI().CreateBuffer( pBuffer->m_Desc, nullptr );
                if( pBuffer->m_hDDIObject != DDI_NULL_HANDLE )
                {
                    // Create memory for buffer
                    SAllocateDesc AllocDesc;
                    AllocDesc.Memory.hDDIBuffer = pBuffer->GetDDIObject();
                    AllocDesc.Memory.memoryUsages = Desc.memoryUsage;
                    AllocDesc.Memory.size = pBuffer->m_Desc.size;
                    //AllocDesc.poolSize = 0; // set 0 for default
                    pBuffer->m_hMemory = m_pCtx->_GetDeviceMemoryManager().AllocateBuffer( AllocDesc );
                    if( pBuffer->m_hMemory == INVALID_HANDLE )
                    {
                        goto ERR;
                    }
                    //if( Desc.pData != nullptr )
                    //{
                    //    SUpdateMemoryInfo UpdateInfo;
                    //    UpdateInfo.pData = Desc.pData;
                    //    UpdateInfo.dstDataOffset = 0;
                    //    UpdateInfo.dataSize = Desc.dataSize;
                    //    BufferPtr pTmp = BufferPtr{ pBuffer };
                    //    //m_pCtx->UpdateBuffer( UpdateInfo, &pTmp );
                    //    UpdateBuffer(UpdateInfo, m_p)
                    //}
                }
                else
                {
                    goto ERR;
                }
            }
            return pBuffer ;
        ERR:
            _DestroyBuffer( &pBuffer );
            return pBuffer;
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
#endif // VKE_VULKAN_RENDER_SYSTEM