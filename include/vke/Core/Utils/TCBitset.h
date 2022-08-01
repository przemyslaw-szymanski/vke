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
                }

                TCBitset(const TCBitset& Other) : TCBitset( Other.m_bits ) {}
                TCBitset(const T& bits) : m_bits(bits)
                {
                    static_assert( ( std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value ||
                                     std::is_same<T, uint32_t>::value || std::is_same<T, uint64_t>::value ) ||
                                       std::is_enum<T>::value,
                                   "Wrong template parameter" );
                }

                ~TCBitset() { Reset(); }

                void Reset() { m_bits = (T)0; }

                bool Contains(const T& value) const { return (m_bits & value) == value; }
                T And(const T& value) const { return m_bits & value; }
                T Or(const T& value) const { return m_bits | value; }
                T Xor(const T& value) const { return m_bits ^ value; }
                T Not() const { return ~m_bits; }
                void Set(const T& bits) { m_bits = bits; }
                bool IsBitSet( const uint8_t idx ) const { return And( Bit( idx ) ) != 0; }
                void SetBit( const uint8_t idx ) { m_bits |= Bit( idx ); }
                void ClearBit( const uint8_t idx ) { m_bits &= ~Bit( idx ); }
                template<bool IsSet>
                void SetBit(const uint8_t idx) { IsSet ? SetBit( idx ) : ClearBit( idx ); }
                T Bit( const uint8_t idx ) const { return (T)1 << idx; }
                static const uint8_t GetBitCount() { return sizeof( T ) * 8; }

                TCBitset& Add(const T& bits) { m_bits |= bits; return *this; }
                TCBitset& Remove(const T& bits) { m_bits |= ~bits; return *this; }

                const T& Get() const { return m_bits; }

                T operator+(const T& bits) { return Or(bits); }
                TCBitset& operator+=(const T& bits) { return Add(bits); }
                TCBitset& operator-=(const T& bits) { return Remove(bits); }
                bool operator==(const T& bits) const { return Contains(bits);  }
                bool operator!=(const T& bits) const { return !Contains(bits); }
                void operator=(const T& bits) { Set(bits); }
                void operator|=( const T& bits ){ Add(bits); }
                TCBitset operator|( const T& bits ){ return Or(bits); }
                T operator+(const TCBitset& r) { return Or(r.m_bits); }
                TCBitset& operator+=(const TCBitset& r) { return Add(r.m_bits); }
                TCBitset& operator-=(const TCBitset& r) { return Remove(r.m_bits); }
                bool operator==(const TCBitset& r) const { return Contains(r.m_bits);  }
                bool operator!=(const TCBitset& r) const { return !Contains(r.m_bits); }
                void operator=(const TCBitset& r) { Set(r.m_bits); }
                void operator|=( const TCBitset& r ){ Add(r.m_bits); }
                TCBitset operator|( const TCBitset& r ){ return Or(r.m_bits); }

                uint8_t CalcSetBitCount() const { return Platform::PopCnt( m_bits ); }
                uint8_t CalcNotSetBitCount() const { return std::numeric_limits< T >::digits - CalcSetBitCount(); }

            protected:

                T     m_bits;
        };

    } // Utils

    using BitsetU8  = Utils::TCBitset< uint8_t >;
    using BitsetU16 = Utils::TCBitset< uint16_t >;
    using BitsetU32 = Utils::TCBitset< uint32_t >;
    using BitsetU64 = Utils::TCBitset< uint64_t >;

} // VKE

#endif // __VKE_TCBITSET_H__