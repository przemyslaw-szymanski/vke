#pragma once
#if VKE_VULKAN_RENDERER
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

            using BufferBuffer = Utils::TSFreePool< BufferRefPtr, uint32_t, 1 >;

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

                BufferRefPtr        CreateBuffer( const SCreateBufferDesc& Desc );
                //VertexBufferRefPtr  CreateBuffer( const SCreateVertexBufferDesc& Desc );
                void                DestroyBuffer( BufferPtr* pInOut );
                Result              UpdateBuffer( const SUpdateMemoryInfo& Info, CContextBase* pCtx, CBuffer** ppInOut );

                BufferRefPtr        GetBuffer( BufferHandle hBuffer );
                BufferRefPtr        GetBuffer( const VertexBufferHandle& hBuffer );
                BufferRefPtr        GetBuffer( const IndexBufferHandle& hBuffer );
                Result              LockMemory( const uint32_t size, BufferPtr* ppBuffer, SBindMemoryInfo* pOut );
                void                UnlockMemory( BufferPtr* ppBuffer );

                void                FreeUnusedAllocations();

            protected:

                CBuffer*            _CreateBufferTask( const SBufferDesc& Desc );
                //CVertexBuffer*      _CreateBuffertask( const SVertexBufferDesc& Desc );
                void                _DestroyBuffer( CBuffer** ppInOut );

                CBuffer*            _FindFreeBufferForReuse( const SBufferDesc& Desc );
                void                _AddBuffer( CBuffer* pBuffer );
    
            protected:

                CDeviceContext*         m_pCtx;
                BufferBuffer            m_Buffers;
                MemMgr                  m_MemMgr;
                CreateBufferTaskPool    m_CreateBufferTaskPool;
                Threads::SyncObject     m_SyncObj;
                std::mutex              m_mutex;
                MemoryViewMap           m_mMemViews;
                CStagingBufferManager*  m_pStagingBufferMgr;
                BufferArray             m_vConstantBuffers;
                Threads::SyncObject     m_MapMemSyncObj;
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER