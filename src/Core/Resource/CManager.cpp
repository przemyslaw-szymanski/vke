#include "Core/Resource/CManager.h"
#include "Core/Memory/Memory.h"

namespace VKE
{
    namespace Resource
    {
        struct CManager::SInternal
        {
            vke_hash_map<handle_t, CManager::ResourceRefPtr> mResources;
            vke_vector<CManager::ResourcePtr> vResources;

            struct
            {
                uint32_t currResId = 0;
            } CheckRes;
        };

        CManager::CManager()
        {

        }

        CManager::~CManager()
        {
            Destroy();
        }

        void CManager::Destroy()
        {
            if (m_pInternal)
            {
                for (auto& Pair : m_pInternal->mResources)
                {
                    _FreeMemory(&Pair.second);
                }

                Memory::DestroyObject(&HeapAllocator, &m_pInternal);
            }
        }

        Result CManager::Create()
        {
            VKE_RETURN_IF_FAILED(Memory::CreateObject(&HeapAllocator, &m_pInternal));

            return VKE_OK;
        }

        Result CManager::CheckNextResourceUse()
        {
            assert(m_pInternal);
            auto& CheckRes = m_pInternal->CheckRes;
            size_t size = m_pInternal->vResources.size();
            if(CheckRes.currResId < size)
            {
                auto pRes = m_pInternal->vResources[CheckRes.currResId];
                if(pRes->GetRefCount() == 1)
                {
                    _ResourceUnused(&pRes);
                }
                ++CheckRes.currResId;
                return VKE_OK;
            }

            CheckRes.currResId = 0;
            return VKE_FAIL;      
        }

        Result CManager::CreateResource(const SCreateInfo* const pInfo, ResourcePtr* ppOut)
        {
            uint16_t stages = pInfo->stages;
            ResourceRawPtr pRes = nullptr;
            if (stages & StageBits::CREATE)
            {
                pRes = _AllocateMemory(pInfo);
            }
             
            if( pRes )
            {
                           
                if (stages & StageBits::LOAD)
                {

                }
                if (stages & StageBits::PREPARE)
                {

                }
                if (stages & StageBits::UNLOAD)
                {

                }
                if (stages & StageBits::INVALID)
                {

                }

                *ppOut = pRes;
                return VKE_OK;
            }
            return VKE_ENOMEMORY;
        }

        Result CManager::CreateResources(uint32_t, const SCreateInfo* const*)
        {
            return VKE_FAIL;
        }

        Result CManager::PrepareResource(const SCreateInfo* const, ResourcePtr*)
        {
            return VKE_FAIL;
        }

        Result CManager::LoadResource(const SCreateInfo* const, ResourcePtr**)
        {
            return VKE_FAIL;
        }

        Result CManager::DestroyResource(ResourcePtr*)
        {
            return VKE_FAIL;
        }

        Result CManager::GetResource(const handle_t&, ResourcePtr*)
        {
            return VKE_FAIL;
        }

        Result CManager::GetResource(cstr_t, uint32_t, ResourcePtr*)
        {
            return VKE_FAIL;
        }

        void CManager::CheckAllResourcesUse()
        {

        }

        CManager::ResourceRawPtr CManager::_AllocateMemory(const SCreateInfo* const)
        {
            ResourceRawPtr pRes;
            Memory::CreateObject(&HeapAllocator, &pRes, this);
            return pRes;
        }

        void CManager::_FreeMemory(ResourcePtr* ppOut)
        {
            ResourceRawPtr pPtr = (*ppOut).Get();
            Memory::DestroyObject(&HeapAllocator, &pPtr);
        }

        void CManager::_ResourceUnused(ResourcePtr*)
        {

        }

        Result CManager::_AddResource(ResourcePtr)
        {
            return VKE_FAIL;
        }

        CManager::ResourcePtr CManager::_RemoveResource(ResourcePtr)
        {
            return ResourcePtr();
        }

        CManager::ResourcePtr CManager::_RemoveResource(const handle_t&)
        {
            return ResourcePtr();
        }

    } // Resource
} // VKE