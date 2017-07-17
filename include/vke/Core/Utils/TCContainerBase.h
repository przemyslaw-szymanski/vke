#pragma once

#include "Core/VKECommon.h"
#include "Core/Memory/Memory.h"

namespace VKE
{
    namespace Utils
    {
        static const uint32_t INVALID_POSITION = static_cast<uint32_t>(~0);

        template<typename DataType>
        class TCArrayIterator : public std::iterator<std::forward_iterator_tag, DataType*, DataType&>
        {

        public:

            using DataTypeRef = DataType&;
            using DataTypePtr = DataType*;
            

        public:

            TCArrayIterator() {}
            TCArrayIterator(const TCArrayIterator& Other) :
                TCArrayIterator(Other.m_pCurr, Other.m_pEnd) {}

            TCArrayIterator(DataType* pData, DataType* pEnd) :
                m_pCurr(pData),
                m_pEnd(pEnd) {}

            TCArrayIterator& operator++()
            {
                assert(m_pCurr < m_pEnd && "Out-of-bounds iterator increment");
                m_pCurr++;
                return *this;
            }

            TCArrayIterator operator++(int)
            {
                assert(m_pCurr < m_pEnd && "Out-of-bounds iterator increment");
                TCArrayIterator Tmp(*this);
                m_pCurr++;
                return Tmp;
            }

            bool operator==(const TCArrayIterator& Right) const { return m_pCurr == Right.m_pCurr; }
            bool operator!=(const TCArrayIterator& Right) const { return m_pCurr != Right.m_pCurr; }
            bool operator<(const TCArrayIterator& Right) const { return m_pCurr < Right.m_pCurr; }
            bool operator<=(const TCArrayIterator& Right) const { return m_pCurr <= Right.m_pCurr; }
            bool operator>(const TCArrayIterator& Right) const { return m_pCurr > Right.m_pCurr; }
            bool operator>=(const TCArrayIterator& Right) const { return m_pCurr >= Right.m_pCurr; }

            DataTypeRef operator*() { return *m_pCurr; }
            DataTypePtr operator->() { return m_pCurr; }

        private:
            DataType* m_pCurr = nullptr;
            DataType* m_pEnd = nullptr;
        };

        struct ArrayContainerDefaultPolicy
        {
            // On Resize
            struct Resize
            {
                static uint32_t Calc(uint32_t current) { return current; }
            };

            // On Reserve
            struct Reserve
            {
                static uint32_t Calc(uint32_t current) { return current; }
            };

            // On Remove
            struct Remove
            {

            };
        };

#define TC_ARRAY_CONTAINER_TEMPLATE \
    typename DataType, class AllocatorType, class Policy


#define TC_ARRAY_CONTAINER_TEMPLATE_PARAMS \
    DataType, AllocatorType, Policy

        template
        <
            typename T,
            class AllocatorType = Memory::CHeapAllocator,
            class Policy = ArrayContainerDefaultPolicy
        >
        class TCArrayContainer
        {
            public:

                using DataType = T;
                using DataTypePtr = DataType*;
                using DataTypeRef = DataType&;
                using SizeType = uint32_t;
                using CountType = uint32_t;
                using AllocatorPtr = Memory::IAllocator*;

                using VisitCallback = std::function<void(CountType, DataTypeRef)>;

                using iterator = TCArrayIterator< DataType >;
                using const_iterator = TCArrayIterator< const DataType >;

            public:

                TCArrayContainer() {}
                TCArrayContainer(const TCArrayContainer& Other);
                TCArrayContainer(TCArrayContainer&& Other);
                TCArrayContainer(std::initializer_list<DataType> List);

                explicit TCArrayContainer(uint32_t count)
                {
                    auto res = Resize(count);
                    assert(res == true);
                }

                explicit TCArrayContainer(uint32_t count, const DataTypeRef DefaultValue)
                {
                    auto res = Resize(count, DefaultValue);
                    assert(res == true);
                }

                TCArrayContainer(uint32_t count, VisitCallback&& Callback)
                {
                    auto res = Resize(count, Callback);
                    assert(res == true);
                }

                virtual ~TCArrayContainer() { Destroy(); }

                void Destroy();

                SizeType GetCapacity() const { return m_capacity; }
                CountType GetCount() const { return m_count; }
                SizeType CalcSize() const { return m_count * sizeof(DataType); }
                bool IsEmpty() const { return GetCount() == 0; }

                bool Reserve(CountType elemCount);
                bool Resize(CountType newElemCount);
                bool Resize(CountType newElemCount, const DataTypeRef DefaultData);
                bool Resize(CountType newElemCount, VisitCallback Callback);

                DataTypeRef Back() { return At(m_count - 1); }
                const DataTypeRef Back() const { return At(m_count - 1); }
                DataTypeRef Front() { return At(0); }
                const DataTypeRef Front() const { return At(0); }

                bool Copy(TCArrayContainer* pOut) const;
                void Move(TCArrayContainer* pOut);

                template<typename IndexType>
                DataTypeRef At(const IndexType& index) { return _At(m_pCurrPtr, index); }
                template<typename IndexType>
                const DataTypeRef At(const IndexType& index) const { return _At(m_pCurrPtr, index); }
                template<typename IndexType>
                DataTypeRef operator[](const IndexType& index) { return At(index); }
                template<typename IndexType>
                const DataTypeRef operator[](const IndexType& index) const { return At(index); }

                TCArrayContainer& operator=(const TCArrayContainer& Other) { Other.Copy(this); return *this; }
                //void operator=(const TCArrayContainer& Other) { Other.Copy(this); }
                TCArrayContainer& operator=(TCArrayContainer&& Other) { Other.Move(this); return *this; }

                iterator begin() { return iterator(m_pCurrPtr, m_pCurrPtr + m_count); }
                iterator end() { return iterator(m_pCurrPtr + m_count, m_pCurrPtr + m_count); }
                const_iterator begin() const { return const_iterator(m_pCurrPtr, m_pCurrPtr + m_count); }
                const_iterator end() const { return const_iterator(m_pCurrPtr + m_count, m_pCurrPtr + m_count); }

            protected:

                template<typename IndexType>
                vke_force_inline
                DataTypeRef _At(DataTypePtr pPtr, const IndexType& idx)
                {
                    assert(pPtr);
                    assert((idx >= static_cast<IndexType>(0) && idx < static_cast<IndexType>(m_count)) && "Element out of bounds.");
                    return pPtr[ idx ];
                }
                
                template<typename IndexType>
                vke_force_inline
                const DataTypeRef _At(DataTypePtr pPtr, const IndexType& idx) const
                {
                    assert(pPtr);
                    assert((idx >= static_cast<IndexType>(0) && idx < static_cast<IndexType>(m_count)) && "Element out of bounds.");
                    return pPtr[ idx ];
                }

                void _DestroyElements(DataTypePtr pData);

            protected:

                DataTypePtr     m_pData = nullptr;
                DataTypePtr     m_pCurrPtr = nullptr;
                SizeType        m_capacity = 0;
                CountType       m_count = 0;
                AllocatorType   m_Allocator = AllocatorType::Create();
        };

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::TCArrayContainer(const TCArrayContainer& Other)
        {
            auto res = Other.Copy(this);
            assert(res);
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::TCArrayContainer(TCArrayContainer&& Other)
        {
            auto res = Other.Move(this);
            assert(res);
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::TCArrayContainer(
            std::initializer_list<DataType> List)
        {
            const auto count = static_cast<CountType>(List.size());
            if (count > 0)
            {
                const auto newMaxCount = Policy::Reserve::Calc(count);
                Reserve(newMaxCount);
                for (auto& El : List)
                {
                    m_pCurrPtr[m_count++] = El;
                }
            }
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        void TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::_DestroyElements(DataTypePtr pData)
        {
            assert(pData);
            for( uint32_t i = m_count; i-- > 0; )
            {
                pData[ i ].~DataType();
            }
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        void TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Destroy()
        {
            if (m_pData)
            {
                _DestroyElements(m_pData);
                Memory::FreeMemory(&m_Allocator, &m_pData);
                //delete[] m_pData;
                //m_pData = nullptr;
            }
            m_count = 0;
            m_capacity = 0;
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        bool TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Copy(TCArrayContainer* pOut) const
        {
            assert(pOut);
            if (this == pOut || GetCount() == 0)
            {
                return true;
            }
            
            if (pOut->Reserve(GetCount()))
            {
                DataTypePtr pData = pOut->m_pCurrPtr;
                pOut->m_count = GetCount();
                Memory::Copy(pData, pOut->GetCapacity(), m_pCurrPtr, CalcSize());
                return true;
            }
            return false;
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        void TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Move(TCArrayContainer* pOut)
        {
            assert(pOut);
            if (this == pOut)
            {
                return;
            }

            m_pData = pOut->m_pData;
            m_pCurrPtr = pOut->m_pCurrPtr;

            m_capacity = pOut->GetCapacity();
            m_count = pOut->GetCount();

            pOut->Destroy();
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        bool TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Reserve(CountType elemCount)
        {
            const SizeType newSize = elemCount * sizeof(DataType);
            if (newSize > m_capacity)
            {

                Destroy();

                //m_pData = Memory::CreateObjects(&m_Allocator, &m_pData, elemCount);
                //m_pData = new(std::nothrow) DataType[elemCount];
                if( VKE_SUCCEEDED( Memory::CreateObjects( &m_Allocator, &m_pData, elemCount ) ) )
                {
                    m_count = 0;
                    m_capacity = newSize;
                    m_pCurrPtr = m_pData;
                    return true;
                }

                return false;
            }

            m_count = 0;
            return true;
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        bool TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Resize(CountType newElemCount)
        {
            const auto newCapacity = newElemCount * sizeof(DataType);
            if (newCapacity > m_capacity)
            {
                TCArrayContainer Tmp;
                if (Copy(&Tmp))
                {
                    const auto count = Policy::Resize::Calc(newElemCount);
                    if (Reserve(count))
                    {
                        if (Tmp.Copy(this))
                        {
                            m_count = newElemCount;
                            return true;
                        }
                    }
                }
                return false;
            }
            m_count = newElemCount;
            return true;
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        bool TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Resize(
            CountType newElemCount,
            const DataTypeRef Default)
        {
            if (Resize(newElemCount))
            {
                for (uint32_t i = m_count; i-- > 0;)
                {
                    m_pCurrPtr[i] = Default;
                }
                return true;
            }
            return false;
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        bool TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Resize(
            CountType newElemCount,
            VisitCallback Callback)
        {
            if (Resize(newElemCount))
            {
                for (uint32_t i = m_count; i-- > 0;)
                {
                    Callback(i, m_pCurrPtr[i]);
                }
                return true;
            }
            return false;
        }

    } // utils
} // VKE