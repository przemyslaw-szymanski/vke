
#define TC_LIST_TEMPLATE \
    typename T, uint32_t DEFAULT_ELEMENT_COUNT, class AllocatorType, class Policy

#define TC_LIST_TEMPLATE_PARAMS \
    T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy

template< TC_LIST_TEMPLATE >
void TCList< TC_LIST_TEMPLATE_PARAMS >::_SetFirst(ElementType* pEl)
{
}

template< TC_LIST_TEMPLATE >
void TCList< TC_LIST_TEMPLATE_PARAMS >::_SetIdxs(uint32_t setIdx)
{
    for (uint32_t i = setIdx; i < this->m_maxElementCount; ++i)
    {
        auto& El = this->m_pPtr[i];
        El.nextIdx = i + 1;
        El.prevIdx = 0;
    }
    this->m_pPtr[this->m_maxElementCount - 1].nextIdx = 0;
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::PushBack(const T& Data)
{
    if (this->m_count < this->m_maxElementCount)
    {
        // Get next free element
        const auto currIdx = m_freeIdx;
        auto& El = this->m_pPtr[currIdx];
        // Get last added element
        auto& LastEl = this->m_pPtr[m_lastIdx];
        El.prevIdx = m_lastIdx;
        m_lastIdx = m_freeIdx;
        // Set next free element
        m_freeIdx = El.nextIdx;
        // Set next id of current element to 0
        El.nextIdx = 0;
        El.Data = Data;

        LastEl.nextIdx = currIdx;

        if (this->m_count == 0)
        {
            m_firstIdx = currIdx;
        }
        this->m_count++;
        return true;
    }
    else
    {
        if( Resize(this->m_count * 2) )
        {
            return PushBack(Data);
        }
        return false;
    }

    return false;
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::_Remove(uint32_t idx, DataTypePtr pOut)
{
    assert(idx > 0);
    auto& CurrEl = this->m_pPtr[idx];
    auto& NextEl = this->m_pPtr[CurrEl.nextIdx];
    auto& PrevEl = this->m_pPtr[CurrEl.prevIdx];
    const auto currIdx = NextEl.prevIdx;
    assert(idx == currIdx || idx == m_lastIdx);
    const auto nextIdx = CurrEl.nextIdx;
    PrevEl.nextIdx = CurrEl.nextIdx;
    NextEl.prevIdx = CurrEl.prevIdx;
    CurrEl.nextIdx = m_freeIdx;
    m_freeIdx = currIdx ? currIdx : 1;
    if (m_firstIdx == currIdx)
    {
        m_firstIdx = nextIdx;
    }
    this->m_count--;
    
    if (pOut)
    {
        *pOut = CurrEl.Data;
    }
    return true;
}

template< TC_LIST_TEMPLATE >
T TCList< TC_LIST_TEMPLATE_PARAMS >::Remove(const Iterator& Itr)
{
    if (Itr != end())
    {
        ElementType& CurrEl = *Itr.m_pCurr;
        auto& NextEl = this->m_pPtr[CurrEl.nextIdx];
        const auto currIdx = NextEl.prevIdx;
        /*auto& PrevEl = this->m_pPtr[CurrEl.prevIdx];
        const auto nextIdx = CurrEl.nextIdx;
        PrevEl.nextIdx = CurrEl.nextIdx;
        NextEl.prevIdx = CurrEl.prevIdx;
        CurrEl.nextIdx = m_freeIdx;
        m_freeIdx = currIdx ? currIdx : 1;
        if (m_firstIdx == currIdx)
        {
            m_firstIdx = nextIdx;
        }
        this->m_count--;
        return CurrEl.Data;*/
        DataType Tmp;
        _Remove(currIdx, &Tmp);
        return Tmp;
    }
    return T();
}

template< TC_LIST_TEMPLATE >
template< class ItrType >
ItrType TCList< TC_LIST_TEMPLATE_PARAMS >::_Find(CountType idx)
{
    uint32_t currIdx = 0;
    const auto& EndItr = end();
    for (auto& Itr = begin(); Itr != EndItr; ++Itr, ++currIdx)
    {
        if (currIdx == idx)
        {
            return Itr;
        }
    }
    return EndItr;
}

template< TC_LIST_TEMPLATE >
template< class ItrType >
ItrType TCList< TC_LIST_TEMPLATE_PARAMS >::_Find(const DataTypeRef Data)
{
    const auto& EndItr = end();
    for (auto& Itr = begin(); Itr != EndItr; ++Itr, ++currIdx)
    {
        if (Data == Itr)
        {
            return Itr;
        }
    }
    return EndItr;
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::PopFront(DataTypePtr pOut)
{
    if (!this->IsEmpty())
    {
        *pOut = *begin();
        return Remove(begin());
    }
    return false;
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::PopBack(DataTypePtr pOut)
{
    if (!this->IsEmpty())
    {
        return _Remove(m_lastIdx, pOut);
    }
    return false;
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::Resize(CountType count)
{
    return Base::Resize(count);
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::Resize(CountType count, const DataTypeRef Default)
{
    if( Base::Resize(count) )
    {
        auto& Itr = begin();
        const auto& ItrEnd = end();
        for( Itr; Itr != ItrEnd; ++Itr )
        {
            *Itr = Default;
        }
        return true;
    }
    return false;
}