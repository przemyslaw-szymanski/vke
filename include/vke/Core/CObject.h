#pragma once

#include "Core/VKECommon.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    namespace Core
    {
#if !VKE_USE_OBJECT_CLASS
#define VKE_ADD_OBJECT_MEMBERS

        class VKE_API CObject
        {
            public:

                vke_force_inline CObject() {}
                vke_force_inline CObject(uint32_t baseRefCount) : m_objRefCount{ baseRefCount } {}
                vke_force_inline virtual ~CObject() {}

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

                vke_force_inline
                uint32_t    _AddRefTS()
                {
                    Threads::ScopedLock l( m_SyncObj );
                    return ++m_objRefCount;
                }

                uint32_t    _RemoveRefTS()
                {
                    Threads::ScopedLock l( m_SyncObj );
                    assert( m_objRefCount > 0 ); return --m_objRefCount;
                }
                vke_force_inline
                uint32_t    GetRefCountTS()
                {
                    Threads::ScopedLock l( m_SyncObj );
                    return m_objRefCount;
                }

                vke_force_inline
                Threads::SyncObject& _GetSyncObject()
                {
                    return m_SyncObj;
                }

                vke_force_inline
                const Threads::SyncObject& _GetSyncObject() const
                {
                    return m_SyncObj;
                }

                vke_force_inline
                const handle_t& GetHandle() const { return m_hObject; }

            protected:

                Threads::SyncObject m_SyncObj;
                handle_t            m_hObject = NULL_HANDLE;
                uint32_t            m_objRefCount = 1;
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