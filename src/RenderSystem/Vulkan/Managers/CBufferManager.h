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
            friend class CBuffer;
            friend struct BufferManagerTasks::SCreateBuffer;

            struct SFreeBufferHandle
            {
                hash_t      descHash;
                uint32_t    idx;
            };

            using BufferType = Utils::TCDynamicArray< BufferRefPtr, Config::RenderSystem::Buffer::MAX_BUFFER_COUNT >;
            using FreeBufferType = Utils::TCDynamicArray< SFreeBufferHandle, Config::RenderSystem::Buffer::MAX_BUFFER_COUNT >;
            using BufferPool = Core::TSMultimapResourceBuffer< BufferRefPtr, CBuffer* >;

            using MemMgr = Memory::CFreeListPool;
            using CreateBufferTaskPoolHelper = TaskPoolHelper< BufferManagerTasks::SCreateBuffer, 1024 >;
            using CreateBufferTaskPool = CreateBufferTaskPoolHelper::Pool;

            public:

                CBufferManager( CDeviceContext* pCtx );
                ~CBufferManager();

                Result              Create( const SBufferManagerDesc& Desc );
                void                Destroy();

                BufferRefPtr        CreateBuffer( const SBufferCreateDesc& Desc );
                void                DstroyBuffer( BufferPtr* pInOut )
                {
                    CBuffer* pBuffer = pInOut->Release();
                    _DestroyBuffer( &pBuffer );
                }

                BufferRefPtr        GetBuffer( BufferHandle hBuffer );

            protected:

                CBuffer*            _CreateBufferTask( const SBufferDesc& Desc );
                void                _DestroyBuffer( CBuffer** ppInOut );

                CBuffer*            _FindFreeBufferForReuse( const SBufferDesc& Desc );
                void                _FreeBuffer( CBuffer** ppInOut );
                void                _AddBuffer( CBuffer* pBuffer );

            protected:

                CDeviceContext*         m_pCtx;
                BufferPool              m_Buffers;
                FreeBufferType          m_FreeBuffers;
                MemMgr                  m_MemMgr;
                CreateBufferTaskPool    m_CreateBufferTaskPool;
                Threads::SyncObject     m_SyncObj;
        };
    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER