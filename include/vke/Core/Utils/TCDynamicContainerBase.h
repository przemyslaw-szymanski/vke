#pragma once

#include "TCContainerBase.h"

namespace VKE
{
    namespace Utils
    {   

#define TC_DYNAMIC_CONTAINER_TEMPLATE \
        template \
        < \
            typename DataType, \
            class AllocatorType, \
            class Policy, \
            class Utils \
        >

#define TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS \
        DataType, AllocatorType, Policy, Utils

        struct DynamicContainerDefaultPolicy : public ArrayContainerDefaultPolicy
        {
            
        };

        struct DynamicContainerDefaultUtils : public ArrayContainerDefaultUtils
        {

        };

        template
        <
            typename T,
            class AllocatorType = Memory::CHeapAllocator,
            class Policy = DynamicContainerDefaultPolicy,
            class Utils = DynamicContainerDefaultUtils
        >
        class TCDynamicContainer : public TCArrayContainer< T, AllocatorType, Policy, Utils >
        {
            using Base = TCArrayContainer< T, AllocatorType, Policy, Utils >;
            public:

                using DataType = T;
                using DataTypePtr = Base::DataTypePtr;
                using DataTypeRef = Base::DataTypeRef;
                using SizeType = uint32_t;
                using CountType = uint32_t;
                using AllocatorPtr = Memory::IAllocator*;       

                using Iterator = TCArrayIterator< DataType >;
                using ConstIterator = TCArrayIterator< const DataType >;

            public:

                TCDynamicContainer()
                {
                }

                explicit TCDynamicContainer(uint32_t count) :
                    TCDynamicContainer()
                {
                    auto res = this->Resize( count );
                    assert( res );
                }

                TCDynamicContainer(uint32_t count, const DataTypeRef DefaultValue) :
                    TCDynamicContainer()
                {
                    auto res = Resize( count, DefaultValue );
                    assert( res );
                }

                /*TCDynamicContainer(uint32_t count, VisitCallback&& Callback) :
                    TCDynamicContainer(),
                    TCArrayContainer(count, Callback),
                {
                    auto res = this->Resize( count, Callback );
                    assert( res );
                }*/

                TCDynamicContainer(const TCDynamicContainer& Other);
                TCDynamicContainer(TCDynamicContainer&& Other);


                virtual ~TCDynamicContainer()
                {
                }            

                uint32_t PushBack(const DataType& el);
                bool PopBack(DataTypePtr pOut);
                template<bool DestructObject = true>
                bool PopBack();

                //bool Resize(CountType newElemCount);
                //bool Resize(CountType newElemCount, const DataType& DefaultData);
                //template<typename VisitCallback>
                //bool Resize(CountType newElemCount, VisitCallback&& Callback);
                //bool Reserve(CountType elemCount);
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
                void _Clear();
                void Clear() { _Clear<false>(); }
                void ClearFull() { _Clear<true>(); }

                //bool Copy(TCDynamicContainer* pOut) const;
                bool Insert(CountType pos, CountType begin, CountType end, const DataType* pData);
                bool Replace(CountType pos, CountType begin, CountType end, const DataType* pData);
                bool Append(const TCDynamicContainer& Other) { return Append(0, Other.GetCount(), Other); }
                bool Append(CountType begin, CountType end, const TCDynamicContainer& Other);
                bool Append(CountType begin, CountType end, const DataType* pData);
                bool Append(CountType count, const DataType* pData);

                TCDynamicContainer& operator=(const TCDynamicContainer& Other) { Other.Copy(this); return *this; }
                TCDynamicContainer& operator=(TCDynamicContainer&& Other) { Other.Move(this); return *this; }

                Iterator begin() { return Iterator(this->m_pCurrPtr, this->m_pCurrPtr + this->m_count); }
                Iterator end() { return Iterator(this->m_pCurrPtr + this->m_count, this->m_pCurrPtr + this->m_count); }
                ConstIterator begin() const { return ConstIterator(this->m_pCurrPtr, this->m_pCurrPtr + this->m_count); }
                ConstIterator end() const { return ConstIterator(this->m_pCurrPtr + this->m_count, this->m_pCurrPtr + this->m_count); }

        };

        TC_DYNAMIC_CONTAINER_TEMPLATE
        TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::TCDynamicContainer(const TCDynamicContainer& Other) :
            TCDynamicContainer()
        {
            auto res = Other.Copy(this);
            assert(res);
        }

        TC_DYNAMIC_CONTAINER_TEMPLATE
        TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::TCDynamicContainer(TCDynamicContainer&& Other) :
            TCDynamicContainer()
        {
            Other.Move(this);
        }

        TC_DYNAMIC_CONTAINER_TEMPLATE
        template<bool DestroyElements>
        void TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::_Clear()
        {
            assert(this->m_pCurrPtr);
            if( DestroyElements )
            {
                this->_DestroyElements(this->m_pCurrPtr);
            }
            this->m_count = 0;
        }

        TC_DYNAMIC_CONTAINER_TEMPLATE
        uint32_t TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::PushBack(const DataType& El)
        {
            const auto currCapacity = this->m_count * sizeof(DataType);
            if( currCapacity < this->m_capacity )
            {
                //this->m_pCurrPtr[m_count++] = El;
                auto& Element = this->m_pCurrPtr[ this->m_count++ ];
                Element = El;
            }
            else
            {
                // Need Resize
                const auto lastCount = this->m_count;
                const auto count = Policy::PushBack::Calc( this->m_count );
                if( TCArrayContainer::Resize( count ) )
                {
                    this->m_count = lastCount;
                    return PushBack( El );
                }
                return INVALID_POSITION;
            }
            return this->m_count - 1;
        }

        TC_DYNAMIC_CONTAINER_TEMPLATE
        bool TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::PopBack(DataTypePtr pOut)
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

        TC_DYNAMIC_CONTAINER_TEMPLATE
        template<bool DestructObject>
        bool TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::PopBack()
        {
            if( !this->IsEmpty() )
            {
                this->m_count--;
                return true;
            }
            return false;
        }

        TC_DYNAMIC_CONTAINER_TEMPLATE
        bool TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::Append(
            CountType begin, CountType end, const TCDynamicContainer& Other)
        {
            return Append(begin, end, Other.m_pCurrPtr);
        }

        TC_DYNAMIC_CONTAINER_TEMPLATE
        bool TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::Append(
            CountType count, const DataType* pData)
        {
            const auto currCount = this->GetCount();
            return Append(0, count, pData);
        }

        TC_DYNAMIC_CONTAINER_TEMPLATE
        bool TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::Append(CountType begin, CountType end,
            const DataType* pData)
        {
            assert(begin <= end);
            const auto count = end - begin;
            if( count )
            {
                auto newCount = this->GetCount() + count;
                const auto newCapacity = ( newCount ) * sizeof( DataType );
                if( newCapacity >= this->GetCapacity() )
                {
                    const auto lastCount = this->GetCount();
                    newCount = Policy::PushBack::Calc( newCount );
                    if( !Resize( newCount ) )
                    {
                        return false;
                    }
                    this->m_count = lastCount;
                }
                const auto dstSize = this->m_capacity - this->m_count * sizeof(DataType);
                const auto bytesToCopy = count * sizeof(DataType);
                DataTypePtr pCurrPtr = this->m_pCurrPtr + this->m_count;
                Memory::Copy( pCurrPtr, dstSize, pData + begin, bytesToCopy );
                this->m_count += count;
                return true;
            }
            return true;
        }

        TC_DYNAMIC_CONTAINER_TEMPLATE
        void TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::Remove(CountType elementIdx)
        {
            const auto dstSize = this->m_capacity - sizeof(DataType);
            const auto sizeToCopy = (this->m_count - 1) * sizeof(DataType);
            Memory::Copy(this->m_pCurrPtr + elementIdx, dstSize, this->m_pCurrPtr + elementIdx + 1, sizeToCopy);
            this->m_count--;
        }

        TC_DYNAMIC_CONTAINER_TEMPLATE
        bool TCDynamicContainer<TC_DYNAMIC_CONTAINER_TEMPLATE_PARAMS>::Insert(CountType pos, CountType begin,
                CountType end, const DataType* pData)
        {
            assert( begin <= end );
            assert( pos <= this->GetCount() );
            
            const auto countToCopy = end - begin;
            const auto sizeToCopy = countToCopy * sizeof( DataType );
            const auto sizeLeft = this->GetCapacity() - ( pos * sizeof( DataType ) );

            // If there is no space left
            if( sizeLeft < sizeToCopy )
            {
                const auto oldCount = this->GetCount();
                const auto newCount = oldCount + pos + countToCopy;
                const auto resizeCount = Policy::PushBack::Calc( newCount );
                if( !Resize( resizeCount ) )
                {
                    return false;
                }
                // Override current count
                this->m_count = oldCount;
                return Insert( pos, begin, end, pData );
            }
            // Move elements
            DataType* pSrc = this->m_pCurrPtr + pos;
            DataType* pDst = this->m_pCurrPtr + pos + countToCopy;
            const auto dstSize = sizeLeft - sizeToCopy;
            const auto srcSize = sizeToCopy;
            Memory::Copy( pDst, dstSize, pSrc, srcSize );
            this->m_count += countToCopy;
            // Copy new elements
            pDst = this->m_pCurrPtr + pos;
            Memory::Copy( pDst, dstSize, pData, sizeToCopy );
            //this->m_pCurrPtr = this->m_pData + this->m_count;
            return true;
        }

    } // Utils
} // VKE