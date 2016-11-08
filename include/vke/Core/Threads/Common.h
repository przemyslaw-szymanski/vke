#pragma once

#include "Core/VKECommon.h"

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

    using CriticalSection = std::mutex;

} // VKE
