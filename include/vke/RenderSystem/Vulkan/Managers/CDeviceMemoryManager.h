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
            handle_t                        hView = NULL_HANDLE;
            bool                            bind = false;
        };

        struct SAllocateInfo
        {
            handle_t    hView = NULL_HANDLE;
            uint32_t    size;
            bool        bind = false;
        };

        

        class CDeviceMemoryManager
        {
            friend class CDeviceContext;

            using ViewVec = Utils::TCDynamicArray< CMemoryPoolView >;

            struct SPool
            {
                SMemoryAllocateData     Data;
                ViewVec                 vViews;
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
            using PoolMap = vke_hash_map< handle_t, ViewVec >;

            public:

                struct SCreateMemoryPoolDesc
                {
                    CDDI::AllocateDescs::SMemory    Memory;
                    bool                            bind = false;
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

                Result      BindMemory( const SBindMemoryInfo& Info );
                Result      UpdateMemory( const SUpdateMemoryInfo& DataInfo, const SBindMemoryInfo& BindInfo );

                template<RESOURCE_TYPE Type>
                handle_t    CreatePool( const SCreateMemoryPoolDesc& Desc )
                {
                    SPool Pool;
                    Result res;
                    SViewHandle ret = { NULL_HANDLE };
                    res = m_pCtx->_GetDDI().Allocate< Type >( Desc.Memory, nullptr, &Pool.Data );
                    if( VKE_SUCCEEDED( res ) )
                    {
                        if( Desc.bind )
                        {
                            SBindMemoryInfo Info;
                            Info.hBuffer = Desc.Memory.hBuffer;
                            Info.hImage = Desc.Memory.hImage;
                            Info.hMemory = Pool.Data.hMemory;
                            Info.offset = 0;
                            res = m_pCtx->_GetDDI().Bind< Type >( Info );
                        }
                        
                        ret.poolIdx = m_vPools.PushBack( Pool );
                    }
                    return ret.handle;
                }

                handle_t CreateView( const SViewDesc& Desc );

            protected:
                
                Result  _AllocateSpace( const SAllocateInfo& Info, CMemoryPoolView::SAllocateData* pOut );
                Result  _AllocateFromView( const SAllocateDesc& Desc, SAllocationHandle* pHandleOut,
                    SBindMemoryInfo* pBindInfoOut );

            protected:

                SDeviceMemoryManagerDesc    m_Desc;
                CDeviceContext*             m_pCtx;
                PoolVec                     m_vPools;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER