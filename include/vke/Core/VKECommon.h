#pragma once

#include "VKEPreprocessor.h"
#include "VKEErrorHandling.h"
#include "Core/VKETypes.h"

namespace VKE
{
    struct EventReportTypes
    {
        enum TYPE
        {
            NONE,
            ASSERT_ON_ALLOC,
            THROW_ON_ALLOC,
            ASSERT_ON_DEALLOC,
            THROW_ON_DEALLOC,
            LOG_ON_ALLOC,
            LOG_ON_DEALLOC,
            _MAX_COUNT
        };
    };
    using EVENT_REPORT_TYPE = EventReportTypes::TYPE;

    namespace Constants
    {
        static const uint32_t   ENGINE_VERSION = 1000;
        static const cstr_t     ENGINE_NAME = "Vulkan Engine";
        static const uint32_t   MAX_NAME_LENGTH = 256;

        namespace Threads
        {
            static const int32_t COUNT_OPTIMAL = 0xFFFFFFFF;
        } // Threads

        namespace RenderSystem
        {
            static const uint32_t MAX_RENDER_QUEUES = 1;
            static const uint32_t MAX_PHYSICAL_DEVICES = 10;
            static const uint32_t MAX_PHYSICAL_DEVICES_QUEUE_FAMILY_PROPERTIES = 10;
            static const uint32_t MAX_EXTENSION_COUNT = 10;
            static const uint32_t MAX_SWAP_CHAIN_ELEMENTS = 10;
            static const uint32_t DEFAULT_DRAWCALL_COUNT = 1000;
        } // RenderSystem

        struct _SOptimal
        {
            template<typename _T_>
            static const _T_ Value() { return std::numeric_limits< _T_ >::max(); }
            operator uint16_t() const { return Value<uint16_t>(); }
            operator uint32_t() const { return Value<uint32_t>(); }
            template<typename _T_>
            static bool IsOptimal(const _T_& v) { return v == Value<_T_>(); }
        };

        static _SOptimal OPTIMAL;
#define VKE_OPTIMAL (~0UL)

    } // Constants

#define VKE_BIT(_bit) (1ULL << (_bit))
#define VKE_SET_BIT(_value, _bit) ( ( _value ) |= VKE_BIT( ( _bit ) ) )
#define VKE_UNSET_BIT(_value, _bit) ( ( _value ) &= ~VKE_BIT( ( _bit ) ) )
#define VKE_SET_MASK(_value, _mask) ( ( _value ) |= ( _mask ) )
#define VKE_UNSET_MASK(_value, _mask) ( ( _value ) &= ~( _mask ) )
#define VKE_GET_BIT(_value, _bit) ((_value) & VKE_BIT(_bit))
#define VKE_CALC_MAX_VALUE_FOR_BITS(_bitCount) ( ( 1 << ( _bitCount ) ) - 1 )

    void VKE_API DebugBreak( cstr_t pMsg );

    //template<typename ... ArgsT>
    static
    void Assert( bool condition, cstr_t pConditionMsg, uint32_t flags, cstr_t pFile, cstr_t pFunction,
        uint32_t line, cstr_t pMsg )
    {
        if( !condition )
        {

            char buff[4096];
            vke_sprintf( buff, sizeof( buff ), "VKE ASSERT: [%d][%s][%s][%d]:\n\"%s\": %s\n",
                flags, pFile, pFunction, line, pConditionMsg, pMsg );

            DebugBreak( buff );
        }
    }

    enum DEFAULT_CTOR_INIT
    {
        DEFAULT_CONSTRUCTOR_INIT
    };

    template<class CallbackFunction, class HeadType, class ... TailType>
    void VAIterate( const CallbackFunction& Callback, const HeadType& head, TailType... tail )
    {
        Callback( head );
        VAIterate( Callback, tail... );
    }

    template<class CallbackFunction>
    void VAIterate( const CallbackFunction& Callback )
    {
        ( void )Callback;
    }

    struct Hash
    {
        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const char* pPtr)
        {
            hash_t& tmp = *pInOut;
            if (pPtr)
            {
                for (auto c = *pPtr; c; c = *pPtr++)
                {
                    tmp ^= c + MagicNumber + (tmp << 6) + (tmp >> 2);
                }
            }
            else
            {
                tmp ^= CalcMagic(tmp);
            }
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const wchar_t* pPtr)
        {
            hash_t& tmp = *pInOut;

            if (pPtr)
            {
                for (auto c = *pPtr; c; c = *pPtr++)
                {
                    tmp ^= c + MagicNumber + (tmp << 6) + (tmp >> 2);
                }
            }
            else
            {
                tmp ^= CalcMagic(tmp);
            }
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine( hash_t* pInOut, const uint8_t& v )
        {
            *pInOut ^= std::hash<uint8_t>{}(v) + CalcMagic(*pInOut);
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const int8_t& v)
        {
            *pInOut ^= std::hash<int8_t>{}(v)+CalcMagic(*pInOut);
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const uint16_t& v)
        {
            *pInOut ^= std::hash<uint16_t>{}(v)+CalcMagic(*pInOut);
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const int16_t& v)
        {
            *pInOut ^= std::hash<int16_t>{}(v)+CalcMagic(*pInOut);
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const uint32_t& v)
        {
            *pInOut ^= std::hash<uint32_t>{}(v)+CalcMagic(*pInOut);
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const int32_t& v)
        {
            *pInOut ^= std::hash<int32_t>{}(v)+CalcMagic(*pInOut);
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const uint64_t& v)
        {
            *pInOut ^= std::hash<uint64_t>{}(v)+CalcMagic(*pInOut);
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const int64_t& v)
        {
            *pInOut ^= std::hash<int64_t>{}(v)+CalcMagic(*pInOut);
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const double& v)
        {
            *pInOut ^= std::hash<double>{}(v)+CalcMagic(*pInOut);
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const float& v)
        {
            *pInOut ^= std::hash<float>{}(v)+CalcMagic(*pInOut);
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline
        void Combine(hash_t* pInOut, const void* v)
        {
            if (v)
            {
                *pInOut ^= std::hash<const void*>{}(v)+CalcMagic(*pInOut);
            }
            else
            {
                *pInOut ^= CalcMagic(*pInOut);
            }
        }

        template<uint32_t MagicNumber = 0x9e3779b9>
        static vke_force_inline hash_t CalcMagic(const hash_t& v)
        {
            return MagicNumber + (v << 6) + (v >> 2);
        }

        template<typename T>
        static hash_t Calc( const T& v )
        {
            std::hash< T > h( v );
            return h;
        }
    };

    namespace Utils
    {
        struct SHash
        {
            hash_t value = 0;

            template<typename T, uint32_t MagicNumber = 0x9e3779b9>
            SHash& operator+=(const T& v)
            {
                Hash::Combine< MagicNumber >(&value, v);
                return *this;
            }

            template<typename ... ARGS>
            void Combine(ARGS&& ... args)
            {
                VAIterate(
                    [&](const auto& arg)
                {
                    Hash::Combine(&value, arg);
                },
                    args...);
            }

        protected:

            template<typename HeadType, typename ... TailTypes>
            void _Combine(const HeadType& first, TailTypes&& ... tail)
            {
                Hash::Combine(&value, first);
                _Combine(first, tail...);
            }
        };
    } // Utils

    namespace Threads
    {

    } // Threads

    namespace Core
    {
        VKE_DECLARE_HANDLE2( Image, handle_t );
    } // Core
} // VKE