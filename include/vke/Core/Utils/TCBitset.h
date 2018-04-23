#ifndef __VKE_TCBITSET_H__
#define __VKE_TCBITSET_H__

#include "Core/VKECommon.h"

namespace VKE
{
    namespace Utils
    {
        template<typename _T_>
        class TCBitset
        {
            public:

                TCBitset()
                {
                    static_assert(std::is_same< _T_, uint8_t  >::value || std::is_same< _T_, uint16_t >::value ||
                                  std::is_same< _T_, uint32_t >::value || std::is_same< _T_, uint64_t >::value, 
                                  "Wrong template parameter");
                }

                TCBitset(const TCBitset& Other) : TCBitset( Other.m_bits ) {}
                explicit TCBitset(const _T_& bits) : m_bits(bits)
                {
                    static_assert(std::is_same< _T_, uint8_t  >::value || std::is_same< _T_, uint16_t >::value ||
                                  std::is_same< _T_, uint32_t >::value || std::is_same< _T_, uint64_t >::value, 
                                  "Wrong template parameter");
                }

                virtual     ~TCBitset() { Reset(); }

                void Reset() { m_bits = 0; }

                bool Contains(const _T_& value) { return (m_bits & value) == value; }
                _T_ And(const _T_& value) { return m_bits & value; }
                _T_ Or(const _T_& value) { return m_bits | value; }
                _T_ Xor(const _T_& value) { return m_bits ^ value; }
                _T_ Not() { return ~m_bits; }
                void Set(const _T_& bits) { m_bits = bits; }

                TCBitset& Add(const _T_& bits) { m_bits |= bits; return *this; }
                TCBitset& Remove(const _T_& bits) { m_bits |= ~bits; return *this; }

                const _T_& Get() const { return m_bits; }

                _T_ operator+(const _T_& bits) { return Or(bits); }
                TCBitset& operator+=(const _T_& bits) { return Add(bits); }
                TCBitset& operator-=(const _T_& bits) { return Remove(bits); }
                bool operator==(const _T_& bits) { return Contains(bits);  }
                void operator=(const _T_& bits) { Set(bits); }

            protected:
                
                _T_     m_bits;
        };
    } // Utils

    using BitsetU8  = Utils::TCBitset< uint8_t >;
    using BitsetU16 = Utils::TCBitset< uint16_t >;
    using BitsetU32 = Utils::TCBitset< uint32_t >;
    using BitsetU64 = Utils::TCBitset< uint64_t >;

} // VKE

#endif // __VKE_TCBITSET_H__