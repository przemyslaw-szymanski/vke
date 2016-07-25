#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/TCSmartPtr.h"
#include "CManager.h"

namespace VKE
{
    namespace Resource
    {
        struct STCManagerInternal;

        template<
            typename _T_,
            typename _RES_RAW_PTR_ = _T_*,
            typename _RES_REF_PTR_ = Utils::TCObjectSmartPtr< _T_ >,
            typename _RES_WEAK_PTR_ = Utils::TCWeakPtr< _T_ > >
        class VKE_API TCManager : public CManager
        {
            protected:
                struct SInternal;

            public:

                using ResourceType = _T_;
                using ResourceRawPtr = _RES_RAW_PTR_;
                using ResourceRefPtr = _RES_REF_PTR_;
                using ResourcePtr = _RES_WEAK_PTR_;
                using AllocFunc = std::function< ResourceRawPtr(const void* const) >;

            public:

            TCManager() {}
            virtual ~TCManager() {}

            ResourcePtr CreateResource(const SCreateInfo* const pInfo)
            {
                CManager::ResourcePtr pRes;
                CManager::CreateResource(pInfo, &pRes);
                return ptr_reinterpret_cast< ResourceType >(pRes);
            }

            Result PrepareResource(const SCreateInfo* const pInfo, ResourcePtr* ppOut)
            { 
                return CManager::PrepareResource(pInfo, ppOut); 
            }

            Result LoadResource(const SCreateInfo* const pInfo, ResourcePtr** ppOut)
            { 
                return CManager::LoadResource(pInfo, ppOut);
            }

            Result DestroyResource(ResourcePtr* ppResInOut)
            { 
                CManager::ResourcePtr pRes = Utils::PtrStaticCast< CManager::ResourceType >( *ppResInOut );
                return CManager::DestroyResource(&pRes);
            }

            ResourcePtr GetResource(const handle_t& handle)
            {
                ResourcePtrT pRes;
                CManager::GetResource(handle, &pRes);
                return pRes;
            }
            
            ResourcePtr GetResource(cstr_t pName, uint32_t nameLen)
            { return CManager::GetResource(pName, nameLen); }

            ResourcePtr GetResource(const vke_string& strName) { return GetResource( strName.c_str(), strName.length() ); }

        };

    } // Resource
} // VKE