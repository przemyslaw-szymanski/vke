#pragma once

#include "Common.h"
#include "Core/Memory/Memory.h"
#include "Core/VKEConfig.h"

namespace VKE
{
    namespace Utils
    {
        static const uint32_t INVALID_POSITION = static_cast<const uint32_t>(~0);

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
                VKE_ASSERT(m_pCurr < m_pEnd, "Out-of-bounds iterator increment");
                m_pCurr++;
                return *this;
            }

            TCArrayIterator operator++(int)
            {
                VKE_ASSERT(m_pCurr < m_pEnd , "Out-of-bounds iterator increment");
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

        struct ArrayContainerDefaultUtils
        {
            template<typename DataType>
            static int32_t Find(const DataType* pBuffer, int32_t elemCount, const DataType& find)
            {
                VKE_ASSERT(pBuffer, "");
                for( int32_t i = 0; i < elemCount; ++i )
                {
                    const DataType& el = pBuffer[ i ];
                    if( el == find )
                        return i;
                }
                return INVALID_POSITION;
            }
        };

#define TC_ARRAY_CONTAINER_TEMPLATE \
    typename DataType, class AllocatorType, class Policy, class Utils

#define TC_ARRAY_CONTAINER_TEMPLATE2 \
    template< typename DataType2, class AllocatorType2, class Policy2, class Utils2 >


#define TC_ARRAY_CONTAINER_TEMPLATE_PARAMS \
    DataType, AllocatorType, Policy, Utils

#define TC_ARRAY_CONTAINER_TEMPLATE_PARAMS2 \
    DataType2, AllocatorType2, Policy2, Utils2

        template
        <
            typename T,
            class AllocatorType = Memory::CHeapAllocator,
            class Policy = ArrayContainerDefaultPolicy,
            class Utils = ArrayContainerDefaultUtils
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

                static const uint32_t NPOS = UNDEFINED_U32;

                using CompareFunc = std::function<bool(const DataType&, const DataType&)>;

            public:

                TCArrayContainer() {}
                TCArrayContainer(const TCArrayContainer& Other);
                TCArrayContainer(TCArrayContainer&& Other);
                TCArrayContainer(std::initializer_list<DataType> List);

                explicit TCArrayContainer(uint32_t count)
                {
                    auto res = Resize(count);
                    VKE_ASSERT(res == true, "" );
                }

                TCArrayContainer(uint32_t count, const DataTypeRef DefaultValue)
                {
                    auto res = Resize(count, DefaultValue);
                    VKE_ASSERT(res == true, "" );
                }

                TCArrayContainer(uint32_t count, VisitCallback&& Callback)
                {
                    auto res = Resize(count, Callback);
                    VKE_ASSERT(res == true, "" );
                }

                virtual ~TCArrayContainer() { Destroy(); }

                void Destroy();

                SizeType GetCapacity() const { return m_capacity; }
                CountType GetCount() const { return m_count; }
                SizeType CalcSize() const { return m_count * sizeof( DataType ); }
                bool IsEmpty() const { return GetCount() == 0; }

                const DataTypePtr GetData() const { return m_pCurrPtr; }

                bool Reserve(CountType elemCount);
                bool Resize(CountType newElemCount);
                bool Resize(CountType newElemCount, const DataTypeRef DefaultData);
                bool Resize(CountType newElemCount, VisitCallback Callback);

                DataTypeRef Back() { return At(m_count - 1); }
                const DataTypeRef Back() const { return At(m_count - 1); }
                DataTypeRef Front() { return At(0); }
                const DataTypeRef Front() const { return At(0); }

                bool Copy(const TCArrayContainer& Other) { return Copy( Other.GetCount(), Other.m_pCurrPtr ); }
                bool Copy(const CountType count, const DataTypePtr pData);
                void Move(TCArrayContainer* pOut);
                bool Insert(CountType pos, const TCArrayContainer& Other)
                {
                    return Insert( pos, 0, Other.GetCount(), Other.GetData() );
                }
                bool Insert(CountType pos, CountType begin, CountType count, const TCArrayContainer& Other)
                {
                    return Insert( pos, begin, count, Other.GetData() );
                }
                bool Insert(CountType pos, CountType begin, CountType count, const DataType* pData);

                template< TC_ARRAY_CONTAINER_TEMPLATE >
                bool Compare(const TCArrayContainer< TC_ARRAY_CONTAINER_TEMPLATE_PARAMS >& Other) const
                {
                    return Compare( Other.GetCount(), Other.GetData() );
                }
                bool Compare(const CountType count, const DataTypePtr pData) const;

                template<typename IndexType>
                DataTypeRef At(const IndexType& index) { return _At(m_pCurrPtr, index); }
                template<typename IndexType>
                const DataTypeRef At(const IndexType& index) const { return _At(m_pCurrPtr, index); }

                uint32_t Find(const DataType& data) const { return Utils::Find(m_pCurrPtr, m_count, data); }
                vke_force_inline
                static const uint32_t Npos() { return NPOS; }

                iterator begin() { return iterator(m_pCurrPtr, m_pCurrPtr + m_count); }
                iterator end() { return iterator(m_pCurrPtr + m_count, m_pCurrPtr + m_count); }
                const_iterator begin() const { return const_iterator(m_pCurrPtr, m_pCurrPtr + m_count); }
                const_iterator end() const { return const_iterator(m_pCurrPtr + m_count, m_pCurrPtr + m_count); }

                CountType Find(const CountType pos, const DataType& Element, CompareFunc Cmp);
                CountType FindLast(const DataType& Element, CompareFunc Cmp);
                CountType FindNot(const CountType pos, const DataType& Element, CompareFunc Cmp);
                CountType FindLastNot(const DataType& Element, CompareFunc Cmp);

                TCArrayContainer& operator=(const TCArrayContainer& Other) { Copy( Other ); return *this; }
                TCArrayContainer& operator=(TCArrayContainer&& Other) { Move( &Other ); return *this; }
                template< TC_ARRAY_CONTAINER_TEMPLATE >
                bool operator==(const TCArrayContainer< TC_ARRAY_CONTAINER_TEMPLATE_PARAMS >& Other) const
                {
                    return Compare( Other );
                }

                // Set default values to all array element
                void Reset(const DataType& value)
                {
                    memset(m_pCurrPtr, value, m_count * sizeof(DataType));
                }

                template<typename IndexType>
                DataTypeRef operator[](const IndexType& index) { return At(index); }
                template<typename IndexType>
                const DataTypeRef operator[](const IndexType& index) const { return At(index); }

            protected:

                template<typename IndexType>
                vke_force_inline
                DataTypeRef _At(DataTypePtr pPtr, const IndexType& idx)
                {
                    static_assert( std::numeric_limits< IndexType >::is_integer ||
                        std::is_enum< IndexType >::value, "IndexType must be representable as integer" );
                    VKE_ASSERT( pPtr, "" );
                    VKE_ASSERT( (idx >= static_cast<IndexType>(0) && (uint32_t)idx < (m_count)), "Element out of bounds." );
                    return pPtr[ idx ];
                }

                template<typename IndexType>
                vke_force_inline
                const DataTypeRef _At(DataTypePtr pPtr, const IndexType& idx) const
                {
                    static_assert( std::numeric_limits< IndexType >::is_integer ||
                        std::is_enum< IndexType >::value,
                        "IndexType must be representable as integer" );
                    VKE_ASSERT( pPtr, "" );
                    VKE_ASSERT( (idx >= static_cast<IndexType>(0) && (uint32_t)idx < (m_count)), "Element out of bounds." );
                    return pPtr[ idx ];
                }

                bool _Copy(DataTypePtr* ppDstOut, CountType* pDstCountInOut, CountType dstCapacity) const;

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
            auto res = Copy( Other );
            VKE_ASSERT(res, "" );
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::TCArrayContainer(TCArrayContainer&& Other)
        {
            auto res = Move( &Other );
            VKE_ASSERT(res, "" );
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::TCArrayContainer(
            std::initializer_list<DataType> List)
        {
            const auto count = static_cast< CountType >( List.size() );
            if( count > 0 )
            {
                Reserve( count );
                for( auto& El : List )
                {
                    m_pCurrPtr[ m_count++ ] = El;
                }
            }
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        void TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::_DestroyElements(DataTypePtr pData)
        {
            VKE_ASSERT(pData, "" );
            for( uint32_t i = m_count; i-- > 0; )
            {
                pData[ i ].~DataType();
            }
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        void TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Destroy()
        {
            if( m_pData )
            {
                _DestroyElements( m_pData );
                Memory::FreeMemory( &m_Allocator, &m_pData );
                //delete[] m_pData;
                //m_pData = nullptr;
            }
            m_count = 0;
            m_capacity = 0;
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        bool TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Copy(
            const CountType count, const DataTypePtr pData)
        {
            if( this->m_pCurrPtr == pData || count == 0 )
            {
                return true;
            }

            if( Reserve( count ) )
            {
                m_count = count;
                for( CountType i = 0; i < count; ++i)
                {
                    m_pCurrPtr[ i ] = pData[ i ];
                }
                return true;
            }
            return false;
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        void TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Move(TCArrayContainer* pOut)
        {
            VKE_ASSERT(pOut, "" );
            if (this == pOut)
            {
                return;
            }

            m_pData = pOut->m_pData;
            m_pCurrPtr = pOut->m_pCurrPtr;

            m_capacity = pOut->GetCapacity();
            m_count = pOut->GetCount();

            pOut->m_pData = nullptr;
            pOut->m_pCurrPtr = pOut->m_pData;
            pOut->m_count = 0;
            pOut->m_capacity = 0;
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        bool TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Reserve(CountType elemCount)
        {
            const SizeType newSize = elemCount * sizeof( DataType );
            if( newSize > m_capacity )
            {
                Destroy();
                //m_pData = Memory::CreateObjects(&m_Allocator, &m_pData, elemCount);
                //m_pData = new(std::nothrow) DataType[elemCount];
                const uint32_t newCount = elemCount;
                if( VKE_SUCCEEDED( Memory::CreateObjects( &m_Allocator, &m_pData, newCount ) ) )
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
            const auto newCapacity = newElemCount * sizeof( DataType );
            if( newCapacity > m_capacity )
            {
                TCArrayContainer Tmp;
                if( Tmp.Copy( *this ) )
                {
                    const auto count = newElemCount;
                    if( Reserve( count ) )
                    {
                        if( Copy( Tmp ) )
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

            {
                for( uint32_t i = 0; i < newElemCount; ++i )
                {
                    m_pCurrPtr[ i ] = Default;
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
            if( Resize( newElemCount ) )
            {
                for( uint32_t i = 0; i < newElemCount; ++i )
                {
                    Callback( i, m_pCurrPtr[ i ] );
                }
                return true;
            }
            return false;
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        bool TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Insert(CountType pos, CountType begin,
                CountType count, const DataType* pData)
        {
            VKE_ASSERT( pos <= this->GetCount(), "" );

            const auto countToCopy = count;
            const auto sizeToCopy = countToCopy * sizeof( DataType );
            const auto sizeLeft = this->GetCapacity() - ( pos * sizeof( DataType ) );

            // If there is no space left
            if( sizeLeft < sizeToCopy )
            {
                const auto oldCount = this->GetCount();
                const auto newCount = oldCount + pos + countToCopy;
                const auto resizeCount = Policy::Resize::Calc( newCount );
                if( !Resize( newCount ) )
                {
                    return false;
                }
                // Override current count
                this->m_count = oldCount;
                return Insert( pos, begin, count, pData );
            }
            // Move elements
            const DataType* pSrc = this->m_pCurrPtr + pos;
            DataType* pDst = this->m_pCurrPtr + pos + countToCopy;
            const auto dstSize = sizeLeft - sizeToCopy;
            const auto srcSize = ( this->GetCount() - pos ) * sizeof( DataType );
            Memory::Copy( pDst, dstSize, pSrc, srcSize );
            this->m_count += countToCopy;
            // Copy new elements
            pDst = this->m_pCurrPtr + pos;
            pSrc = pData + begin;
            for( CountType i = 0; i < countToCopy; ++i )
            {
                pDst[ i ] = pSrc[ i ];
            }
            return true;
        }

        template< TC_ARRAY_CONTAINER_TEMPLATE >
        bool TCArrayContainer<TC_ARRAY_CONTAINER_TEMPLATE_PARAMS>::Compare(const CountType count,
            const DataTypePtr pData) const
        {
            bool ret = false;
            if( count == m_count )
            {
                for( CountType i = count; i-- > 0; )
                {
                    if( pData[i] != m_pCurrPtr[i] )
                    {
                        break;
                    }
                }
                ret = true;
            }
            return ret;
        }

    } // utils
} // VKE