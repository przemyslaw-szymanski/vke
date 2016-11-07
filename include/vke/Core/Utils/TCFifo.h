#pragma once

#include "TCList.h"

namespace VKE
{
    namespace Utils
    {


        struct DefaultFifoQueuePolicy
        {

        };

#define TC_FIFO_QUEUE_TEMPLATE \
        typename DataType, uint32_t DEFAULT_ELEMENT_COUNT, class AllocatorType, class Policy

#define TC_FIFO_QUEUE_TEMPLATE_PARAMS \
        DataType, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy

        template
            <
            typename T,
            uint32_t DEFAULT_ELEMENT_COUNT = 32,
        class AllocatorType = Memory::CHeapAllocator,
        class Policy = DefaultFifoQueuePolicy
            >
        class TCFifo : public TCList< T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy >
        {
        public:

            using Base = TCList< T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy >;
            using DataType = T;
            using DataTypePtr = Base::DataTypePtr;
            using DataTypeRef = Base::DataTypeRef;

            using iterator = Base::iterator;
            using const_iterator = Base::const_iterator;

        protected:

        public:

            TCFifoQueue() : TCList() {}
            TCFifoQueue(const TCFifoQueue& Other) : TCList(Other) {}
            TCFifoQueue(TCFifoQueue&& Other) : TCList(Other) {}
            explicit TCFifoQueue(CountType elemCount) : TCList(elemCount) {}
            TCFifoQueue(CountType elemCount, const DataType Default) :
                TCList(elemCount, Default) {}
            TCFifoQueue(CountType elemCount, VisitCallback Callback) :
                TCFifoQueue(elemCount, Callback) {}

        protected:

        };





    } // Utils
} // VKE