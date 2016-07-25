#include "CFreeList.h"
#include "Core/Utils/CLogger.h"

namespace VKE
{
    namespace Memory
    {
        CFreeList::CFreeList(CFreeList&& Other)
        {
            m_pMemoryPool = Other.m_pMemoryPool;
            m_elementCount = Other.m_elementCount;
            m_elementSize = Other.m_elementSize;
            m_freeBlockCount = Other.m_freeBlockCount;
            m_isMemoryPoolCreated = Other.m_isMemoryPoolCreated;
            m_MemoryRange = Other.m_MemoryRange;
            m_memorySize = Other.m_memorySize;
            m_pFirstFreeBlock = Other.m_pFirstFreeBlock;

            Other.m_pMemoryPool = nullptr;
        }

        void CFreeList::operator=(CFreeList&& Other)
        {
            m_pMemoryPool = Other.m_pMemoryPool;
            m_elementCount = Other.m_elementCount;
            m_elementSize = Other.m_elementSize;
            m_freeBlockCount = Other.m_freeBlockCount;
            m_isMemoryPoolCreated = Other.m_isMemoryPoolCreated;
            m_MemoryRange = Other.m_MemoryRange;
            m_memorySize = Other.m_memorySize;
            m_pFirstFreeBlock = Other.m_pFirstFreeBlock;

            Other.m_pMemoryPool = nullptr;
        }

        void CFreeList::Destroy()
        {
            VKE_DELETE_ARRAY(m_pMemoryPool);
            m_elementCount = 0;
            m_elementSize = 0;
            m_freeBlockCount = 0;
            m_isMemoryPoolCreated = 0;
            m_memorySize = 0;
            m_pFirstFreeBlock = nullptr;
        }

        Result CFreeList::Create(uint32_t elementCount, size_t elementSize)
        {
            m_elementCount = elementCount;
            m_elementSize = Max(elementSize, sizeof(SFreeBlock));
            m_memorySize = m_elementCount * m_elementSize;
            m_pMemoryPool = VKE_NEW DataType[m_memorySize];
            if(m_pMemoryPool == nullptr)
            {
                m_isMemoryPoolCreated = false;
                VKE_LOG_ERR_RET(VKE_ENOMEMORY, "Not enough memory in free list pool");
            }

            //Initialize all block with free blocks
            MemoryPool pMem = m_pMemoryPool;
            SFreeBlock* pLastBlock = nullptr, *pCurrBlock = nullptr;

            for(uint32_t i = 0; i < m_elementCount; ++i)
            {
                pCurrBlock = reinterpret_cast<SFreeBlock*>(pMem);
                pCurrBlock->pNext = pLastBlock;
                pLastBlock = pCurrBlock;
                pMem += m_elementSize;
            }

            m_pFirstFreeBlock = pLastBlock;
            m_freeBlockCount = m_elementCount;
            m_isMemoryPoolCreated = true;
            m_MemoryRange.begin = m_pMemoryPool;
            m_MemoryRange.end = m_pMemoryPool + m_memorySize;
            return VKE_OK;
        }
    } // Memory
} // VKE