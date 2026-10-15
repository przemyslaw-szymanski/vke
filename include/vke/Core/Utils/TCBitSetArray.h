#pragma once

#include <bit>

namespace VKE::Utils
{
    template< typename T, size_t NumElements = 1 >
    class TCBitSetArray
    {
    public:
        using DataType = T;

        static constexpr size_t BITS_PER_ELEMENT = sizeof( DataType ) * 8;
        static constexpr size_t BITS_COUNT       = BITS_PER_ELEMENT * NumElements;

        class SReference
        {
            friend class TCBitSetArray;

        public:
            SReference& operator=( const bool value )
            {
                const T mask_all = -static_cast< T >( value );
                m_Data           = static_cast< T >( ( m_Data & ~m_mask ) | ( mask_all & m_mask ) );
                return *this;
            }

            SReference& operator=( const SReference& Other )
            {
                return *this = static_cast< bool >( Other );
            }

            operator bool() const
            {
                return ( m_Data & m_mask ) != 0;
            }

            bool operator~() const
            {
                return ( m_Data & m_mask ) == 0;
            }

        private:
            constexpr SReference( DataType& Data, const DataType mask ) : m_Data( Data ), m_mask( mask )
            {
            }

            DataType&      m_Data;
            const DataType m_mask;
        };

        constexpr TCBitSetArray() = default;

        SReference operator[]( const size_t index )
        {
            VKE_ASSERT2( index < BITS_COUNT, "Out of range access" );
            return SReference( m_aBits[ _GetElementIndex( index ) ], _GetMask( index ) );
        }

        constexpr bool operator[]( const size_t index ) const
        {
            VKE_ASSERT2( index < BITS_COUNT, "Out of range access" );
            return ( m_aBits[ _GetElementIndex( index ) ] & _GetMask( index ) ) != 0;
        }

        template<typename FuncT>
        void ForEachSetBit( FuncT&& Func ) const
        {
            for( size_t i = 0; i < NumElements; i++ )
            {
                DataType bits = m_aBits[ i ];
                while( bits != 0 )
                {
                    const size_t bit = static_cast< size_t >( std::countr_zero( bits ) );
                    Func( i * BITS_PER_ELEMENT + bit );
                    bits &= static_cast< DataType >( bits - 1 );
                }
            }
        }

        template< typename IntegralViewType >
        SReference& operator=( const IntegralViewType value )
        {
        }

        bool IsEmpty() const
        {
            DataType isEmpty = 0;

            for( size_t i = 0; i < NumElements; i++ )
            {
                isEmpty |= m_aBits[ i ];
            }

            return isEmpty == 0;
        }

        void Reset()
        {
            for( size_t i = 0; i < NumElements; ++i )
            {
                m_aBits[ i ] = 0;
            }
        }

    protected:
        DataType m_aBits[ NumElements ] = {};

        static constexpr size_t _GetElementIndex( const size_t index )
        {
            return index / BITS_PER_ELEMENT;
        }

        static constexpr DataType _GetMask( const size_t index )
        {
            return static_cast< DataType >( DataType( 1 ) << ( index % BITS_PER_ELEMENT ) );
        }
    };

    // TCBitSetArray< uint16_t, 4 > myBits;
    //// myBits expected to be [ 00000000 00000000 00000000 00000000 ]
    // myBits[ 0 ] = true;
    //// myBits expected to be [ 00000001 00000000 00000000 00000000 ]
    // myBits[ 10 ] = false;
    //// myBits expected to be [ 00000001 00000100 00000000 00000000 ]
    // myBits[ 0 ] = false;
    //// myBits expected to be [ 00000000 00000100 00000000 00000000 ]
    // bool bit10 = myBits[ 10 ]; // expects true
    // bool bit11 = myBits[ 11 ]; // expects false

}; // namespace VKE::Utils