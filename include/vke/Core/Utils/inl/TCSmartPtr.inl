
template<typename T>
void TCWeakPtr< T >::operator=(const TCWeakPtr& o)
{
    m_pPtr = o.m_pPtr;
}

template<typename T>
void TCWeakPtr< T >::operator=(TCWeakPtr&& o)
{
    m_pPtr = o.Release();
}

template<typename T>
void TCWeakPtr< T >::operator=(T* pPtr)
{
    m_pPtr = pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator<(const TCWeakPtr& o) const
{
    return m_pPtr < o.m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator<(const T* pPtr) const
{
    m_pPtr < m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator<=(const TCWeakPtr& o) const
{
    return m_pPtr <= o.m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator<=(const T* pPtr) const
{
    m_pPtr <= m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator>(const TCWeakPtr& o) const
{
    return m_pPtr > o.m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator>(const T* pPtr) const
{
    m_pPtr > m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator>=(const TCWeakPtr& o) const
{
    return m_pPtr >= o.m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator>=(const T* pPtr) const
{
    m_pPtr >= m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator==(const TCWeakPtr& o) const
{
    return m_pPtr == o.m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator==(const T* pPtr) const
{
    return m_pPtr == m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator!=(const TCWeakPtr& o) const
{
    return m_pPtr != o.m_pPtr;
}

template<typename T>
bool TCWeakPtr< T >::operator!=(const T* pPtr) const
{
    return m_pPtr != m_pPtr;
}

template<typename T>
T* TCWeakPtr< T >::Release()
{
    auto pPtr = m_pPtr;
    m_pPtr = nullptr;
    return pPtr;
}

// TCSmartPtr

template<typename T>
void TCSmartPtr< T >::operator=(const TCSmartPtr& o)
{
    this->operator=(o.m_pPtr);
}

template<typename T>
void TCSmartPtr< T >::operator=(T* pPtr)
{
    if(this->m_pPtr != pPtr)
    {
        delete this->m_pPtr;
        this->m_pPtr = pPtr;
    }
}


// TCUniquePtr

template<typename T>
TCUniquePtr< T >::TCUniquePtr(TCUniquePtr& o) :
    TCWeakPtr< T >(o.Release())
{
}

template<typename T>
TCUniquePtr< T >::TCUniquePtr(TCUniquePtr&& o) :
    TCWeakPtr< T >(o.Release())
{
}

template<typename T>
TCUniquePtr< T >::TCUniquePtr(T* pPtr) :
    TCWeakPtr< T >(pPtr)
{
    pPtr = nullptr;
}

template<typename T>
void TCUniquePtr< T >::operator=(TCUniquePtr& o)
{
    this->m_pPtr = o.Release();
}

template<typename T>
void TCUniquePtr< T >::operator=(TCUniquePtr&& o)
{
    this->m_pPtr = o.Release();
}

template<typename T>
void TCUniquePtr< T >::operator=(T* pPtr)
{
    this->m_pPtr = pPtr;
    pPtr = nullptr;
}

// TRAITS

template<typename T>
void RefCountTraits< T >::AddRef(T** ppPtr)
{
    (*ppPtr)->_AddRef();
}

template<typename T>
void RefCountTraits< T >::RemoveRef(T** ppPtr)
{
    assert(ppPtr);
    T* pTmp = *ppPtr;
    if( pTmp->_RemoveRef() == 0 )
    {
        delete pTmp;
        *ppPtr = nullptr;
    }
}

 template<typename T>
 void RefCountTraits< T >::Assign(T** ppDst, T* pSrc)
 {
     if(*ppDst != pSrc)
     {
         if( *ppDst )
         {
             RemoveRef( ppDst );
         }
         *ppDst = pSrc;
         if( *ppDst )
         {
             AddRef( *ppDst );
         }
     }
 }

 template<typename T>
 void RefCountTraits< T >::Move(T** ppDst, T** ppSrc)
 {
     *ppDst = *ppSrc;
     *ppSrc = nullptr;
 }

 template<typename T>
 T* RefCountTraits< T >::Move(T** ppSrcOut)
 {
     T* pTmp = *ppSrcOut;
     *ppSrcOut = nullptr;
     return pTmp;
 }

template<typename T, class MutexType, class ScopedLockType>
void ThreadSafeRefCountTraits< T, MutexType, ScopedLockType >::AddRef(T** ppPtr)
{
    (*ppPtr)->_AddRefTS();
}

template<typename T, class MutexType, class ScopedLockType>
void ThreadSafeRefCountTraits< T, MutexType, ScopedLockType >::RemoveRef(T** ppPtr)
{
    assert( ppPtr );
    T* pTmp = *ppPtr;
    if( pTmp->_RemoveRefTS() == 0 )
    {
        delete pTmp;
        *ppPtr = nullptr;
    }
}

template<typename T, class MutexType, class ScopedLockType>
void ThreadSafeRefCountTraits< T, MutexType, ScopedLockType >::Assign(T** ppDst, T* pSrc)
{
    if( *ppDst != pSrc )
    {
        if( *ppDst )
        {
            RemoveRef( ppDst );
        }
        *ppDst = pSrc;
        if( *ppDst )
        {
            AddRef( ppDst );
        }
    }
}

template<typename T, class MutexType, class ScopedLockType>
void ThreadSafeRefCountTraits< T, MutexType, ScopedLockType >::Move(T** ppLeft, T** ppRight)
{
    RefCountTraits< T >::Move(ppLeft, ppRight);
}

// TCObjectSmartPtr

template<typename T, typename Policy>
TCObjectSmartPtr< T, Policy >::TCObjectSmartPtr(T* pPtr)
{
    Policy::Assign( &this->m_pPtr, pPtr );
}

template<typename T, typename Policy>
TCObjectSmartPtr< T, Policy >::TCObjectSmartPtr(const TCObjectSmartPtr& o) :
    TCObjectSmartPtr(o.m_pPtr)
{
}

template<typename T, typename Policy>
TCObjectSmartPtr< T, Policy >::TCObjectSmartPtr(TCObjectSmartPtr&& o) :
    TCWeakPtr< T >( o.Release() )
{
}

template<typename T, typename Policy>
TCObjectSmartPtr< T, Policy >::TCObjectSmartPtr(TCWeakPtr< T >& o)
{
    Policy::Assign( &this->m_pPtr, o.Get() );
}

template<typename T, typename Policy>
TCObjectSmartPtr< T, Policy >::~TCObjectSmartPtr()
{
    if( this->m_pPtr )
    {
        Policy::RemoveRef( &this->m_pPtr );
    }
}

template<typename T, typename Policy>
TCObjectSmartPtr< T, Policy >& TCObjectSmartPtr< T, Policy >::operator=(T* pPtr)
{
    Policy::Assign( &this->m_pPtr, pPtr );
    return *this;
}

template<typename T, typename Policy>
TCObjectSmartPtr< T, Policy >& TCObjectSmartPtr< T, Policy >::operator=(const TCObjectSmartPtr& o)
{
    this->operator=( o.m_pPtr );
    return *this;
}

template<typename T, typename Policy>
TCObjectSmartPtr< T, Policy >& TCObjectSmartPtr< T, Policy >::operator=(TCObjectSmartPtr&& o)
{
    Policy::Move( &this->m_pPtr, &o.m_pPtr );
    return *this;
}

template<typename T, typename Policy>
TCObjectSmartPtr< T, Policy >& TCObjectSmartPtr< T, Policy >::operator=(TCWeakPtr< T >& o)
{
    this->operator=( o.Get() );
    return *this;
}

template<typename T, typename Policy>
TCObjectSmartPtr< T, Policy >& TCObjectSmartPtr< T, Policy >::operator=(TCWeakPtr< T >&& o)
{
    Policy::Move( &this->m_pPtr, o.Get() );
    return *this;
}