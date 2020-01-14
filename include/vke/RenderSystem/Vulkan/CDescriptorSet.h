#pragma once
#if VKE_VULKAN_RENDERER
#include "RenderSystem/Common.h"
#include "Core/Utils/TCSmartPtr.h"
#include "RenderSystem/CDDI.h"

namespace VKE
{
    namespace RenderSystem
    {
        
        class VKE_API CDescriptorSetLayout
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
            VKE_DECL_BASE_OBJECT( DescriptorSetLayoutHandle );

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

        using DescriptorSetLayoutArray = Utils::TCDynamicArray< DescriptorSetLayoutHandle >;

        struct SDescriptorSetDesc
        {
            DescriptorSetLayoutArray    vLayouts;
        };

        class VKE_API CDescriptorSet
        {
            friend class CDeviceContext;
            friend class CDescriptorSetManager;

            struct SHandle
            {
                union
                {
                    uint64_t    hash : 61;
                    uint64_t    type : 3;
                };
                handle_t        value;
            };

            VKE_ADD_DDI_OBJECT( DDIDescriptorSet );
            VKE_DECL_BASE_OBJECT( DescriptorSetHandle );

            public:
                CDescriptorSet(CDescriptorSetManager* pMgr) : m_pMgr( pMgr ) {}
                Result  Init(const SDescriptorSetDesc& Desc);

                static hash_t CalcHash( const SDescriptorSetDesc& Desc );

            protected:

                CDescriptorSetManager*      m_pMgr;
                DescriptorSetLayoutRefPtr   m_pLayout;
        };

        struct VKE_API SDescriptorSet
        {
            DDIDescriptorSet            hDDISet;
            handle_t                    hPool;
            DescriptorSetLayoutHandle   hSetLayout;
        };

        using DescriptorSetPtr = Utils::TCWeakPtr< CDescriptorSet >;
        using DescriptorSetRefPtr = Utils::TCObjectSmartPtr< CDescriptorSet >;

    } // RenderSystem
} // VKE

#endif // VKE_VULKAN_RENDERER