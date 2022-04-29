#pragma once

#include "Core/VKECommon.h"
#include "Core/Threads/Common.h"

namespace VKE
{
    namespace Core
    {
        class CObject;
    } // core

    namespace Utils
    {
        template<typename T>
        class TCWeakPtr
        {
            public:
                inline TCWeakPtr() {}
                inline TCWeakPtr(const TCWeakPtr& Other) : m_pPtr(Other.m_pPtr) {}
                inline TCWeakPtr(TCWeakPtr&& Other) : TCWeakPtr(Other.Release()) { }
                explicit inline TCWeakPtr(T* pPtr) : m_pPtr(pPtr) {}

                virtual ~TCWeakPtr() { m_pPtr = nullptr; }

                inline void operator=(const TCWeakPtr&);
                inline void operator=(TCWeakPtr&&);
                inline void operator=(T*);
                inline bool operator<(const TCWeakPtr&) const;
                inline bool operator<=(const TCWeakPtr&) const;
                inline bool operator>(const TCWeakPtr&) const;
                inline bool operator>=(const TCWeakPtr&) const;
                inline bool operator==(const TCWeakPtr&) const;
                inline bool operator!=(const TCWeakPtr&) const;
                inline bool operator<(const T*) const;
                inline bool operator<=(const T*) const;
                inline bool operator>(const T*) const;
                inline bool operator>=(const T*) const;
                inline bool operator==(const T*) const;
                inline bool operator!=(const T*) const;

                inline T* Get() { return m_pPtr; }
                inline const T* const Get() const { return m_pPtr; }
                inline T* Release();

                inline bool IsNull() const { return Get() == nullptr; }
                inline bool IsValid() const { return !IsNull(); }

                inline T* operator->() { return m_pPtr; }
                inline const T* operator->() const { return m_pPtr; }
                inline T& operator*() { return *m_pPtr; }
                inline const T& operator*() const { return *m_pPtr; }

            protected:

                T*    m_pPtr = nullptr;
        };

        template<typename T>
        class TCSmartPtr : public TCWeakPtr< T >
        {
            public:

                inline TCSmartPtr() {}
                inline TCSmartPtr(const TCSmartPtr& Other) : TCWeakPtr< T >(Other.m_pPtr) {}
                inline TCSmartPtr(TCSmartPtr&& Other) : TCWeakPtr< T >(Other.m_pPtr) {}
                explicit inline TCSmartPtr(T* pPtr) : TCWeakPtr< T >(pPtr) {}

                virtual ~TCSmartPtr() { delete this->m_pPtr; }

                inline void operator=( const TCSmartPtr& );
                inline void operator=( TCSmartPtr&& );
                inline void operator=(T*);

                //virtual inline T* Release() override;
        };

        template<typename T>
        class TCUniquePtr final : public TCSmartPtr< T >
        {
            public:

                inline TCUniquePtr() {}
                inline TCUniquePtr(const TCUniquePtr&) = delete;
                inline TCUniquePtr(TCUniquePtr&);
                inline TCUniquePtr(TCUniquePtr&&);
                explicit inline TCUniquePtr(T*);

                inline void operator=(TCUniquePtr&);
                inline void operator=(const TCUniquePtr&) = delete;
                inline void operator=(TCUniquePtr&&);
                inline void operator=(T*);
        };

        template<typename T>
        struct RefCountTraits
        {
            static void AddRef(T** ppPtr);
            static void RemoveRef(T** ppPtr);
            static void Assign(T** ppLeft, T* pRight);
            static void Move(T** ppDst, T** ppSrc);
            static T* Move(T** ppSrcOut);
        };

        template
        <
            class SyncObjectType,
            class ScopedLockType
        >
        struct MutexPolicy
        {
            using ScopedLock = ScopedLockType;
            using Mutex = SyncObjectType;
        };

        using SyncObjMutexPolicy = MutexPolicy< Threads::SyncObject, Threads::ScopedLock >;
        using StdMutexPolicy = MutexPolicy< std::mutex, std::lock_guard< std::mutex > >;

        template
        <
            class T,
            class MutexType = Threads::SyncObject,
            class ScopedLockType = Threads::ScopedLock
        >
        struct ThreadSafeRefCountTraits
        {
            ThreadSafeRefCountTraits() {}
            ~ThreadSafeRefCountTraits() {}

            static void AddRef(T** ppPtr);
            static void RemoveRef(T** ppPtr);
            static void Move(T** ppLeft, T** ppPtr);
            static void Assign(T** ppLeft, T* pRight);

            static MutexType sMutex;
        };

        template<typename T, class MutexType, class ScopedLockType>
        MutexType ThreadSafeRefCountTraits< T, MutexType, ScopedLockType >::sMutex;

        template<typename T, typename _TRAITS_ = ThreadSafeRefCountTraits< T > >
        class TCObjectSmartPtr final : public TCWeakPtr< T >
        {
            //static_assert(std::is_base_of< ::VKE::Core::CObject, T >::value, "T is not inherited from CObject");

            public:

                inline TCObjectSmartPtr() : TCWeakPtr< T >() {}
                inline TCObjectSmartPtr(const TCObjectSmartPtr&);
                inline TCObjectSmartPtr(TCObjectSmartPtr&&);
                explicit inline TCObjectSmartPtr(T*);
                explicit inline TCObjectSmartPtr(TCWeakPtr< T >&);

                virtual ~TCObjectSmartPtr();

                inline TCObjectSmartPtr& operator=(const TCObjectSmartPtr&);
                inline TCObjectSmartPtr& operator=(TCObjectSmartPtr&&);
                inline TCObjectSmartPtr& operator=(T*);
                inline TCObjectSmartPtr& operator=(TCWeakPtr< T >&);
                inline TCObjectSmartPtr& operator=(TCWeakPtr< T >&&);
        };

        template<typename _DST_, typename _SRC_>
        vke_force_inline TCWeakPtr< _DST_ > PtrStaticCast(TCWeakPtr< _SRC_ >& pPtr)
        {
            return TCWeakPtr< _DST_ >( static_cast< _DST_* >( pPtr.Get() ) );
        }

        template<typename _DST_, typename _SRC_>
        vke_force_inline TCWeakPtr< _DST_ > PtrReinterpretCast(TCWeakPtr< _SRC_ >& pPtr)
        {
            return TCWeakPtr< _DST_ >( reinterpret_cast<_DST_*>(pPtr.Get()) );
        }

#include "inl/TCSmartPtr.inl"

    } // utils

#define ptr_static_cast Utils::PtrStaticCast
#define ptr_reinterpret_cast Utils::PtrReinterpretCast

} // vke