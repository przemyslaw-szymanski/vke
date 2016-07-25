
template<typename _T_>
void TCWeakPtr< _T_ >::operator=(const TCWeakPtr& o)
{
    m_pPtr = o.m_pPtr;
}

template<typename _T_>
void TCWeakPtr< _T_ >::operator=(TCWeakPtr&& o)
{
    m_pPtr = o.Release();
}

template<typename _T_>
void TCWeakPtr< _T_ >::operator=(_T_* pPtr)
{
    m_pPtr = pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator<(const TCWeakPtr& o) const
{
    return m_pPtr < o.m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator<(const _T_* pPtr) const
{
    m_pPtr < m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator<=(const TCWeakPtr& o) const
{
    return m_pPtr <= o.m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator<=(const _T_* pPtr) const
{
    m_pPtr <= m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator>(const TCWeakPtr& o) const
{
    return m_pPtr > o.m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator>(const _T_* pPtr) const
{
    m_pPtr > m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator>=(const TCWeakPtr& o) const
{
    return m_pPtr >= o.m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator>=(const _T_* pPtr) const
{
    m_pPtr >= m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator==(const TCWeakPtr& o) const
{
    return m_pPtr == o.m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator==(const _T_* pPtr) const
{
    m_pPtr == m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator!=(const TCWeakPtr& o) const
{
    return m_pPtr != o.m_pPtr;
}

template<typename _T_>
bool TCWeakPtr< _T_ >::operator!=(const _T_* pPtr) const
{
    m_pPtr != m_pPtr;
}

template<typename _T_>
_T_* TCWeakPtr< _T_ >::Release()
{
    auto pPtr = m_pPtr;
    m_pPtr = nullptr;
    return pPtr;
}

// TCSmartPtr

template<typename _T_>
void TCSmartPtr< _T_ >::operator=(const TCSmartPtr& o)
{
    this->operator=(o.m_pPtr);
}

template<typename _T_>
void TCSmartPtr< _T_ >::operator=(_T_* pPtr)
{
    if(this->m_pPtr != pPtr)
    {
        delete this->m_pPtr;
        this->m_pPtr = pPtr;
    }
}


// TCUniquePtr

template<typename _T_>
TCUniquePtr< _T_ >::TCUniquePtr(TCUniquePtr& o) :
    TCWeakPtr< _T_ >(o.Release())
{
}

template<typename _T_>
TCUniquePtr< _T_ >::TCUniquePtr(TCUniquePtr&& o) :
    TCWeakPtr< _T_ >(o.Release())
{
}

template<typename _T_>
TCUniquePtr< _T_ >::TCUniquePtr(_T_* pPtr) :
    TCWeakPtr< _T_ >(pPtr)
{
    pPtr = nullptr;
}

template<typename _T_>
void TCUniquePtr< _T_ >::operator=(TCUniquePtr& o)
{
    this->m_pPtr = o.Release();
}

template<typename _T_>
void TCUniquePtr< _T_ >::operator=(TCUniquePtr&& o)
{
    this->m_pPtr = o.Release();
}

template<typename _T_>
void TCUniquePtr< _T_ >::operator=(_T_* pPtr)
{
    this->m_pPtr = pPtr;
    pPtr = nullptr;
}

// TRAITS
template<typename _T_>
void RefCountTraits< _T_ >::AddRef(_T_* pPtr)
{
    if(pPtr)
    {
        pPtr->_AddRef();
    }
}

template<typename _T_>
void RefCountTraits< _T_ >::RemoveRef(_T_** ppPtr)
{
    assert(ppPtr);
    _T_* pTmp = *ppPtr;
    if(pTmp && pTmp->_RemoveRef() == 0)
    {
        delete pTmp;
        *ppPtr = nullptr;
    }
}

 template<typename _T_>
 void RefCountTraits< _T_ >::Assign(_T_** ppDst, _T_* pSrc)
 {
     _T_* pDst = *ppDst;
     if(pDst != pSrc)
     {
         RemoveRef(&pDst);
         pDst = pSrc;
         AddRef(pDst);
     }
 }

 template<typename _T_>
 void RefCountTraits< _T_ >::Move(_T_** ppLeft, _T_** ppRight)
 {
     _T_* pDst = *ppLeft;
     _T_* pSrc = *ppRight;
     if(pDst != pSrc)
     {
         RemoveRef(&pDst);
         pDst = pSrc;
         pSrc = nullptr;
     }
 }

 template<typename _T_>
 _T_* RefCountTraits< _T_ >::Move(_T_** ppSrc)
 {
     _T_* pTmp = *ppSrc;
     *ppSrc = nullptr;
     return pTmp;
 }

template<typename _T_>
void ThreadSafeRefCountTraits< _T_ >::AddRef(_T_* pPtr)
{
    Thread::LockGuard l(sMutex);
    RefCountTraits< _T_ >::AddRef(pPtr);
}

template<typename _T_>
void ThreadSafeRefCountTraits< _T_ >::RemoveRef(_T_** ppPtr)
{
    Thread::LockGuard l(sMutex);
    RefCountTraits< _T_ >::RemoveRef(ppPtr);
}

template<typename _T_>
_T_* ThreadSafeRefCountTraits< _T_ >::Move(_T_** ppPtr)
{
    Thread::LockGuard l(sMutex);
    return RefCountTraits< _T_ >::Move(ppPtr);
}

template<typename _T_>
void ThreadSafeRefCountTraits< _T_ >::Assign(_T_** ppLeft, _T_* pRight)
{
    Thread::LockGuard l(sMutex);
    return RefCountTraits< _T_ >::Assign(ppLeft, pRight);
}

template<typename _T_>
void ThreadSafeRefCountTraits< _T_ >::Move(_T_** ppLeft, _T_** ppRight)
{
    Thread::LockGuard l(sMutex);
    RefCountTraits< _T_ >::Move(ppLeft, ppRight);
}

// TCObjectSmartPtr

template<typename _T_, typename _TRAITS_>
TCObjectSmartPtr< _T_, _TRAITS_ >::TCObjectSmartPtr(_T_* pPtr) :
    TCWeakPtr< _T_ >(pPtr)
{
}

template<typename _T_, typename _TRAITS_>
TCObjectSmartPtr< _T_, _TRAITS_ >::TCObjectSmartPtr(const TCObjectSmartPtr& o) :
    TCObjectSmartPtr(o.m_pPtr)
{
}

template<typename _T_, typename _TRAITS_>
TCObjectSmartPtr< _T_, _TRAITS_ >::TCObjectSmartPtr(TCObjectSmartPtr&& o) :
    TCWeakPtr< _T_ >(_TRAITS_::Move(o.m_pPtr))
{
}

template<typename _T_, typename _TRAITS_>
TCObjectSmartPtr< _T_, _TRAITS_ >::TCObjectSmartPtr(const TCWeakPtr< _T_ >& o) :
    TCWeakPtr< _T_ >(o)
{}

template<typename _T_, typename _TRAITS_>
TCObjectSmartPtr< _T_, _TRAITS_ >::~TCObjectSmartPtr()
{
    _TRAITS_::RemoveRef(&this->m_pPtr);
}

template<typename _T_, typename _TRAITS_>
void TCObjectSmartPtr< _T_, _TRAITS_ >::operator=(_T_* pPtr)
{
    if(this->m_pPtr != pPtr)
    {
        this->~TCObjectSmartPtr();

        this->m_pPtr = pPtr;
        _TRAITS_::AddRef(this->m_pPtr);
    }
}

template<typename _T_, typename _TRAITS_>
void TCObjectSmartPtr< _T_, _TRAITS_ >::operator=(const TCObjectSmartPtr& o)
{
    this->operator=(o.m_pPtr);
}

template<typename _T_, typename _TRAITS_>
void TCObjectSmartPtr< _T_, _TRAITS_ >::operator=(TCObjectSmartPtr&& o)
{
    _TRAITS_::Move(&this->m_pPtr, &o.m_pPtr);
}