#pragma once

#include "Core/Memory/CMemoryPoolManager.h"
#if VKE_VULKAN_RENDER_SYSTEM
#include "RenderSystem/CDDI.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SAllocateDesc
        {
            CDDI::AllocateDescs::SMemory    Memory;
            uint32_t                        poolSize = 0; /// 0 for default settings
#if VKE_RENDER_SYSTEM_MEMORY_DEBUG
            union
            {
                const STextureDesc* pTexDesc;
                const SBufferDesc* pBufferDesc;
            };
            uint32_t descType = 0; // 1 tex, 2 buff, 0 undefined
            void SetDebugInfo(const STextureDesc* pDesc)
            {
                pTexDesc = pDesc;
                descType = 1;
            }
            void SetDebugInfo( const SBufferDesc* pDesc )
            {
                pBufferDesc = pDesc;
                descType = 2;
            }
#endif
            void SetDebugInfo( const void* ){}
        };

        class CDeviceMemoryManager
        {
            friend class CDeviceContext;

            using ViewVec = Utils::TCDynamicArray< CMemoryPoolView >;

            using AllocationHandleType = uint32_t;
            using PoolHandleType = uint16_t;

            struct SPool
            {
                SAllocateMemoryData     Data;
            };

            struct SViewHandle
            {
                union
                {
                    struct
                    {
                        uint32_t    poolIdx : 16;
                        uint32_t    viewIdx : 16;
                    };
                    uint32_t        handle;
                };
            };

            union UAllocationHandle
            {
                struct
                {
                    AllocationHandleType    hAllocInfo;
                    PoolHandleType          hPool;
                    uint32_t                dedicated : 1;
                };
                handle_t    handle;

                UAllocationHandle() {}
                UAllocationHandle( const handle_t& h ) : handle{ h } {}
                void operator=( const handle_t& h ) { handle = h; }
            };

            using PoolVec = Utils::TCDynamicArray< SPool >;
            using PoolBuffer = Utils::TSFreePool< SPool >;
            using SyncObjVec = Utils::TCDynamicArray< Threads::SyncObject >;
            using PoolViewVec = Utils::TCDynamicArray< CMemoryPoolView >;
            using AllocationBuffer = Utils::TSFreePool< SMemoryAllocationInfo >;
            using HandleVec = Utils::TCDynamicArray< handle_t >;
            using PoolMap = vke_hash_map< MEMORY_USAGE, HandleVec >;
            using PoolSizeMap = vke_hash_map< MEMORY_USAGE, uint32_t >;

            public:

                struct SCreateMemoryPoolDesc
                {
                    uint32_t        size;
                    uint16_t        alignment;
                    MEMORY_USAGE    usage;
                };

                struct SViewDesc
                {
                    handle_t    hPool;
                    uint32_t    size;
                };

                struct SAllocationHandle
                {
                    union
                    {
                        struct
                        {
                            SViewHandle     hView;
                            uint32_t        offset;
                        };
                        handle_t    handle;
                    };
                };

            public:

                            CDeviceMemoryManager(CDeviceContext* pCtx);
                            ~CDeviceMemoryManager();

                Result      Create(const SDeviceMemoryManagerDesc& Desc);
                void        Destroy();

                handle_t    AllocateTexture( const SAllocateDesc& Desc );
                handle_t    AllocateBuffer( const SAllocateDesc& Desc );

                Result      UpdateMemory( const SUpdateMemoryInfo& DataInfo, const SBindMemoryInfo& BindInfo );
                Result      UpdateMemory( const SUpdateMemoryInfo& DataInfo, const handle_t& hMemory );

                void*       MapMemory(const SUpdateMemoryInfo& DataInfo, const handle_t& hMemory);
                void        UnmapMemory(const handle_t& hMemory);

                const SMemoryAllocationInfo& GetAllocationInfo( const handle_t& hMemory );

                void LogDebug();

            protected:

                handle_t    _AllocateMemory( const SAllocateDesc& Desc, SBindMemoryInfo* pOut );
                handle_t    _CreatePool(const SCreateMemoryPoolDesc& Desc);
                handle_t    _CreatePool(const SAllocateDesc& Desc, const SAllocationMemoryRequirementInfo& MemReq);
                handle_t    _AllocateFromPool( const SAllocateDesc& Desc, const SAllocationMemoryRequirementInfo& MemReq,
                    SBindMemoryInfo* pBindInfoOut );

            protected:

                SDeviceMemoryManagerDesc    m_Desc;
                CDeviceContext*             m_pCtx;
                //PoolVec                     m_vPools;
                PoolMap                     m_mPoolIndices;
                PoolSizeMap                 m_mLastPoolSizes;
                PoolBuffer                  m_PoolBuffer;
                AllocationBuffer            m_AllocBuffer;
                SyncObjVec                  m_vSyncObjects;
                PoolViewVec                 m_vPoolViews;
                uint32_t                    m_maxPoolCount;
                uint32_t m_aMaxPoolCounts[MemoryHeapTypes::_MAX_COUNT];
                size_t m_aMinAllocSizes[MemoryHeapTypes::_MAX_COUNT];
                size_t m_aHeapSizes[ MemoryHeapTypes::_MAX_COUNT ];
                //uint32_t                    m_lastPoolSize = 0;
                //size_t                      m_totalMemAllocated = 0;
                size_t m_aTotalMemAllocated[ MemoryHeapTypes::_MAX_COUNT ] = { 0 };
                size_t m_aTotalMemUsed[ MemoryHeapTypes::_MAX_COUNT ] = { 0 };
                //size_t                      m_totalMemUsed = 0;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDER_SYSTEM