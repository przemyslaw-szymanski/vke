#pragma once

#include "TCContainerBase.h"

namespace VKE
{
    namespace Utils
    {
#define TC_STRING_TEMPLATE \
        template \
        < \
            typename DataType, \
            uint32_t DEFAULT_ELEMENT_COUNT, \
            class AllocatorType, \
            class Policy, \
            class Utils \
        >
#define TC_STRING_TEMPLATE_PARAMS \
        DataType, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy, Utils

        struct StringDefaultPolicy
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
            struct Length
            {
                static uint32_t Calc(cstr_t pString) { return strlen( pString ); }
            };
        };

        template
        <
            typename T = char,
            uint32_t DEFAULT_ELEMENT_COUNT = 32,
            class AllocatorType = Memory::CHeapAllocator,
            class Policy = DynamicArrayDefaultPolicy,
            class Utils = DynamicArrayDefaultUtils
        >
        class TCString : public TCArrayContainer< T, AllocatorType, Policy, Utils >
        {
            using Base = TCArrayContainer< T, AllocatorType, Policy, Utils >;

            public:

                static_assert(DEFAULT_ELEMENT_COUNT > 0, "DEFAULT_ELEMENT_COUNT must be greater than 0.");
                using DataType = T;
                using DataTypePtr = Base::DataTypePtr;
                using DataTypeRef = Base::DataTypeRef;
                using SizeType = uint32_t;
                using CountType = uint32_t;
                using AllocatorPtr = Memory::IAllocator*;

                using Iterator = TCArrayIterator< DataType >;
                using ConstIterator = TCArrayIterator< const DataType >;

            public:

                TCString() {}
                TCString(const TCString& Other);
                TCString(TCString&& Other);
                TCString(DataType* pString);
                TCString(DataType* pString, const CountType length);
                ~TCString() { Destroy(); }

                bool Resize(CountType elemCount);
                bool Resize(CountType elemCount, DataType value);
                bool Reserve(CountType elemCount);
                void Remove(CountType elementIdx);

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

                bool Copy(TCString* pOut) const;
                void Move(TCString* pOut);
                bool Append(const TCString& Other) { return Append(0, Other.GetCount(), Other); }
                bool Append(CountType begin, CountType end, const TCString& Other);
                bool Append(CountType begin, CountType end, const DataType* pData);
                bool Append(CountType count, const DataType* pData);

                TCString& operator=(const TCString& Other) { Other.Copy(this); return *this; }
                TCString& operator=(TCString&& Other) { Other.Move(this); return *this; }
                void operator+=(const TCString& Other) { Append( Other ); }
                void operator+=(const DataType* pString) { Append( Utils::Length::Calc( pString ), pString ); }

                Iterator begin() { return Iterator(this->m_pCurrPtr, this->m_pCurrPtr + this->m_count); }
                Iterator end() { return Iterator(this->m_pCurrPtr + this->m_count, this->m_pCurrPtr + this->m_count); }
                ConstIterator begin() const { return ConstIterator(this->m_pCurrPtr, this->m_pCurrPtr + this->m_count); }
                ConstIterator end() const { return ConstIterator(this->m_pCurrPtr + this->m_count, this->m_pCurrPtr + this->m_count); }

        };

        TC_STRING_TEMPLATE
            TCString<TC_STRING_TEMPLATE_PARAMS>::TCString(const TCString& Other) :
            TCString()
        {
            auto res = Other.Copy(this);
            assert(res);
        }

        TC_STRING_TEMPLATE
            TCString<TC_STRING_TEMPLATE_PARAMS>::TCString(TCString&& Other) :
            TCString()
        {
            Other.Move(this);
        }

        TC_STRING_TEMPLATE
        bool TCString<TC_STRING_TEMPLATE_PARAMS>::Copy(TCString* pOut) const
        {
            assert(pOut);
            if( this == pOut || GetCount() == 0 )
            {
                return true;
            }

            if( pOut->Reserve( GetCount() ) )
            {
                DataTypePtr pData = pOut->m_pCurrPtr;
                pOut->m_count = GetCount();
                Memory::Copy( pData, pOut->GetCapacity(), m_pCurrPtr, CalcSize() );
                return true;
            }
            return false;
        }

        TC_STRING_TEMPLATE
            void TCString<TC_STRING_TEMPLATE_PARAMS>::Move(TCString* pOut)
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

        TC_STRING_TEMPLATE
            bool TCString<TC_STRING_TEMPLATE_PARAMS>::Reserve(CountType elemCount)
        {
            assert(this->m_pCurrPtr);
            if (TCArrayContainer::Reserve(elemCount))
            {
                m_maxElementCount = elemCount;
                if (this->m_pData)
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
    } // Utils
} // VKE