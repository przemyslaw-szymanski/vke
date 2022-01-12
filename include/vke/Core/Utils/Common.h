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
            template<uint32_t MagicNumber = 0x9e3779b9>
            static vke_force_inline void Combine( hash_t* pInOut, const char* pPtr )
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