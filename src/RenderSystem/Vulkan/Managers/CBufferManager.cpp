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
            const handle_t hBuffer = pBuffer->GetHandle();
            m_Buffers.Free( hBuffer );
            _DestroyBuffer( &pBuffer );
        }

        Result CBufferManager::UpdateBuffer( const SUpdateMemoryInfo& Info, CContextBase* pBaseCtx, CBuffer** ppInOut )
        {
            Result ret = VKE_FAIL;
            CBuffer* pDstBuffer = *ppInOut;
            auto& MemMgr = m_pCtx->_GetDeviceMemoryManager();
            {
                // For Uniform buffers use N-buffering mode
                uint32_t dstOffset = 0;
                if( pDstBuffer->m_Desc.chunkCount > 1 )
                {
                    dstOffset = Info.dstDataOffset;
                }

                if( pDstBuffer->m_Desc.memoryUsage & MemoryUsages::GPU_ACCESS )
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
                            //MemMgr.UpdateMemory( Info, BindInfo );
                        }
                        

                        if( pMemory )
                        {
                            CCommandBuffer* pCmdBuffer = pBaseCtx->_CreateCommandBuffer();
                            pCmdBuffer->Begin();

                            

                            SCopyBufferInfo CopyInfo;
                            CopyInfo.hDDISrcBuffer = Data.pBuffer->GetDDIObject();
                            CopyInfo.hDDIDstBuffer = pDstBuffer->GetDDIObject();
                            CopyInfo.Region.size = MapInfo.size;
                            CopyInfo.Region.srcBufferOffset = MapInfo.offset;
                            CopyInfo.Region.dstBufferOffset = dstOffset;
                            SBufferBarrierInfo BarrierInfo;
                            BarrierInfo.hDDIBuffer = pDstBuffer->GetDDIObject();
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
                    /// @TODO this lock is here because validation layer trhwos an error
                    Threads::SyncObject l( m_SyncObj );
                    ret = MemMgr.UpdateMemory( Info, pDstBuffer->m_BindInfo );
                }
            }
            return ret;
        }

        void CBufferManager::_DestroyBuffer( CBuffer** ppInOut )
        {
            CBuffer* pBuffer = *ppInOut;
            
            auto& hDDIObj = pBuffer->m_BindInfo.hDDIBuffer;
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
            
            if( pBuffer == nullptr )
            {
                if( VKE_SUCCEEDED( Memory::CreateObject( &m_MemMgr, &pBuffer, this ) ) )
                {
                    if( VKE_SUCCEEDED( pBuffer->Init( Desc ) ) )
                    {
                        pBuffer->m_hObject = m_Buffers.Add( BufferRefPtr( pBuffer ) );
                    }
                }
            }
            else
            {
                
            }
            if( pBuffer->GetDDIObject() == DDI_NULL_HANDLE )
            {
                bool constantBuffer = false;
                if( Desc.usage & BufferUsages::UNIFORM_BUFFER ||
                    Desc.usage & BufferUsages::UNIFORM_TEXEL_BUFFER ||
                    Desc.chunkCount > 1 )
                {
                    constantBuffer = true;
                    pBuffer->m_Desc.chunkCount = 2;
                }

                m_pCtx->DDI().UpdateDesc( &pBuffer->m_Desc );
                pBuffer->m_chunkSize = pBuffer->m_Desc.size;
                pBuffer->m_Desc.size *= pBuffer->m_Desc.chunkCount;

                pBuffer->m_BindInfo.hDDIBuffer = m_pCtx->_GetDDI().CreateObject( pBuffer->m_Desc, nullptr );
                if( pBuffer->m_BindInfo.hDDIBuffer != DDI_NULL_HANDLE )
                {
                    // Create memory for buffer
                    SAllocateDesc AllocDesc;
                    AllocDesc.Memory.hDDIBuffer = pBuffer->GetDDIObject();
                    AllocDesc.Memory.memoryUsages = Desc.memoryUsage;
                    AllocDesc.Memory.size = pBuffer->m_Desc.size;
                    AllocDesc.poolSize = VKE_MEGABYTES( 10 );
                    handle_t hMemory = m_pCtx->_GetDeviceMemoryManager().AllocateBuffer( AllocDesc, &pBuffer->m_BindInfo );
                    if( hMemory != NULL_HANDLE )
                    {
                        m_vConstantBuffers.PushBack( pBuffer );
                    }
                }
            }
            return pBuffer;
        ERR:
            _DestroyBuffer( &pBuffer );
            return pBuffer;
        }

        BufferRefPtr CBufferManager::GetBuffer( BufferHandle hBuffer )
        {
            return m_Buffers[hBuffer.handle];
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