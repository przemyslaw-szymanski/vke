#pragma once

#include "TCDynamicContainerBase.h"
#include "TCDynamicArray.h"
#include <xhash>

namespace VKE
{
    namespace Utils
    {
#define TC_STRING_TEMPLATE \
        template \
        < \
            typename DataType, \
            class AllocatorType, \
            class Policy, \
            class Utils \
        >
#define TC_STRING_TEMPLATE_PARAMS \
        DataType, AllocatorType, Policy, Utils

        struct StringDefaultPolicy : public DynamicArrayDefaultPolicy
        {
            // On Resize
            struct Resize
            {
                static uint32_t Calc(uint32_t current) { return current; }
            };

            // On Reserve
            struct Reserve
            {
                static uint32_t Calc(uint32_t current) { return current + 1; /*+1 for null terminated char*/ }
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

        struct StringDefaultUtils : public DynamicArrayDefaultUtils
        {
            struct Length
            {
                static uint32_t Calc(cstr_t pString) { return static_cast< uint32_t >( strlen( pString ) ); }
            };
        };

        template
        <
            typename T,
            uint32_t DEFAULT_ELEMENT_COUNT = Config::Utils::String::DEFAULT_ELEMENT_COUNT,
            class AllocatorType = Memory::CHeapAllocator,
            class Policy = StringDefaultPolicy,
            class Utils = StringDefaultUtils
        >
        class TCString : public TCDynamicArray< T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy, Utils >
        {
            using Base = TCDynamicArray< T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy, Utils >;

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

                TCString() : Base() {}
                TCString(const TCString& Other) : Base( Other ) {}
                TCString(TCString&& Other) : Base( Other ) {}
                TC_DYNAMIC_ARRAY_TEMPLATE2
                TCString(const TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2>& Other) : Base( Other ) {}
                TC_DYNAMIC_ARRAY_TEMPLATE2
                TCString(TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2>&& Other) : Base( Other ) {}
                TCString(const DataType* pString, const CountType length);
                explicit TCString(const DataType* pString) : TCString( pString, _CalcLength( pString ) ) {}
                
                ~TCString()
                {
                    if( this->m_pData )
                    {
                        this->m_count = 0;
                    }
                }

                void operator+=(const TCString& Other) { this->Append( Other ); }
                void operator+=(const DataType* pData) { this->Append( _CalcLength( pData ) + 1, pData ); }
                TC_DYNAMIC_ARRAY_TEMPLATE
                void operator+=(const TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>& Other) { this->Append( Other ); }

                TCString& operator=(const TCString& Other) { this->Insert( 0, Other ); return *this; }
                TCString& operator=(TCString&& Other)
                {
                    this->Move( &Other );
                    return *this;
                }
                TCString& operator=(const DataType* pData) { this->Insert( 0, 0, _CalcLength( pData ) + 1, pData ); return *this; }
                TC_DYNAMIC_ARRAY_TEMPLATE
                TCString& operator=(const TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>& Other) { this->Insert( 0, Other ); return *this; }

                //bool Compare(const TCString& Other) const { return Compare( Other->GetData() ); }
                bool Compare(const DataType* pData) const;
                //TC_DYNAMIC_ARRAY_TEMPLATE
                //bool Compare(const TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>& Other) const { return Compare( Other->GetData() ); }

                //bool operator==(const TCString& Other) const { return Compare( Other ); }
                bool operator==(const DataType* pData) const { return Compare( pData ); }

                uint32_t GetLength() const { return m_count; }

                //bool Copy(TCString* pOut) const;

            protected:

                uint32_t _CalcLength(const DataType* pData) const;
        };

        using CString = TCString< char >;

        TC_DYNAMIC_ARRAY_TEMPLATE
        TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::TCString(const DataType* pString, const CountType length)
        {
            this->Insert( 0, 0, length + 1, pString );
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        uint32_t TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::_CalcLength(const DataType* pData) const
        {
            CountType c = 0;
            for (const DataType* pCurr = pData; (*pCurr++); ++c);
            return c;
        }

        TC_DYNAMIC_ARRAY_TEMPLATE
        bool TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::Compare(const DataType* pData) const
        {
            auto ret = strcmp( this->m_pCurrPtr, pData );
            return ret == 0;
        }

    } // Utils
} // VKE

namespace std
{
    template<typename DataType, uint32_t DEFAULT_ELEMENT_COUNT, class AllocatorType, class Policy, class Utils>
    struct hash< VKE::Utils::TCString< TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS > >
    {
        size_t operator()(const VKE::Utils::TCString< TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS >& Str) const
        {
            // Compute individual hash values for two data members and combine them using XOR and bit shifting
            return std::hash< VKE::cstr_t >{}( Str.GetData() );
        }
    };
}