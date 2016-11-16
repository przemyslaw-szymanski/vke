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
                T& GetNextElement(CountType* pCurrentIdx);
                T& GetCurrentElement();
                const T& GetCurrentElement() const;

            protected:

                uint32_t    m_currIdx = 0;
        };

        template< TC_DYNAMIC_RING_ARRAY_TEMPLATE >
        T& TCDynamicRingArray< TC_DYNAMIC_RING_ARRAY_TEMPLATE_PARAMS >::GetNextElement()
        {
            auto idx = m_currIdx;
            m_currIdx++;
            m_currIdx %= this->GetMaxCount();
            return this->At(idx);
        }

        template< TC_DYNAMIC_RING_ARRAY_TEMPLATE >
        T& TCDynamicRingArray< TC_DYNAMIC_RING_ARRAY_TEMPLATE_PARAMS >::GetNextElement(CountType* pOut)
        {
            *pOut = m_currIdx;
            return GetNextElement();
        }

        template< TC_DYNAMIC_RING_ARRAY_TEMPLATE >
        T& TCDynamicRingArray< TC_DYNAMIC_RING_ARRAY_TEMPLATE_PARAMS >::GetCurrentElement()
        {
            return this->At(m_currIdx);
        }

        template< TC_DYNAMIC_RING_ARRAY_TEMPLATE >
        const T& TCDynamicRingArray< TC_DYNAMIC_RING_ARRAY_TEMPLATE_PARAMS >::GetCurrentElement() const
        {
            return this->At(m_currIdx);
        }
    }
}