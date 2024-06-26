#pragma once
#if VKE_VULKAN_RENDER_SYSTEM
#include "Core/Managers/CResourceManager.h"
#include "Core/Memory/CFreeListPool.h"
#include "Core/Threads/ITask.h"
#include "RenderSystem/Resources//CBuffer.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CBufferManager;
        class CStagingBufferManager;
        class CContextBase;

        struct SBufferManagerDesc
        {

        };

        struct BufferManagerTasks
        {
            struct SCreateBuffer : public Threads::ITask
            {
                CBufferManager*     pMgr = nullptr;
                CBuffer*            pBuffer = nullptr;
                SCreateBufferDesc   Desc;

                TaskState _OnStart( uint32_t tid ) override;
                void _OnGet( void** ) override;
            };
        };

        class VKE_API CConstantBuffer
        {
            public:

            protected:
        };

        class CBufferManager
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CTransferContext;
            friend class CBuffer;
            friend struct BufferManagerTasks::SCreateBuffer;
            friend class CContextBase;

            struct SFreeBufferHandle
            {
                hash_t      descHash;
                uint32_t    idx;
            };

            struct SUpdateBufferInfo
            {
                handle_t    hStagingBuffer;
                uint8_t*    pDeviceMemory; // mapped device memory
                uint32_t    sizeUsed = 0;
                uint32_t    size;
                uint32_t    offset;
                handle_t    hMemory;
                DDIBuffer   hDDIBuffer;
            };

            using BufferBuffer = Utils::TSFreePool< CBuffer*, uint32_t, 1 >;

            using UpdateBufferInfoArray = Utils::TCDynamicArray< SUpdateBufferInfo >;
            using MemMgr = Memory::CFreeListPool;
            using CreateBufferTaskPoolHelper = TaskPoolHelper< BufferManagerTasks::SCreateBuffer, 1024 >;
            using CreateBufferTaskPool = CreateBufferTaskPoolHelper::Pool;
            using BufferArray = Utils::TCDynamicArray< CBuffer* >;

            using MemoryViewMap = vke_hash_map< BUFFER_USAGE, handle_t >;

            public:

                CBufferManager( CDeviceContext* pCtx );
                ~CBufferManager();

                Result              Create( const SBufferManagerDesc& Desc );
                void                Destroy();

                BufferHandle        CreateBuffer( const SCreateBufferDesc& Desc );
                BufferRefPtr        GetBuffer( const BufferHandle& hBuffer );
                //VertexBufferRefPtr  CreateBuffer( const SCreateVertexBufferDesc& Desc );
                void                DestroyBuffer( BufferHandle* phInOut );
                void                DestroyBuffer( BufferPtr* pInOut );
                Result              UpdateBuffer( CommandBufferPtr pCmdBuffer, const SUpdateMemoryInfo& Info, CBuffer** ppInOut );

                Result              UploadMemoryToStagingBuffer(const SUpdateMemoryInfo& Info, SStagingBufferInfo* pOut);

                BufferRefPtr        GetBuffer( const VertexBufferHandle& hBuffer );
                BufferRefPtr        GetBuffer( const IndexBufferHandle& hBuffer );
                void*               LockMemory( uint32_t offset, uint32_t size, handle_t* phMemory );
                void                UnlockMemory( handle_t* phMemory );

                uint32_t            LockStagingBuffer(const uint32_t maxSize);
                Result              UpdateStagingBufferMemory( const SUpdateStagingBufferInfo& Info );
                Result              UnlockStagingBuffer(CContextBase* pCtx, const SUnlockBufferInfo& Info);

                void                FreeUnusedAllocations();

                CStagingBufferManager*  GetStagingBufferManager() const { return m_pStagingBufferMgr; }

            protected:

                CBuffer*            _CreateBufferTask( const SBufferDesc& Desc );
                //CVertexBuffer*      _CreateBuffertask( const SVertexBufferDesc& Desc );
                void                _DestroyBuffer( CBuffer** ppInOut );

                CBuffer*            _FindFreeBufferForReuse( const SBufferDesc& Desc );
                void                _AddBuffer( CBuffer* pBuffer );

                Result              _GetStagingBuffer(const SUpdateMemoryInfo& Info, const CDeviceContext* pCtx,
                    handle_t* phInOut, SStagingBufferInfo* pOut, CCommandBuffer** ppTransferCmdBufferOut);

            protected:

                CDeviceContext*         m_pCtx;
                BufferBuffer            m_Buffers;
                MemMgr                  m_MemMgr;
                CreateBufferTaskPool    m_CreateBufferTaskPool;
                Threads::SyncObject     m_SyncObj;
                //Threads::SyncObject     m_StagingBuffSyncObj;
                std::mutex              m_mutex;
                MemoryViewMap           m_mMemViews;
                CStagingBufferManager*  m_pStagingBufferMgr;
                BufferArray             m_vConstantBuffers;
                Threads::SyncObject     m_MapMemSyncObj;
                Threads::SyncObject     m_vUpdateBufferInfoSyncObj;
                UpdateBufferInfoArray   m_vUpdateBufferInfos;
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDER_SYSTEM