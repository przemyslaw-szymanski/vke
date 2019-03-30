
#define TC_LIST_TEMPLATE \
    typename T, uint32_t DEFAULT_ELEMENT_COUNT, class AllocatorType, class Policy

#define TC_LIST_TEMPLATE_PARAMS \
    T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy

template< TC_LIST_TEMPLATE >
void TCList< TC_LIST_TEMPLATE_PARAMS >::_SetFirst( ElementType* /*pEl*/ )
{
}

template< TC_LIST_TEMPLATE >
void TCList< TC_LIST_TEMPLATE_PARAMS >::_SetIdxs( uint32_t setIdx )
{
    for( uint32_t i = setIdx; i < this->m_resizeElementCount; ++i )
    {
        auto& El = this->m_pCurrPtr[i];
        El.nextIdx = i + 1;
        El.prevIdx = i - 1;
    }
    this->m_pCurrPtr[this->m_resizeElementCount - 1].nextIdx = Npos();
    m_firstAddedIdx = Npos();
    m_lastAddedIdx = Npos();

    m_BeginItr.m_pCurr = &this->m_pCurrPtr[ Npos() ];
    m_BeginItr.m_pData = this->m_pCurrPtr;
    m_EndItr = m_BeginItr;
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::PushBack( const T& Data )
{
    if( this->m_count < this->m_resizeElementCount )
    {
        // Get next free element
        //const auto currIdx = m_nextFreeIdx;
        //VKE_ASSERT( currIdx > 0, "" );
        //auto& El = this->m_pCurrPtr[currIdx];
        //// Get last added element
        //auto& LastEl = this->m_pCurrPtr[m_lastAddedIdx];
        //El.prevIdx = m_lastAddedIdx;
        //m_lastAddedIdx = m_nextFreeIdx;
        //// Set next free element
        //m_nextFreeIdx = El.nextIdx;
        //VKE_ASSERT( m_nextFreeIdx > 0, "" );
        //// Set next id of current element to 0
        //El.nextIdx = 0;
        //El.Data = Data;

        //LastEl.nextIdx = currIdx;

        //if( this->m_count == 0 )
        //{
        //    m_firstAddedIdx = currIdx;
        //    VKE_ASSERT( m_firstAddedIdx > 0, "" );
        //}
        VKE_ASSERT( m_nextFreeIdx != Npos(), "" );
        const uint32_t currIdx = m_nextFreeIdx;

        auto& CurrElement = this->m_pCurrPtr[ currIdx ];
        m_nextFreeIdx = CurrElement.nextIdx;
        CurrElement.prevIdx = m_lastAddedIdx;

        CurrElement.nextIdx = Npos();
        CurrElement.Data = Data;

        m_lastAddedIdx = currIdx;

        if( this->m_count == 0 )
        {
            m_firstAddedIdx = currIdx;
            m_BeginItr.m_pCurr = &this->m_pCurrPtr[m_firstAddedIdx];
            m_BeginItr.m_pData = this->m_pCurrPtr;
        }
        else
        {
            VKE_ASSERT( CurrElement.prevIdx != Npos(), "" );
            auto& PrevElement = this->m_pCurrPtr[CurrElement.prevIdx];
            PrevElement.nextIdx = currIdx;
        }

        this->m_count++;
        return true;
    }
    else
    {
        if( Resize( this->m_count * 2 ) )
        {
            return PushBack( Data );
        }
        return false;
    }
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::_Remove( uint32_t idx, DataTypePtr pOut )
{
    //// Note idx must be > 0
    //assert( idx > 0 );
    //auto& CurrEl = this->m_pCurrPtr[idx];
    //auto& NextEl = this->m_pCurrPtr[CurrEl.nextIdx];
    //auto& PrevEl = this->m_pCurrPtr[CurrEl.prevIdx];
    //const auto currIdx = NextEl.prevIdx;
    //assert( idx == currIdx || idx == m_lastAddedIdx );
    //const auto nextIdx = CurrEl.nextIdx;
    //PrevEl.nextIdx = CurrEl.nextIdx;
    //NextEl.prevIdx = CurrEl.prevIdx;
    //CurrEl.nextIdx = m_nextFreeIdx;
    //m_nextFreeIdx = currIdx ? currIdx : 1;
    //VKE_ASSERT( m_nextFreeIdx > 0, "" );
    //if( m_firstAddedIdx == currIdx )
    //{
    //    m_firstAddedIdx = nextIdx;
    //}
    //this->m_count--;

    //if( pOut )
    //{
    //    *pOut = CurrEl.Data;
    //}

    bool ret = true;
    this->m_count--;

    auto& CurrEl = this->m_pCurrPtr[ idx ];
    uint32_t currIdx = Npos();

    auto pNextEl = &CurrEl;
    auto pPrevEl = &CurrEl;

    if( CurrEl.nextIdx != Npos() )
    {
        // A case when current element is not a last one
        auto& NextEl = this->m_pCurrPtr[CurrEl.nextIdx];
        currIdx = NextEl.prevIdx;
        VKE_ASSERT( NextEl.prevIdx == idx, "" );
        pNextEl = &NextEl;
    }
    if( CurrEl.prevIdx != Npos() )
    {
        // A case when current element is not a first one
        auto& PrevEl = this->m_pCurrPtr[CurrEl.prevIdx];
        VKE_ASSERT( PrevEl.nextIdx == idx, "" );
        currIdx = PrevEl.nextIdx;
        pPrevEl = &PrevEl;
    }
    else
    {
        // A first element in the list
        // Change begin iterator
        m_BeginItr.m_pCurr = &this->m_pCurrPtr[CurrEl.nextIdx];
    }
    
    pPrevEl->nextIdx = CurrEl.nextIdx;
    pNextEl->prevIdx = CurrEl.prevIdx;

    *pOut = CurrEl.Data;

    // Update free elements
    CurrEl.nextIdx = m_nextFreeIdx;
    m_nextFreeIdx = idx;

    return ret;
}

template< TC_LIST_TEMPLATE >
T TCList< TC_LIST_TEMPLATE_PARAMS >::Remove( const Iterator& Itr )
{
    //if( Itr != end() )
    //const auto EndItr = end();
    //VKE_ASSERT( Itr != EndItr, "" );
    VKE_ASSERT( this->m_count > 0, "" );
    {
        // Find out current element index
        ElementType& CurrEl = *Itr.m_pCurr;
        // If current element has next index get nextElement->prevElement
        uint32_t currIdx = Npos();
        if( CurrEl.nextIdx != Npos() )
        {
            // A case when current element is not a last one
            auto& NextEl = this->m_pCurrPtr[ CurrEl.nextIdx ];
            currIdx = NextEl.prevIdx;
        }
        else if( CurrEl.prevIdx != Npos() )
        {
            // A case when current element is not a first one
            auto& PrevEl = this->m_pCurrPtr[ CurrEl.prevIdx ];
            currIdx = PrevEl.nextIdx;
        }
        else
        {
            currIdx = m_firstAddedIdx;
        }
        
        DataType Tmp;
        _Remove( currIdx, &Tmp );
        return Tmp;
    }
    return T();
}

template< TC_LIST_TEMPLATE >
template< class ItrType >
ItrType TCList< TC_LIST_TEMPLATE_PARAMS >::_Find( CountType idx )
{
    uint32_t currIdx = 0;
    const auto& EndItr = end();
    for( auto& Itr = begin(); Itr != EndItr; ++Itr, ++currIdx )
    {
        if( currIdx == idx )
        {
            return Itr;
        }
    }
    return EndItr;
}

template< TC_LIST_TEMPLATE >
template< class ItrType >
ItrType TCList< TC_LIST_TEMPLATE_PARAMS >::_Find( const DataTypeRef Data )
{
    const auto& EndItr = end();
    for( auto& Itr = begin(); Itr != EndItr; ++Itr, ++currIdx )
    {
        if( Data == Itr )
        {
            return Itr;
        }
    }
    return EndItr;
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::PopFront( DataTypePtr pOut )
{
    bool ret = false;
    if( !this->IsEmpty() )
    {
        PopFrontFast( pOut );
        ret = true;
    }
    return ret;
}

template< TC_LIST_TEMPLATE >
void TCList< TC_LIST_TEMPLATE_PARAMS >::PopFrontFast( DataTypePtr pOut )
{
    {
        *pOut = Remove( begin() );
    }
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::PopBack( DataTypePtr pOut )
{
    bool ret = false;
    if( !this->IsEmpty() )
    {
        PopBackFast( pOut );
        ret = true;
    }
    return ret;
}

template< TC_LIST_TEMPLATE >
void TCList< TC_LIST_TEMPLATE_PARAMS >::PopBackFast( DataTypePtr pOut )
{
    {
        _Remove( m_lastAddedIdx, pOut );
    }
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::Resize( CountType count )
{
    bool ret = Base::Resize( count );
    m_BeginItr.m_pData = this->m_pCurrPtr;
    m_EndItr.m_pData = this->m_pCurrPtr;
    m_EndItr.m_pCurr = &this->m_pCurrPtr[ Npos() ];
    return ret;
}

template< TC_LIST_TEMPLATE >
bool TCList< TC_LIST_TEMPLATE_PARAMS >::Resize( CountType count, const DataTypeRef Default )
{
    bool ret = Base::Resize( count, Default );
    m_BeginItr.m_pData = this->m_pCurrPtr;
    m_EndItr.m_pData = this->m_pCurrPtr;
    m_EndItr.m_pCurr = &this->m_pCurrPtr[ Npos() ];
    return ret;
}