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
        template<typename _T_>
        class TCWeakPtr
        {
            public:
                inline TCWeakPtr() {}
                inline TCWeakPtr(const TCWeakPtr& Other) : m_pPtr(Other.m_pPtr) {}
                inline TCWeakPtr(TCWeakPtr&& Other) : TCWeakPtr(Other.Release()) { }
                explicit inline TCWeakPtr(_T_* pPtr) : m_pPtr(pPtr) {}

                virtual ~TCWeakPtr() { m_pPtr = nullptr; }

                inline void operator=(const TCWeakPtr&);
                inline void operator=(TCWeakPtr&&);
                inline void operator=(_T_*);
                inline bool operator<(const TCWeakPtr&) const;
                inline bool operator<=(const TCWeakPtr&) const;
                inline bool operator>(const TCWeakPtr&) const;
                inline bool operator>=(const TCWeakPtr&) const;
                inline bool operator==(const TCWeakPtr&) const;
                inline bool operator!=(const TCWeakPtr&) const;
                inline bool operator<(const _T_*) const;
                inline bool operator<=(const _T_*) const;
                inline bool operator>(const _T_*) const;
                inline bool operator>=(const _T_*) const;
                inline bool operator==(const _T_*) const;
                inline bool operator!=(const _T_*) const;

                inline _T_* Get() { return m_pPtr; }
                inline const _T_* const Get() const { return m_pPtr; }
                inline _T_* Release();

                inline bool IsNull() const { return Get() == nullptr; }
                inline bool IsValid() const { return !IsNull(); }

                inline _T_* operator->() { return m_pPtr; }
                inline const _T_* operator->() const { return m_pPtr; }
                inline _T_& operator*() { return *m_pPtr; }
                inline const _T_& operator*() const { return *m_pPtr; }

            protected:

                _T_*    m_pPtr = nullptr;
        };

        template<typename _T_>
        class TCSmartPtr : public TCWeakPtr< _T_ >
        {
            public:

                inline TCSmartPtr() {}
                inline TCSmartPtr(const TCSmartPtr& Other) : TCWeakPtr< _T_ >(Other.m_pPtr) {}
                inline TCSmartPtr(TCSmartPtr&& Other) = delete;
                explicit inline TCSmartPtr(_T_* pPtr) : TCWeakPtr< _T_ >(pPtr) {}

                virtual ~TCSmartPtr() { delete this->m_pPtr; }

                inline void operator=(const TCSmartPtr&);
                inline void operator=(TCSmartPtr&&) = delete;
                inline void operator=(_T_*);
                

                //virtual inline _T_* Release() override;
            
        };

        template<typename _T_>
        class TCUniquePtr final : public TCSmartPtr< _T_ >
        {
            public:

                inline TCUniquePtr() {}
                inline TCUniquePtr(const TCUniquePtr&) = delete;
                inline TCUniquePtr(TCUniquePtr&);
                inline TCUniquePtr(TCUniquePtr&&);
                explicit inline TCUniquePtr(_T_*);

                inline void operator=(TCUniquePtr&);
                inline void operator=(const TCUniquePtr&) = delete;
                inline void operator=(TCUniquePtr&&);
                inline void operator=(_T_*);
        };

        template<typename _T_>
        struct RefCountTraits
        {
            static void AddRef(_T_* pPtr);
            static void RemoveRef(_T_** ppPtr);
            static void Assign(_T_** ppLeft, _T_* pRight);
            static void Move(_T_** ppDst, _T_** ppSrc);
            static _T_* Move(_T_** ppSrc);
        };

        template<typename _T_>
        struct ThreadSafeRefCountTraits
        {
            ThreadSafeRefCountTraits() {}
            ~ThreadSafeRefCountTraits() {}

            static void AddRef(_T_* pPtr);
            static void RemoveRef(_T_** ppPtr);
            static void Move(_T_** ppLeft, _T_** ppPtr);
            static void Assign(_T_** ppLeft, _T_* pRight);
            static _T_* Move(_T_** ppSrc);

            static vke_mutex sMutex;
        };

        template<typename _T_>
        vke_mutex ThreadSafeRefCountTraits< _T_ >::sMutex;

        template<typename _T_, typename _TRAITS_ = ThreadSafeRefCountTraits< _T_ > >
        class TCObjectSmartPtr final : public TCWeakPtr< _T_ >
        {
            static_assert(std::is_base_of< ::VKE::Core::CObject, _T_ >::value, "_T_ is not inherited from CObject");

            public:

                inline TCObjectSmartPtr() {}
                inline TCObjectSmartPtr(const TCObjectSmartPtr&);
                inline TCObjectSmartPtr(TCObjectSmartPtr&&);
                explicit inline TCObjectSmartPtr(_T_*);
                explicit inline TCObjectSmartPtr(const TCWeakPtr< _T_ >&);

                virtual ~TCObjectSmartPtr();

                inline void operator=(const TCObjectSmartPtr&);
                inline void operator=(TCObjectSmartPtr&&);
                inline void operator=(_T_*);
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