#pragma once

#include "Core/VKECommon.h"

namespace VKE
{
    namespace Core
    {
#if !VKE_USE_OBJECT_CLASS
#define VKE_ADD_OBJECT_MEMBERS

        class VKE_API CObject
        {
            public:

                CObject();
                virtual ~CObject();

                vke_force_inline
                    uint32_t    _AddRef()
                {
                    return ++m_objRefCount;
                }

                uint32_t    _RemoveRef()
                {
                    assert(m_objRefCount > 0); return --m_objRefCount;
                }
                vke_force_inline
                    uint32_t    GetRefCount() const
                {
                    return m_objRefCount;
                }

            protected:

                uint32_t    m_objRefCount = 1;
        };
#else
#define VKE_ADD_OBJECT_MEMBERS \
    public: \
    vke_force_inline void _AddRef() { ++m_objRefCount; } \
    vke_force_inline uint32_t _RemoveRef() { assert(m_objRefCount > 0); return --m_objRefCount; } \
    vke_force_inline uint32_t _GetRefCount() const { return m_objRefCount; } \
    private: \
    uint32_t m_objRefCount = 1;

        class CObject
        {};

        
#endif // VKE_USE_OBJECT_CLASS
    } // core
} // vke