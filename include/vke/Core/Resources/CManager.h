#pragma once

#include "Core/VKECommon.h"
#include "Core/Resources/CResource.h"

namespace VKE
{
    namespace Resources
    {
        struct STCManagerInternal;
        class CResource;

        class VKE_API CManager
        {
            protected:
                struct SInternal;

            public:

                using ResourceType = CResource;
                using ResourceRawPtr = ResourceType*;
                using ResourceRefPtr = Utils::TCObjectSmartPtr< ResourceType >;
                using ResourcePtr = Utils::TCWeakPtr< ResourceType >;

            public:

            CManager();
            virtual ~CManager();

            Result Create();
            void Destroy();

            virtual Result CreateResources(uint32_t count, const SCreateDesc* const* ppInfos);
            virtual Result CreateResource(const SCreateDesc* const pInfo, ResourcePtr* ppOut);
            virtual Result PrepareResource(const SCreateDesc* const pInfo, ResourcePtr* ppOut);
            virtual Result LoadResource(const SCreateDesc* const pInfo, ResourcePtr** ppOut);
            virtual Result DestroyResource(ResourcePtr* ppResInOut);
            virtual Result GetResource(const handle_t& handle, ResourcePtr* pOut);
            virtual Result GetResource(cstr_t pName, uint32_t nameLen, ResourcePtr* pOut);
            Result GetResource(const vke_string& strName, ResourcePtr* pOut) { return GetResource( strName.c_str(), static_cast<uint32_t>(strName.length()), pOut ); }
            virtual Result CheckNextResourceUse();
            virtual void CheckAllResourcesUse();

            protected:

            Result _AddResource(ResourcePtr pRes);
            ResourcePtr _RemoveResource(ResourcePtr pRes);
            ResourcePtr _RemoveResource(const handle_t& handle);
            virtual ResourceRawPtr _AllocateMemory(const SCreateDesc* const);
            virtual void _FreeMemory(ResourcePtr* ppOut);
            virtual void _ResourceUnused(ResourcePtr* ppOut);

            protected:

                SInternal*  m_pInternal = nullptr;
                vke_mutex   m_mutex;
        };
    } // Resources
} // VKE