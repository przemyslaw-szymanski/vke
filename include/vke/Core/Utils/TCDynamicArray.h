#pragma once

#include "TCContainerBase.h"

namespace VKE
{
    namespace Utils
    {

#define TC_DYNAMIC_ARRAY_TEMPLATE \
        template \
        < \
            typename DataType, \
            uint32_t DEFAULT_ELEMENT_COUNT, \
            class AllocatorType, \
            class Policy, \
            class Utils \
        >

#define TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS \
        DataType, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy, Utils


#define TC_DYNAMIC_ARRAY_TEMPLATE2 \
        template \
        < \
            typename DataType2, \
            uint32_t DEFAULT_ELEMENT_COUNT2, \
            class AllocatorType2, \
            class Policy2, \
            class Utils2 \
        >

#define TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2 \
        DataType2, DEFAULT_ELEMENT_COUNT2, AllocatorType2, Policy2, Utils2

        struct DynamicArrayDefaultPolicy
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

            // On PushBack
            struct PushBack
            {
                static uint32_t Calc(uint32_t current) { return current * 2; }
            };

            // On Remove
            struct Remove
            {
            };
        };

        struct DynamicArrayDefaultUtils : public ArrayContainerDefaultUtils
        {

        };

        template
        <
            typename T,
            uint32_t DEFAULT_ELEMENT_COUNT = 32,
            class AllocatorType = Memory::CHeapAllocator,
            class Policy = DynamicArrayDefaultPolicy,
            class Utils = DynamicArrayDefaultUtils
        >
        class TCDynamicArray : public TCArrayContainer< T, AllocatorType, Policy, Utils >
        {
            using Base = TCArrayContainer< T, AllocatorType, Policy, Utils >;
            public:

                static_assert( DEFAULT_ELEMENT_COUNT > 0, "DEFAULT_ELEMENT_COUNT must be greater than 0." );
                using DataType = T;
                using DataTypePtr = T*;
                using DataTypeRef = T&;
                using SizeType = uint32_t;
                using CountType = uint32_t;
                using AllocatorPtr = Memory::IAllocator*;

                using Iterator = TCArrayIterator< DataType >;
                using ConstIterator = TCArrayIterator< const DataType >;

                template<uint32_t COUNT, class AllocatorType, class Policy>
                using TCOtherSizeArray = TCDynamicArray< T, COUNT, AllocatorType, Policy >;

            public:

                TCDynamicArray()
                {
                    this->m_capacity = sizeof( m_aData );
                    this->m_pCurrPtr = m_aData;
                    this->m_resizeElementCount = DEFAULT_ELEMENT_COUNT;
                }

                TCDynamicArray(std::initializer_list<DataType> List);

                explicit TCDynamicArray(const uint32_t& count) :
                    TCDynamicArray()
                {
                    const auto res = Resize( count );
                    VKE_ASSERT( res, "" );
                }

                explicit TCDynamicArray(const uint32_t& count, const DataType& DefaultValue) :
                    TCDynamicArray()
                {
                    const auto res = Resize( count, DefaultValue );
                    VKE_ASSERT( res, "" );
                }

                //TCDynamicArray(uint32_t count, VisitCallback&& Callback) :
                //    //TCDynamicArray(),
                //    TCArrayContainer(count, Callback)
                //{
                //    auto res = Resize( count, Callback );
                //    VKE_ASSERT( res, "" );
                //}

                TCDynamicArray(const TCDynamicArray& Other);
                TCDynamicArray(TCDynamicArray&& Other);

                TC_DYNAMIC_ARRAY_TEMPLATE2
                TCDynamicArray(const TCDynamicArray< TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2 >& Other);
                TC_DYNAMIC_ARRAY_TEMPLATE2
                TCDynamicArray(TCDynamicArray< TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2 >&& Other);

                virtual ~TCDynamicArray()
                {
                    Destroy();
                }

                /*SizeType GetCapacity() const { return m_capacity; }
                CountType GetCount() const { return m_count; }
                SizeType CalcSize() const { return m_count * sizeof(DataType); }*/
                SizeType GetMaxCount() const { return m_resizeElementCount; }

                template<EVENT_REPORT_TYPE = EventReportTypes::NONE>
                uint32_t PushBack(const DataType& el);
                template<EVENT_REPORT_TYPE = EventReportTypes::NONE>
                uint32_t PushBack(DataType&& el);
                bool PopBack(DataTypePtr pOut);
                template<bool DestructObject = true>
                bool PopBack();

                DataType PopBackFast(); // do not check for emptyness

                bool Resize();
                bool Resize(CountType newElemCount);
                bool Resize(CountType newElemCount, const DataType& DefaultData);

                bool Reserve(CountType elemCount);
                bool grow(CountType newElemCount, bool Resize = false);
                bool shrink(CountType newElemCount, bool Resize = false);
                void Remove(CountType elementIdx);
                void RemoveFast(CountType elemtnIdx);

                template<bool DestroyElements = true>
                void _Clear();
                void Clear() { _Clear<false>(); }
                void ClearFull() { _Clear<true>(); }
                void Destroy();

                //bool Copy(const TCDynamicArray& Other);
                void Move(TCDynamicArray* pOut);
                bool Append(const TCDynamicArray& Other) { return Append(0, Other.GetCount(), Other); }
                bool Append(CountType begin, CountType end, const TCDynamicArray& Other);
                bool Append(CountType begin, CountType end, const DataType* pData);
                bool Append(CountType count, const DataType* pData);

                bool IsInConstArrayRange(SizeType size) const { return size <= sizeof(m_aData); }

                Iterator begin() { return Iterator(this->m_pCurrPtr, this->m_pCurrPtr + this->m_count); }
                Iterator end() { return Iterator(this->m_pCurrPtr + this->m_count, this->m_pCurrPtr + this->m_count); }
                ConstIterator begin() const { return ConstIterator(this->m_pCurrPtr, this->m_pCurrPtr + this->m_count); }
                ConstIterator end() const { return ConstIterator(this->m_pCurrPtr + this->m_count, this->m_pCurrPtr + this->m_count); }

                TCDynamicArray& operator=(const TCDynamicArray& Other)
                {
                    Copy( Other );
                    return *this;
                }

                TCDynamicArray& operator=(TCDynamicArray&& Other)
                {
                    Move( &Other );
                    return *this;
                }

            protected:

                template<EVENT_REPORT_TYPE>
                void _ReportPushBack(const uint32_t oldCount, const uint32_t newCount);

                void _SetCurrPtr();

            protected:

                DataType    m_aData[ DEFAULT_ELEMENT_COUNT ];
                CountType   m_resizeElementCount = DEFAULT_ELEMENT_COUNT;
        };

        TC_DYNAMIC_ARRAY_TEMPLATE
        TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::TCDynamicArray(const TCDynamicArray& Other) :
            TCDynamicArray()
        {
            const auto res = Copy( Other );
            VKE_ASSERT( res, "" );
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::TCDynamicArray(TCDynamicArray&& Other) :
            TCDynamicArray()
        {
            Move( &Other );
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        TC_DYNAMIC_ARRAY_TEMPLATE2
        TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::TCDynamicArray(
            const TCDynamicArray< TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2 >& Other) :
            TCDynamicArray()
        {
            const auto res = Copy( Other );
            VKE_ASSERT(res, "" );
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        TC_DYNAMIC_ARRAY_TEMPLATE2
        TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::TCDynamicArray(
            TCDynamicArray< TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2 >&& Other) :
            TCDynamicArray()
        {
            Move( &Other );
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::TCDynamicArray(std::initializer_list<DataType> List) :
            TCDynamicArray()
        {
            const auto count = static_cast<CountType>(List.size());
            if (count <= m_resizeElementCount)
            {
                for (auto& El : List)
                {
                    m_aData[m_count++] = El;
                }
            }
            else
            {
                const auto newMaxCount = Policy::Reserve::Calc(count);
                Reserve( newMaxCount );
                for (auto& El : List)
                {
                    m_pData[m_count++] = El;
                }
            }
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Destroy()
        {
            TCArrayContainer::Destroy();
            if( this->m_resizeElementCount )
            {
                this->_DestroyElements( m_aData, DEFAULT_ELEMENT_COUNT );
            }
            this->m_pCurrPtr = m_aData;
            m_capacity = sizeof( m_aData );
            m_resizeElementCount = 0;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        template<bool DestroyElements>
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::_Clear()
        {
            VKE_ASSERT(this->m_pCurrPtr, "" );
            if( DestroyElements )
            {
                this->_DestroyElements(this->m_pCurrPtr, this->m_count);
            }
            m_count = 0;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Move(TCDynamicArray* pOut)
        {
            VKE_ASSERT( this->m_pCurrPtr, "" );
            VKE_ASSERT( pOut, "" );
            if( this == pOut )
            {
                return;
            }

            const SizeType srcCapacity = pOut->GetCount() * sizeof( DataType );
            if( IsInConstArrayRange( srcCapacity ) )
            {
                // Copy
                for( CountType i = 0; i < pOut->GetCount(); ++i )
                {
                    this->m_aData[ i ] = std::move( pOut->m_pCurrPtr[ i ] );
                }
                this->m_pCurrPtr = m_aData;
            }
            else
            {
                m_pData = pOut->m_pData;
                this->m_pCurrPtr = m_pData;
            }
            m_capacity = pOut->GetCapacity();
            m_count = pOut->GetCount();
            m_resizeElementCount = pOut->GetMaxCount();

            pOut->m_pData = nullptr;
            pOut->m_pCurrPtr = pOut->m_aData;
            pOut->m_count = 0;
            pOut->m_capacity = sizeof( pOut->m_aData );
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Reserve(CountType elemCount)
        {
            VKE_ASSERT( this->m_pCurrPtr, "" );
            if( TCArrayContainer::Reserve( elemCount ) )
            {
                if( this->m_pData )
                {
                    this->m_pCurrPtr = this->m_pData;
                }
                else
                {
                    this->m_pCurrPtr = m_aData;
                }
                return true;
            }
            return false;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Resize(CountType newElemCount)
        {
            VKE_ASSERT(this->m_pCurrPtr, "" );
            bool res = true;
            if( m_resizeElementCount < newElemCount )
            {
                res = TCArrayContainer::Resize( newElemCount );
                if( res )
                {
                    m_resizeElementCount = newElemCount;
                    this->m_pCurrPtr = this->m_pData;
                    /*if( this->m_pCurrPtr != this->m_aData )
                    {
                        Memory::Zero( m_aData, DEFAULT_ELEMENT_COUNT );
                    }*/
                }
            }
            else
            {
                this->m_count = newElemCount;
            }
            return res;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Resize(
            CountType newElemCount,
            const DataType& Default)
        {
            if (Resize(newElemCount))
            {
                for (uint32_t i = m_count; i-- > 0;)
                {
                    this->m_pCurrPtr[i] = Default;
                }
                return true;
            }
            return false;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Resize()
        {
            return Resize(DEFAULT_ELEMENT_COUNT);
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        template<EVENT_REPORT_TYPE EventReportType>
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::_ReportPushBack(const uint32_t oldCount,
                                                                                const uint32_t newCount)
        {
            if( EventReportType != EventReportTypes::NONE )
            {
                char buff[ 256 ];
                vke_sprintf( buff, sizeof( buff ), "Resize: %d -> %d", oldCount, newCount );
                if( EventReportType == EventReportTypes::ASSERT_ON_ALLOC )
                {
                    VKE_ASSERT( false, buff );
                }
            }
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        template<EVENT_REPORT_TYPE EventReportType>
        uint32_t TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::PushBack(const DataType& El)
        {
            if( this->m_count < m_resizeElementCount )
            {
                //this->m_pCurrPtr[m_count++] = El;
                auto& Element = this->m_pCurrPtr[ m_count++ ];
                Element = El;
            }
            else
            {
                // Need Resize
                const auto lastCount = m_count;
                const auto count = Policy::PushBack::Calc( m_resizeElementCount );
                if( Resize( count ) )
                {
                    _ReportPushBack< EventReportType >( lastCount, count );
                    m_resizeElementCount = m_count;
                    this->m_count = lastCount;
                    return PushBack( El );
                }
                return INVALID_POSITION;
            }
            return this->m_count - 1;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        template<EVENT_REPORT_TYPE EventReportType>
        uint32_t TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::PushBack(DataType&& El)
        {
            if( this->m_count < m_resizeElementCount )
            {
                //this->m_pCurrPtr[m_count++] = El;
                auto& Element = this->m_pCurrPtr[ m_count++ ];
                Element = std::move( El );
            }
            else
            {
                // Need Resize
                const auto lastCount = m_count;
                const auto count = Policy::PushBack::Calc( m_resizeElementCount );
                if( Resize( count ) )
                {
                    _ReportPushBack< EventReportType >( lastCount, count );
                    m_resizeElementCount = m_count;
                    m_count = lastCount;
                    return PushBack( El );
                }
                return INVALID_POSITION;
            }
            return m_count - 1;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::PopBack(DataTypePtr pOut)
        {
            VKE_ASSERT(pOut, "" );
            if( !this->IsEmpty() )
            {
                *pOut = Back();
                this->m_count--;
                return true;
            }
            return false;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        template<bool DestructObject>
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::PopBack()
        {
            if( !this->IsEmpty() )
            {
                this->m_count--;
                return true;
            }
            return false;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        DataType TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::PopBackFast()
        {
            VKE_ASSERT( !IsEmpty(), "Container is empty." );
            DataType ret = Back();
            this->m_count--;
            return  ret;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Append(
            CountType begin, CountType end, const TCDynamicArray& Other)
        {
            return Append(begin, end, Other.m_pCurrPtr);
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
            bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Append(
            CountType count, const DataType* pData)
        {
            const auto currCount = GetCount();
            return Append(0, count, pData);
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Append(CountType begin, CountType end,
                                                                      const DataType* pData)
        {
            VKE_ASSERT(begin <= end, "" );
            const auto count = end - begin;
            if( count )
            {
                const auto lastCount = this->GetCount();
                if( lastCount + count > GetMaxCount() )
                {
                    const auto newCount = Policy::PushBack::Calc( GetMaxCount() + count );
                    if( !Resize( newCount ) )
                    {
                        return false;
                    }
                    this->m_count = lastCount;
                }
                const auto dstSize = this->m_capacity - this->m_count * sizeof(DataType);
                const auto bytesToCopy = count * sizeof(DataType);
                DataTypePtr pCurrPtr = this->m_pCurrPtr + this->m_count;
                Memory::Copy(pCurrPtr, dstSize, pData + begin, bytesToCopy);
                this->m_count += count;
                return true;
            }
            return true;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Remove(CountType elementIdx)
        {
            const auto dstSize = m_capacity - sizeof(DataType);
            const auto sizeToCopy = (m_resizeElementCount - 1) * sizeof(DataType);
            Memory::Copy(this->m_pCurrPtr + elementIdx, dstSize, this->m_pCurrPtr + elementIdx + 1, sizeToCopy);
            this->m_count--;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::RemoveFast(CountType elementIdx)
        {
            this->m_pCurrPtr[elementIdx] = Back();
            m_count--;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::_SetCurrPtr()
        {
            if (this->m_pData != nullptr)
            {
                this->m_pCurrPtr = this->m_pData;
            }
            else
            {
                this->m_pCurrPtr = this->m_aData;
            }
        }

        template
        <
            typename T,
            typename HandleType = uint32_t,
            uint32_t DEFAULT_ELEMENT_COUNT = 32,
            class AllocatorType = Memory::CHeapAllocator,
            class Policy = DynamicArrayDefaultPolicy
        >
        struct TSFreePool
        {
            static_assert( std::is_unsigned<HandleType>::value, "HandleType must be of unsigned type." );
            using Array = Utils::TCDynamicArray< T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy >;
            using HandleArray = Utils::TCDynamicArray< HandleType, DEFAULT_ELEMENT_COUNT >;
            Array       vPool;
            HandleArray vFreeElements;

            void Clear()
            {
                vPool.Clear();
                vFreeElements.Clear();
            }

            void FullClear()
            {
                vPool.ClearFull();
                vFreeElements.ClearFull();
            }

            HandleType Add( const T& element )
            {
                HandleType ret;
                if( GetFreeHandle( &ret ) )
                {
                    vPool[ret] = ( element );
                }
                else
                {
                    ret = static_cast< HandleType >( vPool.PushBack( ( element ) ) );
                }
                return ret;
            }

            HandleType Add( T&& element )
            {
                HandleType ret;
                if( GetFreeHandle( &ret ) )
                {
                    vPool[ ret ] = std::move( element );
                }
                else
                {
                    ret = static_cast< HandleType >( vPool.PushBack( std::move( element ) ) );
                }
                return ret;
            }

            void Free( const HandleType& handle )
            {
                vFreeElements.PushBack( handle );
            }

            bool GetFreeHandle(HandleType* phOut)
            {
                return vFreeElements.PopBack( phOut );
            }

            uint32_t GetHandle()
            {
                uint32_t ret;
                if( !GetFreeHandle( &ret ) )
                {
                    ret = Add( {} );
                }
                return ret;
            }

            T& operator[](const HandleType& handle) { return vPool[handle]; }
            const T& operator[]( const HandleType& handle ) const { return vPool[handle]; }
        };

    } // Utils
} // VKE