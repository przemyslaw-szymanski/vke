#pragma once

#include "Core/Memory/CMemoryPoolManager.h"
#if VKE_VULKAN_RENDERER
#include "RenderSystem/CDeviceDriverInterface.h"

namespace VKE
{
    namespace RenderSystem
    {
        struct SAllocateDesc
        {
            CDDI::AllocateDescs::SMemory    Memory;
            handle_t                        hPool = NULL_HANDLE;
            bool                            bind = false;
        };

        struct SCreateMemoryPoolDesc
        {
            CDDI::AllocateDescs::SMemory    Memory;
            bool                            bind = false;
        };

        class CDeviceMemoryManager
        {
            friend class CDeviceContext;

            public:

                            CDeviceMemoryManager(CDeviceContext* pCtx);
                            ~CDeviceMemoryManager();

                Result      Create(const SDeviceMemoryManagerDesc& Desc);
                void        Destroy();

                Result      AllocateTexture( const SAllocateDesc& Desc );
                Result      AllocateBuffer( const SAllocateDesc& Desc );

                template<RESOURCE_TYPE Type>
                handle_t    CreatePool( const SCreateMemoryPoolDesc& Desc )
                {
                    SMemoryAllocateData Data;
                    Result res;
                    res = m_pCtx->_GetDDI().Allocate< Type >( Desc.Memory, nullptr, &Data );
                    if( VKE_SUCCEEDED( res ) )
                    {
                        CMemoryPoolManager::SPoolAllocateInfo Info;
                        Info.alignment = Data.alignment;
                        Info.memory = Data.hMemory;
                        Info.size = Data.size;
                        Info.type = Type;
                        res = m_MemoryPoolMgr.AllocatePool( Info );
                        if( VKE_SUCCEEDED( res ) )
                        {
                            if( Desc.bind )
                            {
                                SBindMemoryInfo Info;
                                Info.hBuffer = Desc.Memory.hBuffer;
                                Info.hImage = Desc.Memory.hImage;
                                Info.hMemory = Data.hMemory;
                                Info.offset = 0;
                                res = m_pCtx->_GetDDI().Bind< Type >( Info );
                            }
                        }
                        else
                        {
                            m_pCtx->_GetDDI().Free( &Data.hMemory );
                        }
                    }
                }

            protected:
                

            protected:

                SDeviceMemoryManagerDesc    m_Desc;
                CDeviceContext*             m_pCtx;
                Memory::CMemoryPoolManager  m_MemoryPoolMgr;
        };
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER