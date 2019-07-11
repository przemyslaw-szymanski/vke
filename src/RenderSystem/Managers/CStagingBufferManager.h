#pragma once

#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "Core/VKEConfig.h"
#include "Core/Memory/CMemoryPoolManager.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;

        struct SStagingBufferManagerDesc
        {
            uint32_t    bufferSize = VKE_MEGABYTES( 1 );
        };

        class CStagingBufferManager
        {
            struct SBuffer
            {
                BufferPtr       pBuffer;
            };

            using BufferArray = Utils::TCDynamicArray< BufferRefPtr >;
            using MemViewArray = Utils::TCDynamicArray< CMemoryPoolView >;

            public:

                struct SBufferRequirementInfo
                {
                    CDeviceContext*                 pCtx;
                    SAllocationMemoryRequirements   Requirements;
                };

                struct SBufferData
                {
                    CommandBufferPtr    pCommandBuffer;
                    BufferPtr           pBuffer;
                    uint32_t            size;
                    uint32_t            offset;
                    uint32_t            handle;
                };

                using BufferDataArray = Utils::TCDynamicArray< SBufferData >;

            public:

                Result  Create( const SStagingBufferManagerDesc& Desc );
                void    Destroy(CDeviceContext* pCtx);

                Result  GetBuffer( const SBufferRequirementInfo& Info, SBufferData** ppData );
                void    FreeBuffer( SBufferData** ppInOut );

                void    FreeUnusedAllocations( CDeviceContext* pCtx );

                void    DefragmentMemory();

            protected:

                uint32_t    _FindBuffer( const SBufferRequirementInfo& Info );
                Result      _CreateBuffer( const SBufferRequirementInfo& Info, SBufferData* pData );

            protected:

                SStagingBufferManagerDesc   m_Desc;
                Threads::SyncObject         m_MemViewSyncObj;
                MemViewArray                m_vMemViews;
                BufferArray                 m_vpBuffers;
                BufferDataArray             m_vUsedData;
        };
    }
}