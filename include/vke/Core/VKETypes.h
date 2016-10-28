#ifndef __VKE_TYPES_H__
#define __VKE_TYPES_H__

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

#include <cassert>

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
    
    static const std::string EMPTY_STRING = "";
    static const std::wstring EMPTY_WSTRING = L"";
    static const uint8_t UNDEFINED = 0;
    static const uint8_t UNKNOWN = 0;
    static const uint8_t NONE = 0;

    template<typename _T_>
    struct TSExtent
    {
        union
        {
            struct
            {
                _T_ width;
                _T_ height;
            };

            struct
            {
                _T_ begin;
                _T_ end;
            };

            struct
            {
                _T_ x;
                _T_ y;
            };
        };

        TSExtent() {}
        TSExtent(const TSExtent& Other) : x(Other.x), y(Other.y) {}
        TSExtent(const _T_& v1, const _T_& v2) : x(v1), y(v2) {}
        explicit TSExtent(const _T_& v) : x(v), y(v) {}
    };

    using handle_t = uint64_t;

    static const handle_t RANDOM_HANDLE = std::numeric_limits<handle_t>::max();
    static const handle_t NULL_HANDLE = 0;

    using ExtentI32 = TSExtent< int32_t >;
    using ExtentI16 = TSExtent< int16_t >;
    using ExtentI64 = TSExtent< int64_t >;

    using ExtentU32 = TSExtent< uint32_t >;
    using ExtentU16 = TSExtent< uint16_t >;
    using ExtentU64 = TSExtent< uint64_t >;

    using ExtentF32 = TSExtent< float >;
    using ExtentF64 = TSExtent< double >;

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
#endif // VKE_USE_STL_PORT

    using StringVec = vke_vector< vke_string >;
    using CStrVec = vke_vector< cstr_t >;

} // VKE

#if VKE_COMPILER_VISUAL_STUDIO
#   pragma warning( pop )
#else
#   pragma GCC diagnostic pop
#endif

#endif // __VKE_TYPES_H__