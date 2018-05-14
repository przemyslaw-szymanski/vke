#include "TCFreeListManager.h"
#include "Core/Memory/CFreeListPool.h"
#include "Core/Utils/CLogger.h"

namespace VKE
{
    namespace Memory
    {
        CFreeListManager::CFreeListManager()
        {

        }

        CFreeListManager::~CFreeListManager()
        {
            Destroy();
        }

        void CFreeListManager::Destroy()
        {
            m_mPools.clear();
        }

        Result CFreeListManager::Create()
        {
            return VKE_OK;
        }

        handle_t CFreeListManager::_CalcRandomHandle()
        {
            handle_t handle = 0;
            PoolMap::iterator Itr = m_mPools.end();
            do
            {
                handle = static_cast<handle_t>(rand() % RAND_MAX);
                Itr = m_mPools.find(handle);
            } while(Itr != m_mPools.end());
            return handle;
        }

        CFreeListPool* CFreeListManager::CreatePool(const handle_t& id, bool getIfExists)
        {
            handle_t handle = id;
            if(handle == RANDOM_HANDLE)
            {
                handle = _CalcRandomHandle();
                CFreeListPool Pool;
                auto ret = m_mPools.insert(PoolMap::value_type(handle, Pool));
                if(ret.second)
                    return &ret.first->second;
                else
                {
                    VKE_LOG_ERR("Unable to insert free list pool");
                    return nullptr;
                }
            }

            auto Itr = m_mPools.find(handle);
            if(Itr == m_mPools.end())
            {
                CFreeListPool Pool;
                VKE_STL_TRY(m_mPools.insert(PoolMap::value_type(id, Pool)), nullptr);
            }
            if(getIfExists)
                return &Itr->second;
            return nullptr;
        }

        Result CFreeListManager::DestroyPool(const handle_t& id)
        {
            auto Itr = m_mPools.find(id);
            if(Itr != m_mPools.end())
            {
                Itr->second.Destroy();
                m_mPools.erase(Itr);
            }
            return VKE_ENOTFOUND;
        }

        CFreeListPool* CFreeListManager::GetPool(const handle_t& id)
        {
            auto Itr = m_mPools.find(id);
            if(Itr != m_mPools.end())
                return &Itr->second;
            return nullptr;
        }

    } // Memory
} // VKE
