#pragma once

#include "TCContainerBase.h"

namespace VKE
{
    namespace Utils
    {

#define CONSTANT_ARRAY_TEMPLATE \
        template \
        < \
            typename DataType, \
            uint32_t DEFAULT_ELEMENT_COUNT, \
            class AllocatorType, \
            class Policy, \
            class Utils \
        >

#define TC_CONSTANT_ARRAY_TEMPLATE_PARAMS \
        DataType, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy, Utils

        struct ConstantArrayDefaultPolicy
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

        struct ConstantArrayDefaultUtils : public ArrayContainerDefaultUtils
        {

        };

        template
        <
            typename T,
            uint32_t DEFAULT_ELEMENT_COUNT = 32,
            class AllocatorType = Memory::CHeapAllocator,
            class Policy = ConstantArrayDefaultPolicy,
            class Utils = ConstantArrayDefaultUtils
        >
        class TCConstantArray : public TCArrayContainer< T, AllocatorType, Policy, Utils >
        {
            using Base = TCArrayContainer< T, AllocatorType, Policy, Utils >;
            public:

                using DataType = T;
                using DataTypePtr = T*;
                using DataTypeRef = T&;
                using SizeType = uint32_t;
                using CountType = uint32_t;
                using AllocatorPtr = Memory::IAllocator*;

                using Iterator = TCArrayIterator< DataType >;
                using ConstIterator = TCArrayIterator< const DataType >;

                template<uint32_t COUNT, class AllocatorType, class Policy>
                using TCOtherSizeArray = TCConstantArray< T, COUNT, AllocatorType, Policy >;

            public:

                TCConstantArray()
                {
                    this->m_capacity = sizeof(m_aData);
                    this->m_pCurrPtr = m_aData;
                }

                explicit TCConstantArray(uint32_t count) :
                    TCConstantArray()
                {
                    auto res = Resize( count );
                    assert( res );
                }

                TCConstantArray(uint32_t count, const DataTypeRef DefaultValue) :
                    TCConstantArray()
                {
                    auto res = Resize( count, DefaultValue );
                    assert( res );
                }

                /*TCConstantArray(uint32_t count, VisitCallback&& Callback) :
                    TCConstantArray(),
                    TCArrayContainer(count, Callback),
                {
                    auto res = Resize( count, Callback );
                    assert( res );
                }*/

                TCConstantArray(const TCConstantArray& Other);
                TCConstantArray(TCConstantArray&& Other);

                TCConstantArray(std::initializer_list<DataType> List);

                virtual ~TCConstantArray()
                {
                    Destroy();
                }

                /*SizeType GetCapacity() const { return m_capacity; }
                CountType GetCount() const { return m_count; }
                SizeType CalcSize() const { return m_count * sizeof(DataType); }*/
                SizeType GetMaxCount() const { return this->m_maxElementCount; }


                uint32_t PushBack(const DataType& el);
                bool PopBack(DataTypePtr pOut);
                template<bool DestructObject = true>
                bool PopBack();

                bool Resize();
                bool Resize(CountType newElemCount);
                bool Resize(CountType newElemCount, const DataType& DefaultData);
                //template<typename VisitCallback>
                //bool Resize(CountType newElemCount, VisitCallback&& Callback);
                bool Reserve(CountType elemCount);
                bool grow(CountType newElemCount, bool Resize = false);
                bool shrink(CountType newElemCount, bool Resize = false);
                void Remove(CountType elementIdx);
                void RemoveFast(CountType elemtnIdx);

                template<typename IndexType>
                DataTypeRef At(const IndexType& index) { return this->_At(this->m_pCurrPtr, index); }
                template<typename IndexType>
                const DataTypeRef At(const IndexType& index) const { return this->_At(this->m_pCurrPtr, index); }
                template<typename IndexType>
                DataTypeRef operator[](const IndexType& index) { return At(index); }
                template<typename IndexType>
                const DataTypeRef operator[](const IndexType& index) const { return At(index); }

                template<bool DestroyElements = true>
                void Clear();
                void FastClear() { Clear<false>(); }
                void Destroy();

                bool Copy(TCConstantArray* pOut) const;
                void Move(TCConstantArray* pOut);
                bool Append(const TCConstantArray& Other) { return Append(Other, 0, Other.GetCount()); }
                bool Append(CountType begin, CountType end, const TCConstantArray& Other);
                bool Append(CountType begin, CountType end, const DataTypePtr pData);
                bool Append(CountType count, const DataTypePtr pData);

                bool IsInConstArrayRange() const { return this->m_capacity < sizeof( m_aData ); }

                TCConstantArray& operator=(const TCConstantArray& Other) { Other.Copy(this); return *this; }
                TCConstantArray& operator=(TCConstantArray&& Other) { Other.Move(this); return *this; }

                Iterator begin() { return Iterator(this->m_pCurrPtr, this->m_pCurrPtr + this->m_count); }
                Iterator end() { return Iterator(this->m_pCurrPtr + this->m_count, this->m_pCurrPtr + this->m_count); }
                ConstIterator begin() const { return ConstIterator(this->m_pCurrPtr, this->m_pCurrPtr + this->m_count); }
                ConstIterator end() const { return ConstIterator(this->m_pCurrPtr + this->m_count, this->m_pCurrPtr + this->m_count); }

            public:

                DataType        m_aData[DEFAULT_ELEMENT_COUNT];
        };

        CONSTANT_ARRAY_TEMPLATE
        TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::TCConstantArray(const TCConstantArray& Other) :
            TCConstantArray()
        {
            auto res = Other.Copy(this);
            assert(res);
        }

        CONSTANT_ARRAY_TEMPLATE
        TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::TCConstantArray(TCConstantArray&& Other) :
            TCConstantArray()
        {
            Other.Move(this);
        }

        CONSTANT_ARRAY_TEMPLATE
        TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::TCConstantArray(std::initializer_list<DataType> List) :
            TCConstantArray()
        {
            const auto count = static_cast<CountType>(List.size());
            if (count < DEFAULT_ELEMENT_COUNT)
            {
                for (auto& El : List)
                {
                    this->m_aData[ this->m_count++ ] = El;
                }
            }
            else
            {
                const auto newMaxCount = Policy::Reserve::Calc(count);
                Reserve( newMaxCount );
                for (auto& El : List)
                {
                    this->m_pData[ this->m_count++ ] = El;
                }
            }
        }

        CONSTANT_ARRAY_TEMPLATE
        void TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Destroy()
        {
            VKE_ASSERT2(this->m_pCurrPtr, "Data pinter should points to m_aData");
            TCArrayContainer::Destroy();
            this->m_pCurrPtr = m_aData;
            this->m_capacity = sizeof( m_aData );
        }

        CONSTANT_ARRAY_TEMPLATE
        template<bool DestroyElements>
        void TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Clear()
        {
            VKE_ASSERT2( this->m_pCurrPtr, "Data pinter should points to m_aData" );
            if constexpr( DestroyElements )
            {
                this->_DestroyElements(this->m_pCurrPtr);
            }
            this->m_count = 0;
        }

        CONSTANT_ARRAY_TEMPLATE
        bool TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Copy(TCConstantArray* pOut) const
        {
            VKE_ASSERT2( this->m_pCurrPtr, "Data pinter should points to m_aData" );
            assert(pOut);
            if( this == pOut )
            {
                return true;
            }
            // Do not perform any copy operations if this buffer is empty
            if( this->GetCount() == 0 )
            {
                return true;
            }

            if( pOut->Reserve( this->GetCount() ) )
            {
                DataTypePtr pData = pOut->m_pCurrPtr;
                pOut->m_count = this->GetCount();
                Memory::Copy( pData, pOut->GetCapacity(), this->m_pCurrPtr, this->CalcSize() );
                return true;
            }
            return false;
        }

        CONSTANT_ARRAY_TEMPLATE
        void TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Move(TCConstantArray* pOut)
        {
            VKE_ASSERT2( this->m_pCurrPtr, "Data pinter should points to m_aData" );
            assert( pOut );
            if( this == pOut || pOut->GetCount() == 0 )
            {
                return;
            }

            const size_t srcSize = pOut->GetCount() * sizeof( DataType );
            const size_t dstSize = sizeof( m_aData );
            if( pOut->m_pData )
            {
                this->m_pData = pOut->m_pData;
                this->m_pCurrPtr = this->m_pData;
            }
            else
            {
                VKE_ASSERT2( dstSize >= srcSize, "Src buffer too small. Not all data is copied." );
                // Copy
                Memory::Copy( m_aData, sizeof( m_aData ), pOut->m_aData, dstSize );
                this->m_pCurrPtr = m_aData;
            }
            this->m_capacity = pOut->GetCapacity();
            this->m_count = pOut->GetCount();
            VKE_ASSERT2( this->m_pCurrPtr, "Data pinter should points to m_aData" );
            pOut->Destroy();
        }

        CONSTANT_ARRAY_TEMPLATE
        bool TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Reserve(CountType elemCount)
        {
            return false;
        }

        CONSTANT_ARRAY_TEMPLATE
        bool TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Resize(CountType newElemCount)
        {
            assert(this->m_pCurrPtr);
            assert( DEFAULT_ELEMENT_COUNT >= newElemCount );
            bool res = false;
            if( DEFAULT_ELEMENT_COUNT >= newElemCount )
            {
                this->m_count = newElemCount;
                res = true;
            }
            return res;
        }

        CONSTANT_ARRAY_TEMPLATE
        bool TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Resize(CountType newElemCount, const DataType& Default)
        {
            if( Resize( newElemCount ) )
            {
                for( uint32_t i = this->m_count; i-- > 0; )
                {
                    this->m_pCurrPtr[i] = Default;
                }
                return true;
            }
            return false;
        }

        CONSTANT_ARRAY_TEMPLATE
        bool TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Resize()
        {
            return Resize(DEFAULT_ELEMENT_COUNT);
        }

        CONSTANT_ARRAY_TEMPLATE
        uint32_t TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::PushBack(const DataType& El)
        {
            uint32_t res = INVALID_POSITION;
            if( this->m_count < DEFAULT_ELEMENT_COUNT )
            {
                //this->m_pCurrPtr[m_count++] = El;
                auto& Element = this->m_pCurrPtr[ this->m_count++ ];
                Element = El;
                res = this->m_count - 1;
            }
            return res;
        }

        CONSTANT_ARRAY_TEMPLATE
        bool TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::PopBack(DataTypePtr pOut)
        {
            assert(pOut);
            if( !this->IsEmpty() )
            {
                *pOut = this->Back();
                this->m_count--;
                return true;
            }
            return false;
        }

        CONSTANT_ARRAY_TEMPLATE
        template<bool DestructObject>
        bool TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::PopBack()
        {
            if( !this->IsEmpty() )
            {
                this->m_count--;
                return true;
            }
            return false;
        }

        CONSTANT_ARRAY_TEMPLATE
        bool TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Append(
            CountType begin, CountType end, const TCConstantArray& Other)
        {
            return Append(begin, end, Other.m_pCurrPtr);
        }

        CONSTANT_ARRAY_TEMPLATE
            bool TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Append(
            CountType count, const DataTypePtr pData)
        {
            const auto currCount = this->GetCount();
            return this->Append( currCount, currCount + count, pData );
        }

        CONSTANT_ARRAY_TEMPLATE
        bool TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Append(CountType begin, CountType end,
                                                                        const DataTypePtr pData)
        {
            assert(begin <= end);
            const auto count = end - begin;
            if( count )
            {
                if( this->GetCount() + count >= this->GetMaxCount() )
                {
                    const auto lastCount = this->m_count;
                    const auto newCount = Policy::PushBack::Calc(GetMaxCount() + count);
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

        CONSTANT_ARRAY_TEMPLATE
        void TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::Remove(CountType elementIdx)
        {
            const auto dstSize = this->m_capacity - sizeof( DataType );
            const auto sizeToCopy = ( this->m_maxElementCount - 1 ) * sizeof( DataType );
            Memory::Copy(this->m_pCurrPtr + elementIdx, dstSize, this->m_pCurrPtr + elementIdx + 1, sizeToCopy);
            this->m_count--;
        }

        CONSTANT_ARRAY_TEMPLATE
        void TCConstantArray<TC_CONSTANT_ARRAY_TEMPLATE_PARAMS>::RemoveFast(CountType elementIdx)
        {
            this->m_pCurrPtr[ elementIdx ] = this->back();
            this->m_count--;
        }

    } // Utils
} // VKE