#pragma once

#include "TCDynamicArray.h"

namespace VKE
{
    namespace Utils
    {
        template
        <
            typename T,
            typename DEFAULT_COUNT = 1024,
            typename HandleType = uint32_t,
            class DataContainerType = VKE::Utils::TCDynamicArray< T, DEFAULT_COUNT >,
            class HandleContainerType = VKE::Utils::TCDynamicArray< HandleType, DEFAULT_COUNT >
        >
        class TCHandledBuffer
        {
            public:

                using HandleBuffer = Utils::TCDynamicArray< HandleContainerType >;
                using FreeHandleBuffer = Utils::TCDynamicArray< HandleType, DEFAULT_COUNT >;

            public:

                TCHandledBuffer()
                {
                    // Add null handle
                    PushBack( T() );
                }

                ~TCHandledBuffer()
                {}

                HandleType PushBack(const T& Data)
                {
                    HandleType idx;
                    if( m_vFreeHandles.PopBack( &idx ) )
                    {
                        m_vBuffer[ idx ] = Data;
                        m_vHandleMap[ idx ] = idx;
                    }
                    else
                    {
                        uint32_t handleIdx = m_vBuffer.PushBack( Data );
                        idx = m_vHandleMap.PushBack( handleIdx );
                    }
                    return idx;
                }

                void Free(const HandleType& idx)
                {
                    m_vFreeHandles.PushBack( idx );
                }

                void Remove(const HandleType& idx)
                {
                    m_vBuffer.Remove( idx );
                    m_vHandleMap.Remove( idx );
                    for( uint32_t i = idx; i < m_vHandleMap.GetCount(); ++i )
                    {
                        m_vHandleMap[ i ] = i;
                    }
                }

                T& At(const HandleType& idx)
                {
                    return m_vBuffer[ _GetDataIdx( idx ) ];
                }

                const T& At(const HAndleType& idx) const
                {
                    return m_vBuffer[ _GetDataIdx( idx ) ];
                }

            protected:

                vke_force_inline
                HandleType _GetDataIdx(const HandleType& idx) const
                {
                    return m_vHandleMap[ idx ];
                }

            protected:

                FreeHandleBuffer    m_vFreeHandles;
                DataContainerType   m_vBuffer;
                HandleBuffer        m_vHandleMap;
        };
    } // Utils
} // VKE