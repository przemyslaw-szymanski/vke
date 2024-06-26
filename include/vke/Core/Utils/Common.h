#pragma once

#include "Core/VKECommon.h"

namespace VKE
{
    namespace Utils
    {
        template<typename InputItr>
        class TCRoundCounter
        {
            public:

            TCRoundCounter()
            {}

            explicit TCRoundCounter(InputItr Begin, InputItr End) :
                m_Begin(Begin),
                m_End(End),
                m_Curr(Begin)
            {}

            InputItr    GetNext()
            {
                if( m_Curr == m_End )
                {
                    m_Curr = m_Begin;
                }
                return m_Curr++;
            }

            protected:

                const InputItr    m_Begin;
                const InputItr    m_End;
                InputItr          m_Curr;
        };

        struct Hash
        {
            //https://www.codeproject.com/Articles/716530/Fastest-Hash-Function-for-Table-Lookups-in-C
            // Dedicated to Pippip, the main character in the 'Das Totenschiff' roman, actually the B.Traven himself,
            // his real name was Hermann Albert Otto Maksymilian Feige.
// CAUTION: Add 8 more bytes to the buffer being hashed, usually malloc(...+8) - to prevent out of boundary reads!
// Many thanks go to Yurii 'Hordi' Hordiienko, he lessened with 3 instructions the original 'Pippip', thus:
//#include <stdlib.h>
//#include <stdint.h>
#define _PADr_KAZE( x, n ) ( ( ( x ) << ( n ) ) >> ( n ) )
            uint32_t FNV1A_Pippip_Yurii( const char* str, size_t wrdlen )
            {
                const uint32_t PRIME = 591798841;
                uint32_t hash32;
                uint64_t hash64 = 14695981039346656037;
                size_t Cycles, NDhead;
                if( wrdlen > 8 )
                {
                    Cycles = ( ( wrdlen - 1 ) >> 4 ) + 1;
                    NDhead = wrdlen - ( Cycles << 3 );

                    for( ; Cycles--; str += 8 )
                    {
                        hash64 = ( hash64 ^ ( *( uint64_t* )( str ) ) ) * PRIME;
                        hash64 = ( hash64 ^ ( *( uint64_t* )( str + NDhead ) ) ) * PRIME;
                    }
                }
                else
                    hash64 = ( hash64 ^ _PADr_KAZE( *( uint64_t* )( str + 0 ), ( 8 - wrdlen ) << 3 ) ) * PRIME;
                hash32 = ( uint32_t )( hash64 ^ ( hash64 >> 32 ) );
                return hash32 ^ ( hash32 >> 16 );
            } // Last update: 2019-Oct-30, 14 C lines strong, Kaze.

            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const char* pPtr )
            {
                /*hash_t& tmp = *pInOut;
                if( pPtr )
                {
                    for( auto c = *pPtr; c; c = *pPtr++ )
                    {
                        tmp ^= c + MagicNumber + ( tmp << 6 ) + ( tmp >> 2 );
                    }
                }
                else
                {
                    tmp ^= CalcMagic( tmp );
                }*/
                if( pPtr )
                {
                    const int p = 31;
                    const int m = (int)1e9 + 9;
                    hash_t& hash_value = *pInOut;
                    long long p_pow = 1;
                    for( auto c = *pPtr; c; c = *pPtr++ )
                    {
                        hash_value = ( hash_value + ( c - 'a' + 1 ) * p_pow ) % m;
                        p_pow = ( p_pow * p ) % m;
                    }
                }
                else
                {
                    *pInOut ^= CalcMagic( *pInOut );
                }
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const wchar_t* pPtr )
            {
                hash_t& tmp = *pInOut;
                if( pPtr )
                {
                    for( auto c = *pPtr; c; c = *pPtr++ )
                    {
                        tmp ^= c + MagicNumber + ( tmp << 6 ) + ( tmp >> 2 );
                    }
                }
                else
                {
                    tmp ^= CalcMagic( tmp );
                }
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const uint8_t& v )
            {
                *pInOut ^= std::hash<uint8_t>{}( v ) + CalcMagic( *pInOut );
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const int8_t& v )
            {
                *pInOut ^= std::hash<int8_t>{}( v ) + CalcMagic( *pInOut );
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const uint16_t& v )
            {
                *pInOut ^= std::hash<uint16_t>{}( v ) + CalcMagic( *pInOut );
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const int16_t& v )
            {
                *pInOut ^= std::hash<int16_t>{}( v ) + CalcMagic( *pInOut );
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const uint32_t& v )
            {
                *pInOut ^= std::hash<uint32_t>{}( v ) + CalcMagic( *pInOut );
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const int32_t& v )
            {
                *pInOut ^= std::hash<int32_t>{}( v ) + CalcMagic( *pInOut );
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const uint64_t& v )
            {
                *pInOut ^= std::hash<uint64_t>{}( v ) + CalcMagic( *pInOut );
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const int64_t& v )
            {
                *pInOut ^= std::hash<int64_t>{}( v ) + CalcMagic( *pInOut );
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const double& v )
            {
                *pInOut ^= std::hash<double>{}( v ) + CalcMagic( *pInOut );
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const float& v )
            {
                *pInOut ^= std::hash<float>{}( v ) + CalcMagic( *pInOut );
            }
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const void* v )
            {
                if( v )
                {
                    *pInOut ^= std::hash<const void*>{}( v ) + CalcMagic( *pInOut );
                }
                else
                {
                    *pInOut ^= CalcMagic( *pInOut );
                }
            }
            template<uint32_t MagicNumber = 0x9e3779b9> static vke_force_inline hash_t CalcMagic( const hash_t& v )
            {
                return MagicNumber + ( v << 6 ) + ( v >> 2 );
            }
            template<typename T> static hash_t Calc( const T& v )
            {
                std::hash<T> h( v );
                return h;
            }
        };

        struct SHash
        {
            hash_t value = 0;
            template<typename T, uint32_t MagicNumber = 0x9e3779b9> SHash& operator+=( const T& v )
            {
                Hash::Combine<MagicNumber>( &value, v );
                return *this;
            }
            template<typename... ARGS> void Combine( ARGS&&... args )
            {
                VAIterate( [ & ]( const auto& arg ) { Hash::Combine( &value, arg ); }, args... );
            }

          protected:
            template<typename HeadType, typename... TailTypes>
            void _Combine( const HeadType& first, TailTypes&&... tail )
            {
                Hash::Combine( &value, first );
                _Combine( first, tail... );
            }
        };
    } // Utils
} // VKE