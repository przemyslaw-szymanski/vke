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
            static const int32_t COUNT_OPTIMAL = 0;
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

    template<class CallbackFunction> void VAIterate( CallbackFunction&& Callback ) { ( void )Callback; }

    template<class CallbackFunction, class HeadType, class ... TailType>
    void VAIterate( CallbackFunction&& Callback, HeadType&& head, TailType&&... tail )
    {
        Callback( head );
        VAIterate( Callback, tail... );
    }

    namespace Utils
    {
        
    } // Utils

    namespace Threads
    {

    } // Threads

    namespace Core
    {
        VKE_DECLARE_HANDLE2( Image, handle_t );
    } // Core
} // VKE