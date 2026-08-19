#ifndef __VKE_TCBITSET_H__
#define __VKE_TCBITSET_H__

#include "Core/VKECommon.h"
#include "Core/Platform/CPlatform.h"

namespace VKE
{
    namespace Utils
    {
        template<typename T>
        concept IsIntegralType = std::is_integral_v< T > || std::is_enum_v< T >;

        template<typename T, typename U>
        concept IsSameUnderlyingType =
            ( std::is_enum_v< U > && std::is_integral_v< T > && std::is_same_v< std::underlying_type_t< U >, T > ) ||
            ( std::is_enum_v< T > && std::is_integral_v< U > && std::is_same_v< std::underlying_type_t< T >, U > );

        template< typename T, typename U >
        concept IsCompatibleIntegralType =
            ( ( IsIntegralType< T > ) && (IsIntegralType< U >)) &&
            ( std::is_same_v< U, T > || std::is_convertible_v< U, T > || IsSameUnderlyingType< T, U > ) && ( sizeof( U ) <= sizeof( T ) );

        template< typename T >
            requires IsIntegralType< T >
        class TCBitset
        {
        public:
            using DataType = T;

            TCBitset()
            {
                static_assert( ( std::is_same< T, uint8_t >::value || std::is_same< T, uint16_t >::value ||
                                 std::is_same< T, uint32_t >::value || std::is_same< T, uint64_t >::value ) ||
                                   std::is_enum< T >::value,
                               "Wrong template parameter" );
                static_assert( sizeof( T ) <= sizeof( uint64_t ), "Wrong template parameter" );
            }

            template< typename U >
            TCBitset( const TCBitset< U >& Other ) : TCBitset( static_cast< T >( Other.m_bits ) )
            {
                CheckOverflow( Other.m_bits );
            }

            template< typename U >
            TCBitset( U bits ) : m_bits( static_cast< T >( bits ) )
            {
                CheckOverflow( bits );
                static_assert( ( std::is_same< T, uint8_t >::value || std::is_same< T, uint16_t >::value ||
                                 std::is_same< T, uint32_t >::value || std::is_same< T, uint64_t >::value ) ||
                                   std::is_enum< T >::value,
                               "Wrong template parameter" );
                static_assert( sizeof( T ) <= sizeof( uint64_t ), "Wrong template parameter" );
            }

            /*explicit TCBitset( uint32_t v )
                : m_bits( v )
            {
                static_assert( sizeof( T ) >= sizeof( uint32_t ), "Could not set more bits for this type" );
            }
            explicit TCBitset( uint64_t v )
                : m_bits( v )
            {
                static_assert( sizeof( T ) >= sizeof( uint64_t ),
                    "Could not set more bits for this type" );
            }*/

            ~TCBitset()
            {
            }

            void Reset()
            {
                m_bits = (T)0;
            }

            template<typename U>
            void vke_force_inline CheckOverflow(U bits) const
            {
                if constexpr( sizeof( U ) > sizeof( T ) )
                {
                    if constexpr( std::is_enum_v< T > )
                    {
                        VKE_ASSERT2( ( bits ) <= std::numeric_limits< std::underlying_type_t< T > >::max(), "Bitset overflow" );
                    }
                    else
                    {
                        VKE_ASSERT2( bits <= std::numeric_limits< T >::max(), "Bitset overflow" );
                    }
                }
            }

            template<typename U>
            bool Contains( U bits ) const
            {
                CheckOverflow( bits );
                return ( m_bits & bits ) != 0;
            }

            bool Contains( TCBitset Bits ) const
            {
                return Contains( (T)Bits.m_bits );
            }

            template< typename U >
            T And( U bits ) const
            {
                CheckOverflow( bits );
                return (T)( m_bits & bits );
            }

            template< typename U >
            T Or( U bits ) const
            {
                CheckOverflow( bits );
                return (T)( m_bits | bits );
            }

            template< typename U >
            T Xor( U bits ) const
            {
                CheckOverflow( bits );
                return (T)( m_bits ^ bits );
            }

            T Not() const
            {
                return (T)~m_bits;
            }

            template< typename U >
            T Set( U bits )
            {
                CheckOverflow( bits );
                m_bits = static_cast< T >( bits );
                return m_bits;
            }

            bool IsBitSet( const uint8_t idx ) const
            {
                return And( Bit( idx ) ) != 0;
            }

            void SetBit( const uint8_t idx )
            {
                m_bits |= Bit( idx );
            }

            void ClearBit( const uint8_t idx )
            {
                m_bits &= ~Bit( idx );
            }

            /// <summary>
            /// Sets bit at position bitIndex to 1 if set is true, otherwise sets it to 0
            /// </summary>
            /// <param name="idx">bit position numbering from 0</param>
            /// <returns>Current bit mask</returns>
            vke_force_inline T SetBit( uint8_t bitIndex, bool set )
            {
                m_bits = ( m_bits & ~( 1U << bitIndex ) ) | ( static_cast< T >( set ) << bitIndex );
                return m_bits;
            }

            template< uint8_t BitIndex >
            vke_force_inline T SetBit( bool set )
            {
                m_bits = ( m_bits & ~( 1U << BitIndex ) ) | ( static_cast< T >( set ) << BitIndex );
                return m_bits;
            }

            T Bit( const uint8_t idx ) const
            {
                return (T)( 1 << idx );
            }

            static const uint8_t GetBitCount()
            {
                return sizeof( T ) * 8;
            }

            template< typename U >
            TCBitset& Add( U bits )
            {
                CheckOverflow( bits );
                if constexpr( std::is_enum_v< T > )
                {
                    m_bits = static_cast< T >( static_cast< std::underlying_type_t< T > >( m_bits ) |
                                               static_cast< std::underlying_type_t< T > >( bits ) );
                }
                else
                {
                    m_bits |= bits;
                }

                return *this;
            }

            template< typename U >
            TCBitset& Remove( U bits )
            {
                CheckOverflow( bits );
                if constexpr( std::is_enum_v< T > )
                {
                    m_bits = static_cast< T >( static_cast< std::underlying_type_t< T > >( m_bits ) &
                                               ~static_cast< std::underlying_type_t< T > >( bits ) );
                }
                else
                {
                    m_bits &= ~bits;
                }
                return *this;
            }

            T Get() const
            {
                return (T)m_bits;
            }

            template< typename U >
            T operator+( U bits )
            {
                return Or( bits );
            }

            template< typename U >
            TCBitset& operator+=( U bits )
            {
                return Add( bits );
            }

            template< typename U >
            TCBitset& operator-=( U bits )
            {
                return Remove( bits );
            }

            template< typename U >
            bool operator==( U bits ) const
            {
                return Contains( bits );
            }

            template< typename U >
            bool operator!=( U bits ) const
            {
                return !Contains( bits );
            }

            template< typename U >
            TCBitset& operator=( U bits )
            {
                Set( bits );
                return *this;
            }

            template< typename U >
            void operator|=( U bits )
            {
                Add( bits );
            }

            template< typename U >
            TCBitset& operator|( U bits )
            {
                return Or( bits );
            }

            T operator+( TCBitset r )
            {
                return Or( (T)r.m_bits );
            }

            TCBitset& operator+=( TCBitset r )
            {
                return Add( (T)r.m_bits );
            }

            TCBitset& operator-=( TCBitset r )
            {
                return Remove( (T)r.m_bits );
            }

            bool operator==( TCBitset r ) const
            {
                return Contains( (T)r.m_bits );
            }

            bool operator!=( TCBitset r ) const
            {
                return !Contains( (T)r.m_bits );
            }

            void operator=( TCBitset r )
            {
                Set( (T)r.m_bits );
            }

            void operator|=( TCBitset r )
            {
                Add( (T)r.m_bits );
            }

            TCBitset operator|( TCBitset r )
            {
                return Or( (T)r.m_bits );
            }

            /// <summary>
            /// Calculates number of bits set in the bitset
            /// </summary>
            /// <returns></returns>
            uint8_t CalcSetBitCount() const
                requires std::is_integral_v< T >
            {
                if constexpr( std::numeric_limits< T >::digits <= 8 )
                {
                    return (uint8_t)Platform::CountBits( (uint8_t)m_bits );
                }
                else if constexpr( std::numeric_limits< T >::digits <= 16 )
                {
                    return (uint8_t)Platform::CountBits( (uint16_t)m_bits );
                }
                else if constexpr( std::numeric_limits< T >::digits <= 32 )
                {
                    return (uint8_t)Platform::CountBits( (uint32_t)m_bits );
                }
                else
                {
                    return (uint8_t)Platform::CountBits( (uint64_t)m_bits );
                }
            }

            /// <summary>
            /// Calculates number of bits set in the bitset
            /// </summary>
            /// <returns></returns>
            uint8_t CalcSetBitCount() const
                requires std::is_enum_v< T >
            {
                if constexpr( std::numeric_limits< std::underlying_type_t< T > >::digits <= 8 )
                {
                    return (uint8_t)Platform::CountBits( (uint8_t)m_bits );
                }
                else if constexpr( std::numeric_limits< std::underlying_type_t< T > >::digits <= 16 )
                {
                    return (uint8_t)Platform::CountBits( (uint16_t)m_bits );
                }
                else if constexpr( std::numeric_limits< std::underlying_type_t< T > >::digits <= 32 )
                {
                    return (uint8_t)Platform::CountBits( (uint32_t)m_bits );
                }
                else
                {
                    return (uint8_t)Platform::CountBits( (uint64_t)m_bits );
                }
            }

            /// <summary>
            /// Calculates number of bits not set in the bitset
            /// </summary>
            /// <returns></returns>
            uint8_t CalcNotSetBitCount() const
            {
                return std::numeric_limits< T >::digits - CalcSetBitCount();
            }

            friend std::ostream& operator<<( std::ostream& o, TCBitset< T > data );

        protected:
            T m_bits;
        };

        template< typename T >
        std::ostream& operator<<( std::ostream& o, TCBitset< T > data )
        {
            constexpr auto bitCount = sizeof( T ) * 8;
            for( uint32_t i = 0; i < bitCount; ++i )
            {
                o << data.Bit( i );
            }
            return o;
        }

    } // namespace Utils

    using BitsetU8  = Utils::TCBitset< uint8_t >;
    using BitsetU16 = Utils::TCBitset< uint16_t >;
    using BitsetU32 = Utils::TCBitset< uint32_t >;
    using BitsetU64 = Utils::TCBitset< uint64_t >;

} // namespace VKE

#endif // __VKE_TCBITSET_H__