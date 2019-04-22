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

            struct SPool
            {
                SAllocateMemoryData     Data;
                CMemoryPoolView         View;
                uint32_t                sizeUsed = 0; // Total size used by all created views
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

            using PoolVec = Utils::TCDynamicArray< SPool >;
            using HandleVec = Utils::TCDynamicArray< handle_t >;
            using PoolMap = vke_hash_map< MEMORY_USAGE, HandleVec >;

            public:

                struct SCreateMemoryPoolDesc
                {
                    uint32_t    size;
                    uint32_t    alignment;
                    uint32_t    usage;
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

                handle_t    AllocateTexture( const SAllocateDesc& Desc, SBindMemoryInfo* pBindInfoOut );
                handle_t    AllocateBuffer( const SAllocateDesc& Desc, SBindMemoryInfo* pBindInfoOut );

                Result      UpdateMemory( const SUpdateMemoryInfo& DataInfo, const SBindMemoryInfo& BindInfo );
               

            protected:
                
                handle_t    _AllocateMemory( const SAllocateDesc& Desc, SBindMemoryInfo* pOut );
                handle_t    _CreatePool(const SCreateMemoryPoolDesc& Desc);
                Result      _AllocateFromPool( const SAllocateDesc& Desc, SAllocationHandle* pHandleOut,
                    SBindMemoryInfo* pBindInfoOut );

            protected:

                SDeviceMemoryManagerDesc    m_Desc;
                CDeviceContext*             m_pCtx;
                //PoolVec                     m_vPools;
                PoolMap                     m_mPoolIndices;
                PoolVec                     m_vPools;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER