#pragma once

#include "TCList.h"

namespace VKE
{
    namespace Utils
    {
        struct DefaultListPolicy
        {

        };

        template<typename DataType>
        struct TSFreeListElement
        {
            DataType Data;
            uint32_t prevIdx;
            uint32_t nextIdx;
        };

        template<typename T>
        class TCFreeListIterator : public std::iterator<std::forward_iterator_tag, T*, T&>
        {
            template<typename T, uint32_t, class, class> friend class TCFreeList;
        public:

            using DataType = T;
            using DataTypePtr = T*;
            using DataTypeRef = T&;

        protected:

            using ElementType = TSListElement< T >;

        public:

            TCFreeListIterator() {}
            TCFreeListIterator(ElementType* pEl, ElementType* pData) : m_pCurr(pEl), m_pData(pData) {}
            TCFreeListIterator(const TCFreeListIterator& Other) :
                TCFreeListIterator(Other.m_pCurr, Other.m_pData) {}

            TCFreeListIterator& operator++()
            {
                //assert(m_pCurr && m_pCurr->nextIdx && "Out-of-bounds iterator increment");
                const auto idx = m_pCurr->nextIdx;
                m_pCurr = &m_pData[idx];
                return *this;
            }

            TCFreeListIterator operator++(int)
            {
                assert(m_pCurr && m_pCurr->nextIdx && "Out-of-bounds iterator increment");
                TCArrayIterator Tmp(*this);
                const auto idx = m_pCurr->nextIdx;
                m_pCurr = m_pData[idx];
                return Tmp;
            }

            TCFreeListIterator& operator=(const TCFreeListIterator& Right)
            {
                m_pData = Right.m_pData;
                m_pCurr = Right.m_pCurr;
                return *this;
            }

            TCFreeListIterator operator+(uint32_t idx) const
            {
                auto currIdx = m_pCurr->nextIdx;
                ElementType* pCurr = nullptr;
                for (uint32_t i = 0; i < idx; ++i)
                {
                    if (idx != 0)
                    {
                        pCurr = &m_pData[currIdx];
                        currIdx = pCurr->nextIdx;
                    }
                    else
                    {
                        return TCFreeListIterator();
                    }
                }
                return TCFreeListIterator(pCurr, m_pData);
            }

            bool operator!=(const TCFreeListIterator& Right) const
            {
                return m_pCurr != Right.m_pCurr;
            }

            DataTypeRef operator*() { return m_pCurr->Data; }
            DataTypePtr operator->() { return &m_pCurr->Data; }
            DataTypeRef operator*() const { return m_pCurr->Data; }
            DataTypePtr operator->() const { return &m_pCurr->Data; }
            //operator DataTypeRef() { return m_pCurr->Data; }

        protected:

            ElementType*    m_pCurr = nullptr;
            ElementType*    m_pData = nullptr;
        };

        template<typename T, uint32_t DEFAULT_ELEMENT_COUNT = 32,
        class AllocatorType = Memory::CHeapAllocator,
        class Policy = DefaultListPolicy>
        class TCFreeList : protected TCList<
            T,
            DEFAULT_ELEMENT_COUNT,
            AllocatorType,
            Policy>
        {
            protected:

                using ElementType = TSListElement< T >;
                using Base = TCList< ElementType, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy >;


            public:

                using DataType = T;
                using DataTypePtr = DataType*;
                using DataTypeRef = DataType&;

                using VisitCallback = std::function<void(DataTypeRef)>;
                using Iterator = TCFreeListIterator< DataType >;
                using ConstIterator = TCFreeListIterator< const DataType >;
                using CountType = Base::CountType;

            protected:

                void _SetFirst(ElementType* pEl);
                void _SetIdxs(uint32_t startIdx);

            public:

                TCFreeList() : TCList() { }
                TCFreeList(const TCFreeList& Other) : TCList(Other), TCFreeList() {}
                TCFreeList(TCFreeList&& Other) : TCList(Other), TCFreeList() {}
                explicit TCFreeList(CountType elemCount) : TCList(elemCount), TCFreeList() {}
                TCFreeList(CountType elemCount, const DataTypeRef Default = DataType);
                TCFreeList(CountType elemCount, VisitCallback Callback);

                

            protected:



            protected:


        };


    } // Utils
} // VKE