#include "Core/Memory/CFreeListPool.h"
#include "Core/Memory/CFreeList.h"
#include "Core/Utils/CLogger.h"

namespace VKE
{
    namespace Memory
    {
        const CFreeListPool::SHandle CFreeListPool::INVALID_HANDLE = { CFreeListPool::MAX_POOL_COUNT, CFreeListPool::MAX_ALLOC_COUNT };

        CFreeListPool::CFreeListPool()
        {

        }

        CFreeListPool::~CFreeListPool()
        {
            Destroy();
        }

        void CFreeListPool::Destroy()
        {
            if( !m_vpFreeLists.empty() )
            {
                for( auto& pPtr : m_vpFreeLists )
                {
                    VKE_DELETE( pPtr );
                }
                m_vpFreeLists.clear();
                m_vFreeListMemRanges.clear();
            }
        }

        Result CFreeListPool::Create(uint32_t freeListElementCount, size_t freeListElemenetSize, uint32_t freeListCount)
        {
            m_freeListElemCount = freeListElementCount;
            m_freeListElemSize = freeListElemenetSize;
            VKE_STL_TRY(m_vpFreeLists.reserve(freeListCount), VKE_ENOMEMORY);
            Result err = AddNewLists(freeListCount);
            if(VKE_FAILED(err))
            {
                return err;
            }
            m_pCurrList = m_vpFreeLists[0];
            m_currListId = 0;
            return VKE_OK;
        }

        Result CFreeListPool::AddNewLists(uint32_t count)
        {
            VKE_ASSERT2( m_freeListElemCount > 0, "" );
            VKE_ASSERT2( m_freeListElemSize > 0, "" );
            Result ret = VKE_OK;

            for(uint32_t i = 0; i < count; ++i)
            {
                auto* pList = VKE_NEW CFreeList();
                if(pList)
                {
                    if(VKE_FAILED(pList->Create(m_freeListElemCount, m_freeListElemSize)))
                    {
                        ret = VKE_FAIL;
                    }

                    VKE_STL_TRY(m_vpFreeLists.push_back(pList), VKE_ENOMEMORY);
                    VKE_STL_TRY(m_vFreeListMemRanges.push_back(pList->GetMemoryRange()), VKE_ENOMEMORY);
                }
                else
                {
                    VKE_LOG_ERR( "Not enough memory for free list" );
                    ret = VKE_ENOMEMORY;
                }
            }
            return ret;
        }

        memptr_t CFreeListPool::Allocate(const uint32_t)
        {
            // Try to allocate in a current list
            auto* pPtr = m_pCurrList->Alloc();
            if(pPtr)
            {
                return pPtr;
            }
            
            // If failed get a new one list
            Result err = AddNewLists(1);
            if(VKE_SUCCEEDED(err))
            {
                m_pCurrList = m_vpFreeLists.back();
                m_currListId = static_cast<uint32_t>(m_vpFreeLists.size() - 1);
                return m_pCurrList->Alloc();
            }
            return nullptr;
        }

        memptr_t CFreeListPool::Allocate( SHandle* pHandle )
        {
            VKE_ASSERT( m_pCurrList->GetAllocCount() < MAX_ALLOC_COUNT );
            // Try to allocate in a current list
            uint32_t index;
            pHandle->poolIndex = m_currListId;
            auto pPtr = m_pCurrList->Allocate( &index );
            if( pPtr )
            {
                pHandle->allocIndex = index;
                return static_cast<memptr_t>(pPtr);
            }
            // If failed get a new one list
            VKE_ASSERT( m_vpFreeLists.size() < MAX_POOL_COUNT );
            Result err = AddNewLists( 1 );
            if( VKE_SUCCEEDED( err ) )
            {
                m_pCurrList = m_vpFreeLists.back();
                m_currListId = static_cast<uint32_t>( m_vpFreeLists.size() - 1 );
                pHandle->poolIndex = m_currListId;

                pPtr = static_cast<memptr_t>(m_pCurrList->Allocate( &index ));
                pHandle->allocIndex = index;
                return pPtr;
            }
            return nullptr;
        }

        Result CFreeListPool::Free(const uint32_t, void** ppPtr)
        {
            auto* pPtr = *ppPtr;
            if(pPtr)
            {
                // Find this address in ranges
                size_t count = m_vFreeListMemRanges.size();
                for(size_t i = 0; i < count; ++i)
                {
                    const auto& Range = m_vFreeListMemRanges[i];
                    if(pPtr >= Range.begin && pPtr < Range.end)
                    {
                        m_vpFreeLists[i]->FreeNoCheck(pPtr);
                        *ppPtr = nullptr;
                        return VKE_OK;
                    }
                }
                VKE_LOG_ERR_RET(VKE_ENOMEMORY, "Memory at: " << pPtr << " not found in free list pools.");
            }
            return VKE_FAIL;
        }

        Result CFreeListPool::Free(SHandle handle)
        {
            m_vpFreeLists[ handle.poolIndex ]->Deallocate(handle.allocIndex);
            return VKE_OK;
        }

        memptr_t CFreeListPool::Get(SHandle handle)
        {
            return m_vpFreeLists[ handle.poolIndex ]->Get( handle.allocIndex );
        }

    } // Memory
} // VKE
