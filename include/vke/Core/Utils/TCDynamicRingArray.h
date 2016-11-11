#pragma once

#include "TCDynamicArray.h"

namespace VKE
{
    namespace Utils
    {
#define TC_DYNAMIC_RING_ARRAY_TEMPLATE \
    typename T, \
    uint32_t DEFAULT_ELEMENT_COUNT, \
    typename AllocatorType, \
    typename Policy

#define TC_DYNAMIC_RING_ARRAY_TEMPLATE_PARAMS \
    T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy

        template
        <
            typename T,
            uint32_t DEFAULT_ELEMENT_COUNT = 32,
            typename AllocatorType = Memory::CHeapAllocator,
            typename Policy = DynamicArrayDefaultPolicy
        >
        class TCDynamicRingArray : public TCDynamicArray< T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy >
        {
            public:

                T& GetNextElement();

            protected:

                uint32_t    m_currIdx = 0;
        };

        template< TC_DYNAMIC_RING_ARRAY_TEMPLATE >
        T& TCDynamicRingArray< TC_DYNAMIC_RING_ARRAY_TEMPLATE_PARAMS >::GetNextElement()
        {
            if( m_currIdx >= this->GetCount() )
                m_currIdx = 0;
            return this->At(m_currIdx++);
        }
    }
}