#pragma once

#include "TCDynamicArray.h"

namespace VKE
{
    namespace Utils
    {
        struct ListDefaultPolicy : public DynamicArrayDefaultPolicy
        {

        };

        template<typename DataType>
        struct TSListElement
        {
            DataType Data;
            uint32_t prevIdx;
            uint32_t nextIdx;
        };

        template<typename T>
        class TCListIterator : public std::iterator<std::forward_iterator_tag, T*, T&>
        {
            template<typename T, uint32_t, class, class> friend class TCList;
        public:

            using DataType = T;
            using DataTypePtr = T*;
            using DataTypeRef = T&;

        protected:

            using ElementType = TSListElement< T >;

        public:

            TCListIterator() {}
            TCListIterator(ElementType* pEl, ElementType* pData) : m_pCurr(pEl), m_pData(pData) {}
            TCListIterator(const TCListIterator& Other) :
                TCListIterator(Other.m_pCurr, Other.m_pData) {}

            TCListIterator& operator++()
            {
                //assert(m_pCurr && m_pCurr->nextIdx && "Out-of-bounds iterator increment");
                const auto idx = m_pCurr->nextIdx;
                m_pCurr = &m_pData[idx];
                return *this;
            }

            TCListIterator operator++(int)
            {
                assert(m_pCurr && m_pCurr->nextIdx && "Out-of-bounds iterator increment");
                TCArrayIterator Tmp(*this);
                const auto idx = m_pCurr->nextIdx;
                m_pCurr = m_pData[idx];
                return Tmp;
            }

            TCListIterator& operator=(const TCListIterator& Right)
            {
                m_pData = Right.m_pData;
                m_pCurr = Right.m_pCurr;
                return *this;
            }

            TCListIterator operator+(uint32_t idx) const
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
                        return TCListIterator();
                    }
                }
                return TCListIterator(pCurr, m_pData);
            }

            bool operator!=(const TCListIterator& Right) const
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
        class Policy = ListDefaultPolicy>
        class TCList : protected TCDynamicArray<
            TSListElement< T >,
            DEFAULT_ELEMENT_COUNT,
            AllocatorType,
            Policy>
        {
        protected:

            using ElementType = TSListElement< T >;
            using Base = TCDynamicArray< ElementType, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy >;


        public:

            using DataType = T;
            using DataTypePtr = DataType*;
            using DataTypeRef = DataType&;

            using VisitCallback = std::function<void(DataTypeRef)>;
            using Iterator = TCListIterator< DataType >;
            using ConstIterator = TCListIterator< const DataType >;

        protected:

            void _SetFirst(ElementType* pEl);
            void _SetIdxs(uint32_t startIdx);

        public:

            TCList() : TCDynamicArray() { _SetFirst(this->m_pCurrPtr); _SetIdxs(0); }
            TCList(const TCList& Other) : TCDynamicArray(Other), TCList() {}
            TCList(TCList&& Other) : TCDynamicArray(Other), TCList() {}
            explicit TCList(CountType elemCount) : TCDynamicArray(elemCount), TCList() {}
            TCList(CountType elemCount, const DataTypeRef Default = DataType);
            TCList(CountType elemCount, VisitCallback Callback);

            bool PushBack(const DataType& El);
            bool Remove(CountType idx);
            bool Resize(CountType count);
            bool Resize(CountType count, const DataTypeRef Default);

            DataTypeRef Front() { return this->m_pCurrPtr[m_firstIdx].Data; }
            const DataTypeRef Front() const { return Front(); }
            DataTypeRef Back() { return this->m_pCurrPtr[m_lastIdx].Data; }
            const DataTypeRef Back() const { return Back(); }
            bool IsEmpty() const { return Base::IsEmpty(); }

            DataType PopFront()
            {
                DataType Tmp;
                if (PopFront(&Tmp))
                {
                    return Tmp;
                }
                return DataType();
            }

            bool PopFrontFast(DataTypePtr pOut);
            bool PopFront(DataTypePtr pOut);
            
            DataType PopBack()
            {
                DataType Tmp;
                if (PopBack(&Tmp))
                {
                    return Tmp;
                }
            }
            bool PopBack(DataTypePtr pOut);
            bool PopBackFast(DataTypePtr pOut);

            Iterator begin()
            {
                return Iterator(&this->m_pCurrPtr[m_firstIdx], this->m_pCurrPtr);
            }
            ConstIterator begin() const { return ConstIterator(&this->m_pCurrPtr[m_firstIdx], this->m_pCurrPtr); }
            Iterator end()
            {
                return Iterator(&this->m_pCurrPtr[0], this->m_pCurrPtr);
            }
            ConstIterator end() const { return Iterator(&this->m_pCurrPtr[m_lastIdx + 1], this->m_pCurrPtr); }

            Iterator Find(CountType idx) { return _Find< Iterator >(idx); }
            ConstIterator Find(CountType idx) const { return _Find< ConstIterator >(idx); }
            Iterator Find(const DataTypeRef Data) { return _Find< Iterator >(Data); }
            ConstIterator Find(const DataTypeRef Data) const { return _Find< ConstIterator >(Data); }

            DataType Remove(const Iterator& Itr);
            bool Insert(const Iterator& ItrWhere, const Iterator& ItrWhat);

        protected:

            template<typename ItrType>
            ItrType _Find(CountType idx);
            template<typename ItrType>
            ItrType _Find(const DataTypeRef Data);

            bool _Remove(uint32_t idx, DataTypePtr pOut);

        protected:

            uint32_t    m_firstIdx = 0;
            uint32_t    m_freeIdx = 1;
            uint32_t    m_lastIdx = 0;
        };

#include "inl/TCList.inl"
    } // Utils
} // VKE