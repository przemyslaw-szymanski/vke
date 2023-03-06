#pragma once

#include "Core/VKECommon.h"
#include "Core/Platform/CPlatform.h"
#include "Core/Utils/TCBitset.h"
#include "Core/VKEConfig.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/TCString.h"

namespace VKE
{
    namespace Threads
    {
        class ITask;
    } // Threads

    enum _THREAD_SAFE
    {
        NO_THREAD_SAFE = 0,
        THREAD_SAFE = 1
    };

    namespace Threads
    {
        using SyncObject = Platform::Thread::CSpinlock;

        using LockGuard = std::lock_guard< std::mutex >;
        using UniqueLock = std::unique_lock< std::mutex >;

        struct TaskFlags
        {
            enum FLAG : uint16_t
            {
                UNKNOWN = 0x0,
                LOW_PRIORITY = VKE_BIT( 0 ),
                MEDIUM_PRIORITY = VKE_BIT( 1 ),
                HIGH_PRIORITY = VKE_BIT( 2 ),
                LIGHT_WORK = VKE_BIT( 3 ),
                MEDIUM_WORK = VKE_BIT( 4 ),
                HEAVY_WORK = VKE_BIT( 5 ),
                MAIN_THREAD = VKE_BIT( 6 ),
                RENDER_THREAD = VKE_BIT( 7 ),
                _MAX_COUNT = 9,
                DEFAULT = LOW_PRIORITY | LIGHT_WORK
            };
        };
        using TASK_FLAGS = uint16_t;
        using TaskFlagBits = Utils::TCBitset< TASK_FLAGS >;


        struct ThreadUsageBits
        {
            enum USAGE : uint32_t
            {
                GENERAL = VKE_BIT( 0 ),
                GRAPHICS = VKE_BIT( 1 ),
                LOGIC = VKE_BIT( 2 ),
                PHYSICS = VKE_BIT( 3 ),
                SOUND = VKE_BIT( 4 ),
                NETWORK = VKE_BIT( 5 ),
                FILE_IO = VKE_BIT( 6 ),
                COMPILE = VKE_BIT(7),
                RESOURCE_PREPARE = VKE_BIT(8),
                CUSTOM_1 = VKE_BIT( 9 ),
                CUSTOM_N = VKE_BIT( 10 + Config::Threads::MAX_CUSTOM_THREAD_COUNT ),
                MAIN_THREAD = VKE_BIT(31),
                ANY_THREAD = VKE_BIT(30),
                ANY_EXCEPT_MAIN = VKE_BIT(29)
            };
        };
        using ThreadUsages = BitsetU32;
        /*struct ThreadUsages
        {
            enum USAGE
            {
                GENERAL,
                GRAPHICS,
                LOGIC,
                PHYSICS,
                SOUND,
                NETWORK,
                FILE_IO,
                COMPILE,
                RESOURCE_PREPARE,
                CUSTOM_1,
                CUSTOM_N = CUSTOM_1 + 10,
                _MAX_COUNT,
                UNKNOWN = _MAX_COUNT
            };
        };
        using THREAD_USAGE = ThreadUsages::USAGE;*/


        template<class SyncObjType>
        class TCTryLock final
        {
            public:
            TCTryLock(SyncObjType& Obj) :
                m_Obj(Obj)
            {
                m_locked = m_Obj.try_lock();
            }
            ~TCTryLock()
            {
                if( m_locked )
                {
                    m_Obj.unlock();
                }
            }
            void operator=(const TCTryLock&) = delete;
            bool IsLocked() const { return m_locked; }
            private:
                SyncObjType& m_Obj;
                bool m_locked;
        };

        using TryLock = TCTryLock< std::mutex >;

        template<class SyncObjType>
        class TCScopedLock final
        {
            public:

            TCScopedLock(SyncObjType& Obj) :
                m_Obj(Obj)
            {
                m_Obj.lock();
            }

            ~TCScopedLock()
            {
                m_Obj.unlock();
            }

            private:

            SyncObjType& m_Obj;
        };

        template<> class TCScopedLock< Platform::Thread::CSpinlock > final
        {
            public:

                TCScopedLock(Platform::Thread::CSpinlock& Obj) :
                    m_Obj(Obj)
                {
                    m_Obj.Lock();
                }

                ~TCScopedLock()
                {
                    m_Obj.Unlock();
                }

            private:

                Platform::Thread::CSpinlock& m_Obj;
        };

        using ScopedLock = TCScopedLock< SyncObject >;

       
        struct SThreadDesc
        {
            Threads::ThreadUsages Usages;
        };
        struct VKE_API SThreadPoolInfo
        {
            // int16_t     threadCount     = 0;
            using ThreadDescArray = Utils::TCDynamicArray<SThreadDesc>;
            ThreadDescArray vThreadDescs;
            uint16_t threadCount = Constants::Threads::COUNT_OPTIMAL;
            uint16_t taskMemSize = 1024;
            uint16_t maxTaskCount = 1024;
        };
        struct VKE_API STaskResult
        {
            friend class CThreadWorker;
            friend class CThreadPool;
            void Wait()
            {
                while( !m_ready )
                {
                    std::this_thread::yield();
                }
            }
            bool IsReady() const
            {
                return m_ready;
            }

          private:
            void NotifyReady()
            {
                m_ready = true;
            }
            bool m_ready = false;
        };
        template<typename _T_> struct TSTaskResult : STaskResult
        {
            const _T_& Get()
            {
                Wait();
                return data;
            }
            _T_ data;
        };
        template<typename _T_> struct TSTaskParam
        {
            friend class CThreadWorker;
            friend class CThreadPool;
            const void* pData;
            const size_t dataSize = sizeof( _T_ );
            void operator=( const TSTaskParam& ) = delete;
        };
        struct VKE_API STaskParams
        {
            STaskParams()
            {
            }
            STaskParams( const void* pIn, size_t inSize, STaskResult* pRes )
                : pInputParam( pIn )
                , inputParamSize( inSize )
                , pResult( pRes )
            {
            }
            template<typename _T_>
            STaskParams( const TSTaskParam<_T_>& In, STaskResult* pRes )
                : STaskParams( In.pData, In.dataSize, pRes )
            {
            }
            const void* pInputParam = nullptr;
            STaskResult* pResult = nullptr;
            size_t inputParamSize = 0;
        };
        template<typename _IN_, typename _OUT_> struct TSTaskInOut
        {
            TSTaskInOut( void* pIn, STaskResult* pOut )
                : pInput( reinterpret_cast<_IN_*>( pIn ) )
                , pOutput( static_cast<TSTaskResult<_OUT_>*>( pOut ) )
            {
            }
            _IN_* pInput;
            TSTaskResult<_OUT_>* pOutput;
        };
        
        struct TaskResults
        {
            enum RESULT : uint8_t
            {
                OK = 0,
                FAIL,
                WAIT,
                NONE,
                _MAX_COUNT
            };
        };
        using TASK_RESULT = TaskResults::RESULT;
        using TaskFunction = std::function<TASK_RESULT( void* )>;
        template<class T> struct TSSimpleTask
        {
            using ThisType = TSSimpleTask<T>;
            TaskFunction Task;
            T* pData = nullptr;
            /// <summary>
            /// If pData is an array this is array element count
            /// </summary>
            uint32_t dataElementCount = 1;
            VKE_DEBUG_TEXT;
        };

        template<typename T>
        class TCTaskReturn
        {
          public:
            template<bool WaitForData = true>
            T& Get()
            {
                if constexpr( WaitForData )
                {
                    Wait();
                }
                return m_data;
            }
            template<bool WaitForData = true>
            Result Get(T* pOut)
            {
                if constexpr( WaitForData )
                {
                    Wait();
                }
                *pOut = m_data;
                return m_result;
            }

            void Wait()
            {
                while( !m_isReady )
                {
                    Platform::ThisThread::Pause();
                }
            }

            void SetReady( T& v, Result v2 )
            {
                m_isReady = true;
                m_data = v;
                m_result = v2;
            }

          protected:
            T m_data;
            Result m_result = VKE_FAIL;
            bool m_isReady = false;
        };

        template<class T>
        using TaskReturnPtr = TCTaskReturn< T >*;
        using TaskQueue = std::deque<Threads::ITask*>;
    } // Threads
} // VKE
