#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#include "RenderSystem/Vulkan/Vulkan.h"

namespace VKE
{
    namespace RenderSystem
    {
        
        class VKE_API CDescriptorSetLayout : public Core::CObject
        {
            friend class CDescriptorSetManager;
            friend class CDeviceContext;

            public:

                CDescriptorSetLayout(CDescriptorSetManager* pMgr) : m_pMgr( pMgr ) {}
                Result  Init(const SDescriptorSetLayoutDesc& Desc);

                const VkDescriptorSetLayout&    GetNative() const { return m_vkDescriptorSetLayout; }

            protected:

                SDescriptorSetLayoutDesc    m_Desc;
                CDescriptorSetManager*      m_pMgr;
                VkDescriptorSetLayout       m_vkDescriptorSetLayout = VK_NULL_HANDLE;
        };

        using DescriptorSetLayoutPtr = Utils::TCWeakPtr< CDescriptorSetLayout >;
        using DescriptorSetLayoutRefPtr = Utils::TCObjectSmartPtr< CDescriptorSetLayout >;

        struct SDescriptorSetDesc
        {
            DESCRIPTOR_SET_TYPE     type;
            DescriptorSetLayoutPtr pLayout;
        };

        class VKE_API CDescriptorSet : public Core::CObject
        {
            friend class CDeviceContext;
            friend class CDescriptorSetManager;

            public:
                CDescriptorSet(CDescriptorSetManager* pMgr) : m_pMgr( pMgr ) {}
                Result  Init(const SDescriptorSetDesc& Desc);

                const VkDescriptorSet&  GetNative() const { return m_vkDescriptorSet; }

            protected:

                CDescriptorSetManager*      m_pMgr;
                DescriptorSetLayoutRefPtr   m_pLayout;
                VkDescriptorSet             m_vkDescriptorSet = VK_NULL_HANDLE;
        };

        using DescriptorSetPtr = Utils::TCWeakPtr< CDescriptorSet >;
        using DescriptorSetRefPtr = Utils::TCObjectSmartPtr< CDescriptorSet >;

    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER