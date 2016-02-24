#pragma once

#include "VKECommon.h"

namespace VKE
{
    namespace Core
    {
        class VKE_API CObject
        {
            public:

                CObject();
                virtual ~CObject();

                vke_force_inline 
                uint32_t    _AddRef() { return ++m_objRefCount; }

                uint32_t    _RemoveRef() { assert(m_objRefCount > 0); return --m_objRefCount; }
                vke_force_inline
                uint32_t    GetRefCount() const { return m_objRefCount; }

            protected:

                uint32_t    m_objRefCount = 1;
        };
    } // core
} // vke