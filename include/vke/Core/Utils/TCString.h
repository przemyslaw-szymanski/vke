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
        class TCString : protected TCDynamicArray< T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy, Utils >
        {
            using Base = TCDynamicArray< T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy, Utils >;

            public:

                using DataType = T;
                using DataTypePtr = DataType*;
                using DataTypeRef = DataType&;
                using SizeType = uint32_t;
                using CountType = uint32_t;
                using AllocatorPtr = Memory::IAllocator*;

                using Iterator = TCArrayIterator< DataType >;
                using ConstIterator = TCArrayIterator< const DataType >;

            public:

                TCString() : Base() { m_aData[0] = 0; }
                TCString(const TCString& Other) : Base( Other ) {}
                TCString(TCString&& Other) : Base( Other ) {}
                TC_DYNAMIC_ARRAY_TEMPLATE2
                TCString(const TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2>& Other) { ConvertToOther(Other); }
                TC_DYNAMIC_ARRAY_TEMPLATE2
                TCString(TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2>&& Other) : Base( Other ) {}
                TCString(const CountType length, const DataTypePtr pString);
                TCString(const DataTypePtr pString) : TCString( _CalcLength( pString ), pString ) {}
                TCString(const int32_t v) { Convert(v, GetData(), GetMaxCount()); }
                TCString(const uint32_t v) { Convert(v, GetData(), GetMaxCount()); }
                TCString(const int64_t v) { Convert(v, GetData(), GetMaxCount()); }
                TCString(const uint64_t v) { Convert(v, GetData(), GetMaxCount()); }
                TCString(const float v) { Convert(v, GetData(), GetMaxCount()); }
                TCString(const double v) { Convert(v, GetData(), GetMaxCount()); }

                ~TCString()
                {
                    if( this->m_pData )
                    {
                        this->m_count = 0;
                    }
                }

                vke_force_inline DataTypePtr GetData() const { return Base::GetData(); }
                vke_force_inline uint32_t GetCount() const { return Base::GetCount(); }

                void Append(const uint32_t begin, const uint32_t end, const DataType* pData)
                {
                    // Remove null from last position
                    if (this->m_count > 0)
                    {
                        this->m_count -= 1;
                    }
                    Base::Append(begin, end, pData);
                    const auto count = end - begin;
                    if (count <= 0)
                    {
                        this->m_count += 1; // if nothing copied restore null
                    }
                }

                void Append(const TCString& Other) { Append(0, Other.GetCount(), Other.GetData()); }

                bool IsEmpty() const { return Base::IsEmpty(); }

                void operator+=(const TCString& Other) { this->Append(Other); }
                void operator+=(const DataType* pData) { this->Append( 0, _CalcLength( pData ) + 1, pData ); }
                TC_DYNAMIC_ARRAY_TEMPLATE
                void operator+=(const TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>& Other) { this->Append( Other ); }

                TCString& operator=(const TCString& Other) { this->Copy(Other); return *this; }
                TCString& operator=(TCString&& Other)
                {
                    this->Move( &Other );
                    return *this;
                }
                TCString& operator=(const DataType* pData) { this->Copy(pData, _CalcLength(pData)+1); return *this; }
                TC_DYNAMIC_ARRAY_TEMPLATE
                TCString& operator=(const TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>& Other) { this->Insert( 0, Other ); return *this; }

                //bool Compare(const TCString& Other) const { return Compare( Other->GetData() ); }
                bool Compare(const DataType* pData) const;
                //TC_DYNAMIC_ARRAY_TEMPLATE
                //bool Compare(const TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>& Other) const { return Compare( Other->GetData() ); }

                //bool operator==(const TCString& Other) const { return Compare( Other ); }
                bool operator==(const DataType* pData) const { return Compare( pData ); }

                uint32_t GetLength() const { return GetCount() == 0 ? 0 : GetCount() - 1; }

                template<typename... Args>
                size_t Format(cstr_t format, Args&&... args)
                {
                    size_t ret = vke_sprintf(GetData(), Base::GetCapacity(), format, args...);
                    if (ret > 0)
                    {
                        this->m_count += (uint32_t)ret;
                    }
                    return ret;
                }

                template<typename... Args>
                size_t Format( cwstr_t format, Args&&... args )
                {
                    size_t ret = vke_wsprintf( GetData(), Base::GetCapacity(), format, args... );
                    if( ret > 0 )
                    {
                        this->m_count += ( uint32_t )ret;
                    }
                    return ret;
                }

                size_t GettUtf16(wchar_t* pDst, const uint32_t dstLength) const
                {
                    size_t ret;
#if VKE_WINDOWS
                    mbstowcs_s(&ret, pDst, dstLength, GetData(), GetCount());
#else
                    ret = mbstowcs(pDst, GetData(), GetCount());
#endif
                    return ret;
                }

                size_t GetUtf8(char* pDst, const uint32_t dstSize) const
                {
                    size_t ret;
#if VKE_WINDOWS
                    wcstombs_s(&ret, pDst, dstSize, GetData(), GetCount());
#else
                    ret = wcstombs(pDst, GetData(), GetCount());
#endif
                    return ret;
                }

                static vke_force_inline size_t Convert(cstr_t pSrc, const uint32_t srcSize, wchar_t* pDst,
                    const uint32_t dstSize)
                {
                    size_t ret;
                    vke_mbstowcs(ret, pDst, dstSize, pSrc, srcSize);
                    return ret;
                }

                static vke_force_inline size_t Convert(cwstr_t pSrc, const uint32_t srcSize, wchar_t* pDst,
                    const uint32_t dstSize)
                {
                    Memory::Copy(pDst, dstSize, pSrc, srcSize);
                    return Math::Min(srcSize, dstSize);
                }

                static vke_force_inline size_t Convert(cstr_t pSrc, const uint32_t srcSize, char* pDst,
                    const uint32_t dstSize)
                {
                    Memory::Copy(pDst, dstSize, pSrc, srcSize);
                    return Math::Min(srcSize, dstSize);
                }

                static vke_force_inline size_t Convert(cwstr_t pSrc, const uint32_t srcSize, char* pDst,
                    const uint32_t dstSize)
                {
                    size_t ret;
                    vke_wcstombs(ret, pDst, dstSize, pSrc, srcSize);
                    return ret;
                }

                static vke_force_inline void Convert(const int32_t value, char* pSrc, const uint32_t srcSize)
                {
                    vke_sprintf(pSrc, srcSize, "%d", value);
                }

                static vke_force_inline void Convert(const uint32_t value, char* pSrc, const uint32_t srcSize)
                {
                    vke_sprintf(pSrc, srcSize, "%d", value);
                }

                static vke_force_inline void Convert(const int64_t value, char* pSrc, const uint32_t srcSize)
                {
                    vke_sprintf(pSrc, srcSize, "%ll", value);
                }

                static vke_force_inline void Convert(const uint64_t value, char* pSrc, const uint32_t srcSize)
                {
                    vke_sprintf(pSrc, srcSize, "%llu", value);
                }

                static vke_force_inline void Convert(const float value, char* pSrc, const uint32_t srcSize)
                {
                    vke_sprintf(pSrc, srcSize, "%f", value);
                }

                static vke_force_inline void Convert(const double value, char* pSrc, const uint32_t srcSize)
                {
                    vke_sprintf(pSrc, srcSize, "%f", value);
                }

                static vke_force_inline void Convert(const int32_t value, wchar_t* pSrc, const uint32_t srcSize)
                {
                    vke_wsprintf(pSrc, srcSize, L"%d", value);
                }

                static vke_force_inline void Convert(const uint32_t value, wchar_t* pSrc, const uint32_t srcSize)
                {
                    vke_wsprintf(pSrc, srcSize, L"%d", value);
                }

                static vke_force_inline void Convert(const int64_t value, wchar_t* pSrc, const uint32_t srcSize)
                {
                    vke_wsprintf(pSrc, srcSize, L"%ll", value);
                }

                static vke_force_inline void Convert(const uint64_t value, wchar_t* pSrc, const uint32_t srcSize)
                {
                    vke_wsprintf(pSrc, srcSize, L"%llu", value);
                }

                static vke_force_inline void Convert(const float value, wchar_t* pSrc, const uint32_t srcSize)
                {
                    vke_wsprintf(pSrc, srcSize, L"%f", value);
                }

                static vke_force_inline void Convert(const double value, wchar_t* pSrc, const uint32_t srcSize)
                {
                    vke_wsprintf(pSrc, srcSize, L"%f", value);
                }

                operator const DataType*() const { return GetData(); }

                TC_DYNAMIC_ARRAY_TEMPLATE2
                size_t ConvertToOther(const TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2>& Other)
                {
                    if (std::is_same< DataType2, wchar_t >::value &&
                        std::is_same< DataType, char >::value)
                    {
                        Resize(Other.GetCount() * sizeof(wchar_t));
                    }
                    else
                    {
                        Resize(Other.GetCount());
                    }
                    auto pDst = GetData();
                    return Convert(Other.GetData(), Other.GetCount(), pDst, GetCount());
                }

                size_t Convert(cstr_t pStr)
                {
                    const uint32_t count = (uint32_t)strlen(pStr) + 1;
                    Resize(count);
                    auto pDst = GetData();
                    return Convert(pStr, count, pDst, count);
                }

                TC_DYNAMIC_ARRAY_TEMPLATE2
                operator TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS2>() const
                {
                    Other Ret;
                    Ret.ConvertToOther(*this);
                    return Ret;
                }

                hash_t CalcHash() const
                {
                    hash_t ret = 5381;
                    DataTypePtr pCurr = GetData();
                    DataType c;
                    while( c = *pCurr++ )
                    {
                        ret = ((ret << 5) + ret) ^ c;
                    }
                    return ret;
                }

                uint32_t Copy(const TCString& Other)
                {
                    return Copy(Other.GetData(), Other.GetCount());
                }

                uint32_t Copy(const DataType* pData, const CountType& count)
                {
                    //auto c = Math::Min(this->m_resizeElementCount, count);
                    auto c = count;
                    if (this->Reserve(c))
                    {
                        this->_SetCurrPtr();
                        auto pCurrDst = this->m_pCurrPtr;
                        auto pCurrSrc = pData;
                        while (*pCurrDst++ = *pCurrSrc++) {}
                        this->m_pCurrPtr[c] = 0;
                        this->m_count = c;
                    }
                    else
                    {
                        c = INVALID_POSITION;
                    }
                    return c;
                }

                bool Resize( CountType newElemCount )
                {
                    return Base::Resize( newElemCount );
                }

            protected:

                uint32_t _CalcLength(const DataType* pData) const;
        };

        using CString = TCString< char >;

        TC_DYNAMIC_ARRAY_TEMPLATE
        TCString<TC_DYNAMIC_ARRAY_TEMPLATE_PARAMS>::TCString(const CountType length, const DataTypePtr pString)
        {
            Base::Copy( length + 1, pString );
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

    using ResourceName = Utils::TCString< char, Config::Resource::MAX_NAME_LENGTH >;
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