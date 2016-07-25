#pragma once

#include "Core/VKECommon.h"
#include "Core/Utils/TCSingleton.h"
#include "Core/Memory/CFreeListPool.h"

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
