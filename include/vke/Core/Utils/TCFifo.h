#pragma once

#include "TCList.h"

namespace VKE
{
    namespace Utils
    {


        struct FifoDefaultPolicy : public ListDefaultPolicy
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
        class Policy = FifoDefaultPolicy
            >
        class TCFifo : public TCList< T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy >
        {
        public:

            using Base = TCList< T, DEFAULT_ELEMENT_COUNT, AllocatorType, Policy >;
            using DataType = T;
            using DataTypePtr = Base::DataTypePtr;
            using DataTypeRef = Base::DataTypeRef;
            using CountType = Base::CountType;

            using iterator = typename Base::iterator;
            using const_iterator = typename Base::const_iterator;

        protected:

        public:

            TCFifo() : Base() {}
            TCFifo(const TCFifo& Other) : Base(Other) {}
            TCFifo(TCFifo&& Other) : Base(Other) {}
            explicit TCFifo(CountType elemCount) : Base(elemCount) {}
            TCFifo(CountType elemCount, const DataType Default) :
                Base(elemCount, Default) {}
            /*TCFifo(CountType elemCount, VisitCallback Callback) :
                TCFifo(elemCount, Callback) {}*/

        protected:

        };





    } // Utils
} // VKE