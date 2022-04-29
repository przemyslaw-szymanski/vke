#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <array>
#include <list>
#include <deque>
#include <stack>
#include <sstream>
#include <bitset>
#include <type_traits>
#include <array>
#include <initializer_list>

#include <thread>
#include <mutex>
#include <future>
#include <atomic>

#include <cassert>

#include <regex>

#if VKE_COMPILER_VISUAL_STUDIO
#   pragma warning( push )
#   pragma warning( disable : 4201 )
#else
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wpedantic"
#endif

namespace VKE
{
    using cstr_t = const char*;
    using cwstr_t = const wchar_t*;
    using str_t = std::string;
    using wstr_t = std::wstring;
    using mem_t = uint8_t;
    using memptr_t = mem_t*;
    using hash_t = std::size_t;
    using image_dimm_t = uint16_t;
    using image_size_t = image_dimm_t;

    static const std::string EMPTY_STRING = "";
    static const std::wstring EMPTY_WSTRING = L"";
    static const uint8_t UNDEFINED = 0;
    static const uint8_t UNKNOWN = 0;
    static const uint8_t NONE = 0;
    static const uint8_t UNDEFINED_U8 = static_cast<uint8_t>(-1);
    static const uint16_t UNDEFINED_U16 = static_cast<uint16_t>(-1);
    static const uint32_t UNDEFINED_U32 = static_cast<uint32_t>(-1);
    static const uint64_t UNDEFINED_U64 = static_cast<uint64_t>(-1);

    static const uint32_t INVALID_POSITION = static_cast<const uint32_t>( ~0 );

    template<typename T>
    struct TSExtent
    {
        using Type = T;
        union
        {
            struct
            {
                Type width;
                Type height;
            };

            struct
            {
                Type begin;
                Type end;
            };

            struct
            {
                Type x;
                Type y;
            };

            struct
            {
                Type min;
                Type max;
            };
        };

        TSExtent() = default;
        TSExtent(const TSExtent& Other) = default;
        TSExtent( TSExtent&& Other ) = default;
        constexpr TSExtent( const Type& v1, const Type& v2 ) : x{ v1 }, y{ v2 } {}
        constexpr TSExtent( const Type& v ) : x{ v }, y{ v } {}
        ~TSExtent() = default;

        TSExtent& operator=( const TSExtent& ) = default;
        TSExtent& operator=( TSExtent&& ) = default;

        template<class OtherExtentType>
        TSExtent& operator=( const OtherExtentType& Other )
        {
            x = (Type)Other.x;
            y = (Type)Other.y;
            return *this;
        }

        template<class OtherExtentType>
        bool operator==( const OtherExtentType& Other ) const
        {
            return x == (Type)Other.x && y == (Type)Other.y;
        }

        template<class OtherExtentType>
        bool operator!=( const OtherExtentType& Other ) const
        {
            return x != (Type)Other.x || y != (Type)Other.y;
        }

        template<class OtherExtentType>
        void operator+=( const OtherExtentType& Other )
        {
            x += (Type)Other.x;
            y += (Type)Other.y;
        }

        template<class OtherExtentType>
        void operator-=( const OtherExtentType& Other )
        {
            x -= (Type)Other.x;
            y -= (Type)Other.y;
        }

        template<class OtherExtentType>
        TSExtent operator+( const OtherExtentType& Other ) const
        {
            return TSExtent( x + (Type)Other.x, y + (Type)Other.y );
        }

        template<class OtherExtentType>
        TSExtent operator-( const OtherExtentType& Other ) const
        {
            return TSExtent( x - (Type)Other.x, y - (Type)Other.y );
        }

        template<class OtherExtentType>
        TSExtent operator*( const OtherExtentType& Other ) const
        {
            return TSExtent( x * (Type)Other.x, y * (Type)Other.y );
        }

        template<class OtherExtentType>
        TSExtent operator/( const OtherExtentType& Other ) const
        {
            return TSExtent( x / (Type)Other.x, y / (Type)Other.y );
        }

        /*template<class OtherExtentType>
        operator OtherExtentType() const
        {
            static_assert( std::is_base_of_v<TSExtent, OtherExtentType>::value, "Destination type must be TSExtent." );
            return { (OtherExtentType::Type)x, ( OtherExtentType::Type )y };
        }*/
    };

    using handle_t = uint64_t;

    static const handle_t RANDOM_HANDLE = std::numeric_limits<handle_t>::max();
    //static const handle_t INVALID_HANDLE = 0;
    //static const handle_t NULL_HANDLE_VALUE = (0);

    struct InvalidTag
    {};

#define _VKE_DECL_CMP_OPERATOR_V(_valueType, _thisMember, _op) \
    bool operator _op (const _valueType& v) const { return _thisMember _op v; }

#define _VKE_DECL_CMP_OPERATOR(_otherType, _thisMember, _otherMember, _op) \
    bool operator _op (const _otherType& Other) const { return _thisMember _op Other._otherMember; }

#define _VKE_DECL_CMP_OPERATORS(_otherType, _thisMember, _otherMember) \
    _VKE_DECL_CMP_OPERATOR(_otherType, _thisMember, _otherMember, ==) \
    _VKE_DECL_CMP_OPERATOR(_otherType, _thisMember, _otherMember, <) \
    _VKE_DECL_CMP_OPERATOR(_otherType, _thisMember, _otherMember, <=) \
    _VKE_DECL_CMP_OPERATOR(_otherType, _thisMember, _otherMember, >) \
    _VKE_DECL_CMP_OPERATOR(_otherType, _thisMember, _otherMember, >=) \
    _VKE_DECL_CMP_OPERATOR(_otherType, _thisMember, _otherMember, &&) \
    _VKE_DECL_CMP_OPERATOR(_otherType, _thisMember, _otherMember, ||) \
    _VKE_DECL_CMP_OPERATOR(_otherType, _thisMember, _otherMember, !=)

#define _VKE_DECL_CMP_OPERATORS_V(_otherValueType, _thisMember) \
    _VKE_DECL_CMP_OPERATOR_V(_otherValueType, _thisMember, _otherMember, ==) \
    _VKE_DECL_CMP_OPERATOR_V(_otherValueType, _thisMember, _otherMember, <) \
    _VKE_DECL_CMP_OPERATOR_V(_otherValueType, _thisMember, _otherMember, <=) \
    _VKE_DECL_CMP_OPERATOR_V(_otherValueType, _thisMember, _otherMember, >) \
    _VKE_DECL_CMP_OPERATOR_V(_otherValueType, _thisMember, _otherMember, >=) \
    _VKE_DECL_CMP_OPERATOR_V(_otherValueType, _thisMember, _otherMember, &&) \
    _VKE_DECL_CMP_OPERATOR_V(_otherValueType, _thisMember, _otherMember, ||) \
    _VKE_DECL_CMP_OPERATOR_V(_otherValueType, _thisMember, _otherMember, !=)

    template<typename T, typename HandleT = handle_t, HandleT INVALID_VALUE = HandleT()>
    struct _STagHandle final
    {
        HandleT handle;

        _STagHandle() = default;
        _STagHandle( const _STagHandle& ) = default;
        _STagHandle( _STagHandle&& ) = default;
        constexpr _STagHandle( const _STagHandle< InvalidTag >& ) : handle{ INVALID_VALUE } {}
        explicit _STagHandle(const HandleT& hOther) : handle{ hOther } {}
        constexpr _STagHandle( const std::nullptr_t& ) : handle{ INVALID_VALUE } {}

        _STagHandle& operator=( const _STagHandle& ) = default;
        _STagHandle& operator=( _STagHandle&& ) = default;
        void operator=(const _STagHandle<InvalidTag>&) { handle = INVALID_VALUE; }
        void operator=( const std::nullptr_t& ) { handle = INVALID_VALUE; }
        bool operator==( const std::nullptr_t& ) const { return handle == INVALID_VALUE; }
        bool operator!=( const std::nullptr_t& ) const { return handle != INVALID_VALUE; }
        bool operator!() const { return !handle; }
        _VKE_DECL_CMP_OPERATORS(_STagHandle, handle, handle);

        friend std::ostream& operator<<(std::ostream& o, const _STagHandle& Handle) { o << Handle.handle; return o; }
    };

    template<>
    struct _STagHandle< InvalidTag > final
    {
        operator uint32_t() const { return 0; }
        friend std::ostream& operator<<(std::ostream& o, const _STagHandle< InvalidTag >& Handle)
        {
            o << 0x0; return o;
        }
    };

#define VKE_DECLARE_HANDLE(_name) \
    struct _name##Tag {}; \
    using _name##Handle = _STagHandle< _name##Tag >

#define VKE_DECLARE_HANDLE2(_name, _type) \
    struct _name##Tag {}; \
    using _name##Handle = _STagHandle< _name##Tag, _type >

    template<typename DstHandleT, typename SrcHandleT>
    static vke_force_inline DstHandleT HandleCast( const SrcHandleT& hSrc )
    {
        return DstHandleT{ hSrc.handle };
    }

    template<typename DstHandleT>
    static vke_force_inline DstHandleT HandleCast( const handle_t& hSrc )
    {
        return DstHandleT{ ( DstHandleT )hSrc };
    }

    template<typename DstHandleT, typename SrcHandleT>
    static vke_force_inline DstHandleT ReinterpretHandle( const SrcHandleT& hSrc )
    {
        return DstHandleT{ ( DstHandleT )hSrc.handle };
    }

    using NullHandle = _STagHandle< InvalidTag >;
    static const NullHandle INVALID_HANDLE;

#define VKE_VALID_HANDLE(_handle) ((_handle) != VKE::INVALID_HANDLE)

    using ExtentI8  = TSExtent< int8_t >;
    using ExtentI32 = TSExtent< int32_t >;
    using ExtentI16 = TSExtent< int16_t >;
    using ExtentI64 = TSExtent< int64_t >;

    using ExtentU8  = TSExtent< uint8_t>;
    using ExtentU32 = TSExtent< uint32_t >;
    using ExtentU16 = TSExtent< uint16_t >;
    using ExtentU64 = TSExtent< uint64_t >;

    using ExtentF32 = TSExtent< float >;
    using ExtentF64 = TSExtent< double >;

    using ImageDimmension = TSExtent<image_dimm_t>;
    using ImageSize = ImageDimmension;

    using AtomicBool    = std::atomic<bool>;
    using AtomicInt32   = std::atomic<int32_t>;
    using AtomicUInt32  = std::atomic<uint32_t>;

    template<typename PositionT, typename SizeT>
    struct TSRect2D
    {
        TSExtent<PositionT> Position;
        TSExtent<SizeT> Size;
    };

    using Rect2DI32 = TSRect2D< int32_t, uint32_t >;
    using Rect2DF32 = TSRect2D< float, float >;
    using Rect2D = Rect2DI32;

    template<typename _T1_, typename _T2_>
    static inline _T1_ Min(const _T1_& t1, const _T2_& t2)
    {
        return (t1 < static_cast< _T1_ >(t2)) ? t1 : static_cast< _T1_ >(t2);
    }

    template<typename _T1_, typename _T2_>
    static inline _T1_ Max(const _T1_& t1, const _T2_& t2)
    {
        return (t1 > static_cast< _T1_ >(t2)) ? t1 : static_cast< _T1_ >(t2);
    }

#if VKE_USE_STL_PORT

#else
#   define vke_vector       std::vector
#   define vke_map          std::map
#   define vke_hash_map     std::unordered_map
#   define vke_list         std::list
#   define vke_queue        std::deque
#   define vke_sort         std::sort
#   define vke_string       std::string
#   define vke_pair         std::pair
#   define vke_mutex        std::mutex
#   define vke_bool_vector  std::vector<bool>
#endif // VKE_USE_STL_PORT

#if VKE_WINDOWS
#   define vke_sprintf(_pBuff, _buffSize, _pFormat, ...) sprintf_s((_pBuff), (_buffSize), (_pFormat), __VA_ARGS__)
#   define vke_wsprintf(_pBuff, _buffSize, _pFormat, ...) swprintf_s((_pBuff), (_buffSize), (_pFormat), __VA_ARGS__)
#   define vke_strcpy(_pDst, _dstSize, _pSrc) strcpy_s( (_pDst), (_dstSize), (_pSrc) )
#   define vke_wstrcpy(_pDst, _dstSize, _pSrc) wcscpy_s( (_pDst), (_dstSize), (_pSrc) )
#   define vke_mbstowcs(_errOut, _pDst, _pDstSizeInWors, _pSrc, _pSrcSizeInBytes) mbstowcs_s( &(_errOut), (_pDst), (_pDstSizeInWors), (_pSrc), (_pSrcSizeInBytes) )
#   define vke_wcstombs(_errOut, _pDst, _pDstSizeInWors, _pSrc, _pSrcSizeInBytes) wcstombs_s( &(_errOut), (_pDst), (_pDstSizeInWors), (_pSrc), (_pSrcSizeInBytes) )
#else
#   define vke_sprintf(_pBuff, _buffSize, _pFormat, ...) snprintf((_pBuff), (_buffSize), (_pFormat), __VA_ARGS__)
#   define vke_wsprintf(_pBuff, _buffSize, _pFormat, ...) wnwprintf((_pBuff), (_buffSize), (_pFormat), __VA_ARGS__)
#   define vke_strcpy(_pDst, _dstSize, _pSrc) strncpy( (_pDst), (_pSrc), (_dstSize) )
#   define vke_wstrcpy(_pDst, _dstSize, _pSrc) std::wcscpy( (_pDst), (_dstSize), (_pSrc) )
#   define vke_mbstowcs(_errOut, _pDst, _pDstSizeInWors, _pSrc, _pSrcSizeInBytes) do { (_errOut) = mbstowcs( (_pDst), (_pDstSizeInWors), (_pSrc), (_pSrcSizeInBytes) ); } while(0,0)
#   define vke_wcstombs(_errOut, _pDst, _pDstSizeInWors, _pSrc, _pSrcSizeInBytes) do{ (_errOut) = wcstombs( (_pDst), (_pDstSizeInWors), (_pSrc), (_pSrcSizeInBytes) ); } while(0,0)
#endif

} // VKE

#if VKE_COMPILER_VISUAL_STUDIO
#   pragma warning( pop )
#else
#   pragma GCC diagnostic pop
#endif