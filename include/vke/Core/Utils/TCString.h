#pragma once

#include "TCDynamicContainerBase.h"

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

        struct StringDefaultUtils : public ArrayContainerDefaultUtils
        {
            struct Length
            {
                static uint32_t Calc(cstr_t pString) { return strlen( pString ); }
            };
        };

        template
        <
            typename T = char,
            class AllocatorType = Memory::CHeapAllocator,
            class Policy = StringDefaultPolicy,
            class Utils = StringDefaultUtils
        >
        class TCString : public TCDynamicContainer< T, AllocatorType, Policy, Utils >
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

                TCString() {}
                TCString(const TCString& Other) : Base( Other ) {}
                TCString(TCString&& Other) : Base( Other ) {}
                TCString(DataType* pString);
                TCString(DataType* pString, const CountType length);
                
                ~TCString() { }

                void operator+=(const TCString& Other) { this->Append( Other ); }
                void operator+=(const DataType* pData) { this->Append( _CalcLength( pData ) + 1, pData ); }

                TCString& operator=(const DataType* pData) { this->Insert( 0, 0, _CalcLength( pData ) + 1, pData ); return *this; }

                uint32_t GetLength() const { return m_count; }

                //bool Copy(TCString* pOut) const;

            protected:

                uint32_t _CalcLength(const DataType* pData) const;
        };

        TC_STRING_TEMPLATE
        uint32_t TCString<TC_STRING_TEMPLATE_PARAMS>::_CalcLength(const DataType* pData) const
        {
            CountType c = 0;
            for (const DataType* pCurr = pData; (*pCurr++); ++c);
            return c;
        }

        /*TC_STRING_TEMPLATE
        uint32_t TCString<TC_STRING_TEMPLATE_PARAMS>::Copy(TCString* pOut) const
        {
            Base::Copy( pOut );

        }*/

    } // Utils
} // VKE