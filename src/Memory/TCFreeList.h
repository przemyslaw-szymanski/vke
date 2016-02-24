#ifndef __VKE_TCFREE_LIST_H__
#define __VKE_TCFREE_LIST_H__

#include "CFreeList.h"

namespace VKE
{
    namespace Memory
    {
        template<typename _T_>
        class TCFreeList : public CFreeList
        {
            public:

                TCFreeList(uint32_t elementCount) : TCFreeList(elementCount, sizeof(_T_)) {}
                virtual ~TCFreeList() {}

                Result Create(uint32_t elementCount) { return Create(elementCount, sizeof(_T_)); }
        };
    } // Memory
} // VKE

#endif // __VKE_TCFREE_LIST_H__