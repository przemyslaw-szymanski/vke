#pragma once
#if VKE_VULKAN_RENDERER
#include "Core/CObject.h"
#include "RenderSystem/CDeviceDriverInterface.h"
#include "RenderSystem/Resources/CResource.h"

namespace VKE
{
    namespace RenderSystem
    {

        class VKE_API CBuffer : public VKE::Resources::CResource
        {
            friend class CBufferManager;
            VKE_ADD_OBJECT_MEMBERS

            public:

                CBuffer( CBufferManager* pMgr );
                ~CBuffer();

                Result Init( const SBufferDesc& Desc );
                void Destroy();

                const DDIBuffer& GetDDIObject() const { return m_DDIObject; }

                static hash_t CalcHash( const SBufferDesc& Desc );

            protected:

                void _Destroy();

            protected:

                SBufferDesc         m_Desc;
                CBufferManager*     m_pMgr;
                DDIBuffer           m_DDIObject = DDINullHandle;
        };
        using BufferPtr = Utils::TCWeakPtr< CBuffer >;
        using BufferRefPtr = Utils::TCObjectSmartPtr< CBuffer >;
    } // RenderSystem
} // VKE
#endif // VKE_VULKAN_RENDERER