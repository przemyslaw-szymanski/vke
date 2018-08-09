#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Managers/CResourceManager.h"
#include "Core/Memory/CFreeListPool.h"
#include "Core/Threads/ITask.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CBufferManager;

        struct SBufferManagerDesc
        {

        };

        struct BufferManagerTasks
        {
            struct SCreateBuffer : public Threads::ITask
            {
                CBufferManager*     pMgr = nullptr;
                CBuffer*            pBuffer = nullptr;
                SBufferCreateDesc   Desc;

                TaskState _OnStart( uint32_t tid ) override;
                void _OnGet( void** ) override;
            };
        };

        class CBufferManager
        {
            friend class CDeviceContext;
            friend class CGraphicsContext;
            friend class CComputeContext;
            friend class CTransferContext;
            friend struct BufferManagerTasks::SCreateBuffer;

            template<uint32_t COUNT>
            using TBuffers = TSResourceBuffer< CBuffer*, CBuffer*, COUNT, handle_t >;

            using VertexBuffers = TBuffers< Config::RenderSystem::Buffer::MAX_VERTEX_BUFFER_COUNT >;
            using IndexBuffers = TBuffers< Config::RenderSystem::Buffer::MAX_INDEX_BUFFER_COUNT >;
            using MemMgr = Memory::CFreeListPool;
            using CreateBufferTaskPoolHelper = TaskPoolHelper< BufferManagerTasks::SCreateBuffer, 1024 >;
            using CreateBufferTaskPool = CreateBufferTaskPoolHelper::Pool;

            public:

                CBufferManager( CDeviceContext* pCtx );
                ~CBufferManager();

                Result              Create( const SBufferManagerDesc& Desc );
                void                Destroy();

                BufferRefPtr        CreateBuffer( const SBufferCreateDesc& Desc );
                void                DstroyBuffer( BufferPtr* pInOut );

            protected:

                CBuffer*            _CreateBufferTask( const SBufferDesc& Desc );

            protected:

                CDeviceContext*         m_pCtx;
                VertexBuffers           m_VertexBuffers;
                IndexBuffers            m_IndexBuffers;
                MemMgr                  m_VertexBufferMemMgr;
                MemMgr                  m_IndexBufferMemMgr;
                CreateBufferTaskPool    m_CreateBufferTaskPool;
                Threads::SyncObject     m_SyncObj;
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER