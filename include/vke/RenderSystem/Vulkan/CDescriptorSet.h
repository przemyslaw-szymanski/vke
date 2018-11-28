#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#include "RenderSystem/CDeviceDriverInterface.h"

namespace VKE
{
    namespace RenderSystem
    {
        
        class VKE_API CDescriptorSetLayout : public Core::CObject
        {
            friend class CDescriptorSetManager;
            friend class CDeviceContext;

            struct SHandle
            {
                union
                {
                    hash_t      hash;
                    handle_t    value;
                };
            };

            VKE_ADD_DDI_OBJECT( DDIDescriptorSetLayout );

            public:

                CDescriptorSetLayout(CDescriptorSetManager* pMgr) : m_pMgr( pMgr ) {}
                Result  Init(const SDescriptorSetLayoutDesc& Desc);

                static hash_t CalcHash( const SDescriptorSetLayoutDesc& Desc );

            protected:

                SDescriptorSetLayoutDesc    m_Desc;
                CDescriptorSetManager*      m_pMgr;
        };

        using DescriptorSetLayoutPtr = Utils::TCWeakPtr< CDescriptorSetLayout >;
        using DescriptorSetLayoutRefPtr = Utils::TCObjectSmartPtr< CDescriptorSetLayout >;

        using DescriptorSetLayoutArray = Utils::TCDynamicArray< DescriptorSetLayoutPtr >;

        struct SDescriptorSetDesc
        {
            DescriptorSetLayoutArray    vpLayouts;
        };

        class VKE_API CDescriptorSet : public Core::CObject
        {
            friend class CDeviceContext;
            friend class CDescriptorSetManager;

            struct SHandle
            {
                union
                {
                    hash_t      hash : 61;
                    uint64_t    type : 3;
                };
                handle_t        value;
            };

            VKE_ADD_DDI_OBJECT( DDIDescriptorSet );

            public:
                CDescriptorSet(CDescriptorSetManager* pMgr) : m_pMgr( pMgr ) {}
                Result  Init(const SDescriptorSetDesc& Desc);

                static hash_t CalcHash( const SDescriptorSetDesc& Desc );

            protected:

                CDescriptorSetManager*      m_pMgr;
                DescriptorSetLayoutRefPtr   m_pLayout;
        };

        using DescriptorSetPtr = Utils::TCWeakPtr< CDescriptorSet >;
        using DescriptorSetRefPtr = Utils::TCObjectSmartPtr< CDescriptorSet >;

    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER