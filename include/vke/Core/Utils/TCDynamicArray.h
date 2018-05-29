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
            using DataTypePtr = Base::DataTypePtr;
            using DataTypeRef = Base::DataTypeRef;
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
                this->m_capacity = sizeof(m_aData);
                this->m_pCurrPtr = m_aData;
            }

            explicit TCDynamicArray(uint32_t count) :
                TCDynamicArray()
            {
                auto res = Resize( count );
                assert( res );
            }

            TCDynamicArray(uint32_t count, const DataTypeRef DefaultValue) :
                TCDynamicArray()
            {
                auto res = Resize( count, DefaultValue );
                assert( res );
            }

            TCDynamicArray(uint32_t count, VisitCallback&& Callback) :
                TCDynamicArray(),
                TCArrayContainer(count, Callback),
            {
                auto res = Resize( count, Callback );
                assert( res );
            }

            TCDynamicArray(const TCDynamicArray& Other);
            TCDynamicArray(TCDynamicArray&& Other);

            TCDynamicArray(std::initializer_list<DataType> List);

            virtual ~TCDynamicArray()
            {
                Destroy();
            }
            

            /*SizeType GetCapacity() const { return m_capacity; }
            CountType GetCount() const { return m_count; }
            SizeType CalcSize() const { return m_count * sizeof(DataType); }*/
            SizeType GetMaxCount() const { return m_maxElementCount; }
            

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
            void _Clear();
            void Clear() { _Clear<false>(); }
            void ClearFull() { _Clear<true>(); }
            void Destroy();

            bool Copy(TCDynamicArray* pOut) const;
            void Move(TCDynamicArray* pOut);
            bool Append(const TCDynamicArray& Other) { return Append(0, Other.GetCount(), Other); }
            bool Append(CountType begin, CountType end, const TCDynamicArray& Other);
            bool Append(CountType begin, CountType end, const DataType* pData);
            bool Append(CountType count, const DataType* pData);

            bool IsInConstArrayRange() const { return m_capacity < sizeof(m_aData); }

            TCDynamicArray& operator=(const TCDynamicArray& Other) { Other.Copy(this); return *this; }
            TCDynamicArray& operator=(TCDynamicArray&& Other) { Other.Move(this); return *this; }

            Iterator begin() { return Iterator(this->m_pCurrPtr, this->m_pCurrPtr + this->m_count); }
            Iterator end() { return Iterator(this->m_pCurrPtr + this->m_count, this->m_pCurrPtr + this->m_count); }
            ConstIterator begin() const { return ConstIterator(this->m_pCurrPtr, this->m_pCurrPtr + this->m_count); }
            ConstIterator end() const { return ConstIterator(this->m_pCurrPtr + this->m_count, this->m_pCurrPtr + this->m_count); }

        public:

            DataType        m_aData[DEFAULT_ELEMENT_COUNT];
            //DataTypePtr     m_pData = nullptr;
            //DataTypePtr     this->m_pCurrPtr = m_aData;
            //AllocatorType   m_Allocator = AllocatorType::Create();
            //CountType       m_count = 0;
            SizeType        m_maxElementCount = DEFAULT_ELEMENT_COUNT;
            //SizeType        m_capacity = sizeof(m_aData);
        };

        TC_DYNAMIC_ARRAY_TEMPLATE
        TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::TCDynamicArray(const TCDynamicArray& Other) :
            TCDynamicArray()
        {
            auto res = Other.Copy(this);
            assert(res);
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::TCDynamicArray(TCDynamicArray&& Other) :
            TCDynamicArray()
        {
            Other.Move(this);
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::TCDynamicArray(std::initializer_list<DataType> List) :
            TCDynamicArray()
        {
            const auto count = static_cast<CountType>(List.size());
            if (count < m_maxElementCount)
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
            assert(this->m_pCurrPtr);
            TCArrayContainer::Destroy();
            this->m_pCurrPtr = m_aData;
            m_capacity = sizeof(m_aData);
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        template<bool DestroyElements>
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::_Clear()
        {
            assert(this->m_pCurrPtr);
            if( DestroyElements )
            {
                this->_DestroyElements(this->m_pCurrPtr);
            }
            m_count = 0;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Copy(TCDynamicArray* pOut) const
        {
            assert(this->m_pCurrPtr);
            assert(pOut);
            if( this == pOut )
            {
                return true;
            }
            // Do not perform any copy operations if this buffer is empty
            if( GetCount() == 0 )
            {
                return true;
            }

            if( pOut->Reserve( GetCount() ) )
            {
                DataTypePtr pData = pOut->m_pCurrPtr;
                pOut->m_count = GetCount();
                Memory::Copy(pData, pOut->GetCapacity(), this->m_pCurrPtr, CalcSize());
                return true;
            }
            return false;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Move(TCDynamicArray* pOut)
        {
            assert(this->m_pCurrPtr);
            assert(pOut);
            if (this == pOut)
            {
                return;
            }

            if (IsInConstArrayRange())
            {
                // Copy
                Memory::Copy(m_aData, sizeof(m_aData), pOut->m_aData, pOut->GetCount() * sizeof(DataType));
                this->m_pCurrPtr = m_aData;
            }
            else
            {
                m_pData = pOut->m_pData;
                this->m_pCurrPtr = m_pData;
            }
            m_capacity = pOut->GetCapacity();
            m_count = pOut->GetCount();
            m_maxElementCount = pOut->GetMaxCount();

            pOut->m_pCurrPtr = pOut->m_pData = nullptr;
            pOut->Destroy();
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Reserve(CountType elemCount)
        {
            assert(this->m_pCurrPtr);
            if (TCArrayContainer::Reserve(elemCount))
            {
                m_maxElementCount = elemCount;
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
            assert(this->m_pCurrPtr);
            bool res = true;
            if( m_maxElementCount < newElemCount )
            {
                res = TCArrayContainer::Resize(newElemCount);
                if( res )
                {
                    m_maxElementCount = newElemCount;
                    this->m_pCurrPtr = this->m_pData;
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

        /*TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Resize(
            CountType newElemCount,
            VisitCallback&& Callback)
        {
            if (Resize(newElemCount))
            {
                for (uint32_t i = m_count; i-- > 0;)
                {
                    (Callback)(i, this->m_pCurrPtr[i]);
                }
                return true;
            }
            return false;
        }*/

        TC_DYNAMIC_ARRAY_TEMPLATE
        uint32_t TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::PushBack(const DataType& El)
        {
            if (m_count < m_maxElementCount)
            {
                //this->m_pCurrPtr[m_count++] = El;
                auto& Element = this->m_pCurrPtr[ m_count++ ];
                Element = El;
            }
            else
            {
                // Need Resize
                const auto lastCount = m_count;
                const auto count = Policy::PushBack::Calc(m_maxElementCount);
                if (TCArrayContainer::Resize(count))
                {
                    m_maxElementCount = m_count;
                    m_count = lastCount;
                    return PushBack(El);
                }
                return INVALID_POSITION;
            }
            return m_count - 1;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::PopBack(DataTypePtr pOut)
        {
            assert(pOut);
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
            assert(begin <= end);
            const auto count = end - begin;
            if( count )
            {
                if( GetCount() + count >= GetMaxCount() )
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

        TC_DYNAMIC_ARRAY_TEMPLATE
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Remove(CountType elementIdx)
        {
            const auto dstSize = m_capacity - sizeof(DataType);
            const auto sizeToCopy = (m_maxElementCount - 1) * sizeof(DataType);
            Memory::Copy(this->m_pCurrPtr + elementIdx, dstSize, this->m_pCurrPtr + elementIdx + 1, sizeToCopy);
            this->m_count--;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        void TCDynamicArray<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::RemoveFast(CountType elementIdx)
        {
            this->m_pCurrPtr[elementIdx] = Back();
            m_count--;
        }

        template
        <
            typename T,
            typename HandleType = T,
            uint32_t DEFAULT_ELEMENT_COUNT = 32,
            class AllocatorType = Memory::CHeapAllocator,
            class Policy = DynamicArrayDefaultPolicy
        >
        struct TSFreePool
        {
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
        };



    } // Utils
} // VKE