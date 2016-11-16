#pragma once

#include "Core/VKECommon.h"
#include "Core/Platform/CPlatform.h"

namespace VKE
{
    namespace Threads
    {
        class ITask;
    } // Threads

    struct VKE_API SThreadPoolInfo
    {
        int16_t     threadCount     = 0;
        uint16_t    taskMemSize     = 1024;
        uint16_t    maxTaskCount    = 1024;
    };

    struct VKE_API STaskResult
    {
        friend class CThreadWorker;
        friend class CThreadPool;

        void Wait() { while(!m_ready) { std::this_thread::yield(); } }
        bool IsReady() const { return m_ready; }

        private:
            void    NotifyReady() { m_ready = true; }
            bool    m_ready = false;
    };

    template<typename _T_>
    struct TSTaskResult : STaskResult
    {
        const _T_&  Get() { Wait(); return data; }
        _T_     data;
    };

    template<typename _T_>
    struct TSTaskParam
    {
        friend class CThreadWorker;
        friend class CThreadPool;

        const void*     pData;
        const size_t    dataSize = sizeof(_T_);

        void operator=(const TSTaskParam&) = delete;
    };

    struct VKE_API STaskParams
    {
        STaskParams() {}
        STaskParams(const void* pIn, size_t inSize, STaskResult* pRes) :
            pInputParam(pIn), inputParamSize(inSize), pResult(pRes) {}
        template<typename _T_>
        STaskParams(const TSTaskParam< _T_ >& In, STaskResult* pRes) :
            STaskParams(In.pData, In.dataSize, pRes) {}
        const void*     pInputParam = nullptr;
        STaskResult*    pResult = nullptr;
        size_t          inputParamSize = 0;
    };

    template<typename _IN_, typename _OUT_>
    struct TSTaskInOut
    {
        TSTaskInOut(void* pIn, STaskResult* pOut) : 
            pInput( reinterpret_cast< _IN_* >( pIn ) ), 
            pOutput( static_cast< TSTaskResult< _OUT_ >* >( pOut ) ) {}
        _IN_*                   pInput;
        TSTaskResult< _OUT_ >*  pOutput;
    };

    using TaskFunction = std::function<void(void*, STaskResult*)>;
    using TaskFunction2 = std::function<void(int32_t, void*)>;

    using TaskQueue = std::deque< Threads::ITask* >;
    
    namespace Threads
    {
        using SyncObject = Platform::Thread::CSpinlock;

        using LockGuard = std::lock_guard< std::mutex >;
        using UniqueLock = std::unique_lock< std::mutex >;

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

            Platform::Thread::CSpinlock m_Obj;
        };

        using ScopedLock = TCScopedLock< SyncObject >;
    } // Threads
} // VKE
