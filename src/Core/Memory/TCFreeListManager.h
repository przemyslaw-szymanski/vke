#ifndef __VKE_CFREE_LIST_MANAGER_H__
#define __VKE_CFREE_LIST_MANAGER_H__

#include "VKECommon.h"
#include "Utils/TCSingleton.h"
#include "CFreeListPool.h"

namespace VKE
{
    namespace Memory
    {
        class CFreeListManager : public Utils::TCSingleton< CFreeListManager >
        {
            using PoolMap = std::unordered_map< handle_t, CFreeListPool >;

            public:

                            CFreeListManager();
                virtual     ~CFreeListManager();

                Result Create();
                void Destroy();

                CFreeListPool*  CreatePool(const handle_t& id, bool getIfExists = true);

                Result           DestroyPool(const handle_t& id);

                CFreeListPool*  GetPool(const handle_t& id);

            protected:

                handle_t        _CalcRandomHandle();

            protected:

                PoolMap     m_mPools;
        };
    } // Memory
} // VKE

#endif // __VKE_TYPES_H__