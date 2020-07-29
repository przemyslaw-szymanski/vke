#pragma once

#include "Core/Memory/CMemoryPoolManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDDI.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SAllocateDesc
        {
            CDDI::AllocateDescs::SMemory    Memory;
            uint32_t                        poolSize = 0;
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

            protected:

                handle_t    _AllocateMemory( const SAllocateDesc& Desc, SBindMemoryInfo* pOut );
                handle_t    _CreatePool(const SCreateMemoryPoolDesc& Desc);
                handle_t    _AllocateFromPool( const SAllocateDesc& Desc, const SAllocationMemoryRequirementInfo& MemReq,
                    SBindMemoryInfo* pBindInfoOut );

            protected:

                SDeviceMemoryManagerDesc    m_Desc;
                CDeviceContext*             m_pCtx;
                //PoolVec                     m_vPools;
                PoolMap                     m_mPoolIndices;
                PoolBuffer                  m_PoolBuffer;
                AllocationBuffer            m_AllocBuffer;
                SyncObjVec                  m_vSyncObjects;
                PoolViewVec                 m_vPoolViews;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER