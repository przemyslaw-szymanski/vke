#ifndef __VKE_CFREE_LIST_POOL_H__
#define __VKE_CFREE_LIST_POOL_H__

#include "VKECommon.h"
#include "Utils/TCSingleton.h"

namespace VKE
{
    namespace Memory
    {
        class CFreeList;

        class CFreeListPool
        {
            using FreeListVec = std::vector< CFreeList* >;
            using MemRange = TSExtent< memptr_t >;
            using MemRangeVec = std::vector< MemRange >;

            public:

                            CFreeListPool();
                virtual     ~CFreeListPool();

                Result       Create(uint32_t freeListElementCount, size_t freeListElemenetSize, uint32_t freeListCount);
                Result       AddNewLists(uint32_t count);
                void        Destroy();

                memptr_t    Alloc();
                template<typename _T_> vke_force_inline
                _T_*        Alloc() { return reinterpret_cast<_T_*>(Alloc()); }

                Result       Free(memptr_t* ppPtr);

            protected:

                FreeListVec     m_vpFreeLists;
                MemRangeVec     m_vFreeListMemRanges;
                CFreeList*      m_pCurrList = nullptr;
                uint32_t        m_currListId = 0;
                uint32_t        m_freeListElemCount = 0;
                size_t          m_freeListElemSize = 0;
        };
    } // Memory
} // VKE

#endif // __VKE_CFREE_LIST_POOL_H__