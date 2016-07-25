#ifndef __VKE_STL_UTILS_H__
#define __VKE_STL_UTILS_H__

#include "VKETypes.h"

namespace VKE
{
    namespace Utils
    {
        namespace Map
        {
            template<typename _KEY_, typename _VALUE_>
            auto Insert(std::map<_KEY_, _VALUE_>& mMap, const _KEY_& tKey, const _VALUE_& tValue) ->
                decltype(std::map<_KEY_, _VALUE_>::iterator)
            {
                return mMap.insert(std::map<_KEY_, _VALUE_>::value_type(tKey, tValue)).first;
            }

            template<typename _KEY_, typename _VALUE_>
            auto Insert(std::unordered_map<_KEY_, _VALUE_>& mMap, const _KEY_& tKey, const _VALUE_& tValue) ->
                decltype(std::unordered_map<_KEY_, _VALUE_>::iterator)
            {
                return mMap.insert(std::unordered_map<_KEY_, _VALUE_>::value_type(tKey, tValue)).first;
            }

        } // Map
    } // Utils
} // VKE

#endif // __VKE_STL_UTILS_H__