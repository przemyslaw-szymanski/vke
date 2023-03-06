#ifndef __VKE_TCBITSET_H__
#define __VKE_TCBITSET_H__

#include "Core/VKECommon.h"
#include "Core/Platform/CPlatform.h"

namespace VKE
{
    namespace Utils
    {
        template<typename T>
        class TCBitset
        {
            public:

                using DataType = T;

                TCBitset()
                {
                    static_assert( ( std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value ||
                                     std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value ) ||
                                       std::is_enum<T>::value,
                                   "Wrong template parameter" );
                    static_assert( sizeof( T ) <= sizeof( uint64_t ), "Wrong template parameter" );
                }

                TCBitset(const TCBitset& Other) : TCBitset( Other.m_bits ) {}
                TCBitset(T bits) : m_bits(bits)
                {
                    static_assert( ( std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value ||
                                     std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value ) ||
                                       std::is_enum<T>::value,
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

                ~TCBitset() { }

                void Reset() { m_bits = (T)0; }

                bool Contains(T value) const { return (m_bits & value) == value; }
                bool Contains( TCBitset Bits ) const { return Contains(Bits.m_bits); }
                T And(T value) const { return m_bits & value; }
                T Or(T value) const { return m_bits | value; }
                T Xor(T value) const { return m_bits ^ value; }
                T Not() const { return ~m_bits; }
                void Set(T bits) { m_bits = bits; }
                bool IsBitSet( const uint8_t idx ) const { return And( Bit( idx ) ) != 0; }
                void SetBit( const uint8_t idx ) { m_bits |= Bit( idx ); }
                void ClearBit( const uint8_t idx ) { m_bits &= ~Bit( idx ); }
                template<bool IsSet>
                void SetBit(const uint8_t idx) { IsSet ? SetBit( idx ) : ClearBit( idx ); }
                T Bit( const uint8_t idx ) const { return (T)1 << idx; }
                static const uint8_t GetBitCount() { return sizeof( T ) * 8; }

                TCBitset& Add(T bits) { m_bits |= bits; return *this; }
                TCBitset& Remove(T bits) { m_bits &= ~bits; return *this; }

                T Get() const { return m_bits; }

                T operator+(T bits) { return Or(bits); }
                TCBitset& operator+=(T bits) { return Add(bits); }
                TCBitset& operator-=(T bits) { return Remove(bits); }
                bool operator==(T bits) const { return Contains(bits);  }
                bool operator!=(T bits) const { return !Contains(bits); }
                void operator=(T bits) { Set(bits); }
                void operator|=( T bits ){ Add(bits); }
                TCBitset operator|( T bits ){ return Or(bits); }
                T operator+(const TCBitset& r) { return Or(r.m_bits); }
                TCBitset& operator+=(const TCBitset& r) { return Add(r.m_bits); }
                TCBitset& operator-=(const TCBitset& r) { return Remove(r.m_bits); }
                bool operator==(const TCBitset& r) const { return Contains(r.m_bits);  }
                bool operator!=(const TCBitset& r) const { return !Contains(r.m_bits); }
                void operator=(const TCBitset& r) { Set(r.m_bits); }
                void operator|=( const TCBitset& r ){ Add(r.m_bits); }
                TCBitset operator|( const TCBitset& r ){ return Or(r.m_bits); }

                uint8_t CalcSetBitCount() const { return Platform::CountBits( m_bits ); }
                uint8_t CalcNotSetBitCount() const { return std::numeric_limits< T >::digits - CalcSetBitCount(); }

                friend std::ostream& operator<<( std::ostream& o, TCBitset<T> data );

            protected:

                T     m_bits;
        };

        template<typename T> std::ostream& operator<<(std::ostream& o, TCBitset<T> data)
        {
            constexpr auto bitCount = sizeof( T ) * 8;
            for(uint32_t i = 0; i < bitCount; ++i)
            {
                o << data.Bit( i );
            }
            return o;
        }

    } // Utils

    using BitsetU8  = Utils::TCBitset< uint8_t >;
    using BitsetU16 = Utils::TCBitset< uint16_t >;
    using BitsetU32 = Utils::TCBitset< uint32_t >;
    using BitsetU64 = Utils::TCBitset< uint64_t >;

} // VKE

#endif // __VKE_TCBITSET_H__