#pragma once
#include "Core/CObject.h"
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {

        class VKE_API CBuffer : public VKE::Core::CObject
        {
            friend class CBufferManager;
            VKE_ADD_OBJECT_MEMBERS

            public:

                CBuffer( CBufferManager* pMgr );
                ~CBuffer();

                Result Init( const SBufferDesc& Desc );
                void Destroy();

                const VkBuffer& GetNative() const { return m_vkBuffer; }

            protected:

                void _Destroy();

            protected:

                SBufferDesc         m_Desc;
                CBufferManager*     m_pMgr;
                VkBuffer            m_vkBuffer = VK_NULL_HANDLE;
        };
        //using BufferPtr = Utils::TCWeakPtr< Resources::CBuffer >;
        //using BufferRefPtr = Utils::TCObjectSmartPtr< Resources::CBuffer >;
    } // RenderSystem
} // VKE