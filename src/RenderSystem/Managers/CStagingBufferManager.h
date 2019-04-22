#pragma once

#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CBuffer.h"
#include "Core/VKEConfig.h"

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
                BufferPtr   pBuffer;
                handle_t    hMemoryView;
            };

            using BufferArray = Utils::TCDynamicArray< BufferRefPtr >;
            using MemViewArray = Utils::TCDynamicArray< handle_t >;

            public:

                struct SBufferRequirementInfo
                {
                    CDeviceContext* pCtx;
                    uint32_t        size;
                };

                struct SBufferData
                {
                    BufferPtr   pBuffer;
                    //hddi
                    uint32_t    offset;
                };

            public:

                Result  Create( const SStagingBufferManagerDesc& Desc );
                void    Destroy();

                Result  GetBuffer( const SBufferRequirementInfo& Info, SBufferData* pData );

            protected:

                uint32_t    _FindBuffer( const SBufferRequirementInfo& Info );
                Result      _CreateBuffer( const SBufferRequirementInfo& Info, SBufferData* pData );

            protected:

                SStagingBufferManagerDesc   m_Desc;
                BufferArray                 m_vpBuffers;
                MemViewArray                m_vMemViews;
        };
    }
}