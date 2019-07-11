#pragma once

#include "Core/VKECommon.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    namespace Core
    {
#if !VKE_USE_OBJECT_CLASS
#define VKE_ADD_OBJECT_MEMBERS

#define VKE_DECL_OBJECT_HANDLE(_type) \
    public: _type GetHandle() const { return m_hObject; } \
    protected: void vke_force_inline _SetHandle(const _type& handle) { m_hObject = handle; } \
    protected: _type m_hObject

#define VKE_DECL_OBJECT_REF_COUNT(_startRefCount) \
    public: vke_force_inline uint32_t _AddRef() { return ++m_objRefCount; } \
    public: vke_force_inline uint32_t _RemoveRef() { VKE_ASSERT( m_objRefCount > 0, "" ); return --m_objRefCount; } \
    public: vke_force_inline uint32_t GetRefCount() const { return m_objRefCount; } \
    protected: uint32_t m_objRefCount = (_startRefCount)

#define VKE_DECL_OBJECT_TS_REF_COUNT(_startRefCount) \
    public: vke_force_inline uint32_t _AddRefTS() { Threads::ScopedLock l( m_SyncObj ); return _AddRef(); } \
    public: vke_force_inline uint32_t _RemoveRefTS() { Threads::ScopedLock l( m_SyncObj ); return _RemoveRef(); } \
    public: vke_force_inline uint32_t GetRefCountTS() { Threads::ScopedLock l( m_SyncObj ); return GetRefCount(); } \
    protected: vke_force_inline Threads::SyncObject& _GetSyncObject() { return m_SyncObj; } \
    VKE_DECL_OBJECT_REF_COUNT(_startRefCount); \
    protected: Threads::SyncObject m_SyncObj

#define VKE_DECL_BASE_OBJECT(_handleType) \
    VKE_DECL_OBJECT_HANDLE( _handleType ); \
    VKE_DECL_OBJECT_TS_REF_COUNT( 1 )

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

            protected:

                Threads::SyncObject m_SyncObj;
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