#pragma once
#include "Core/Resources/Common.h"
#include "Core/VKEConfig.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Resources/CFile.h"
#include "Core/Memory/CFreeListPool.h"

namespace VKE
{
    namespace Resources
    {
        class CFile;
    }
    
    class CVKEngine;

    namespace Core
    {
        struct SFileManagerDesc
        {
            uint32_t maxFileCount = 0;
        };

        class VKE_API CFileManager
        {
            friend class CVKEngine;

            using Desc = SFileManagerDesc;
            using CFile = Resources::CFile;
            using FileBuffer = Utils::TSFreePool< CFile*, CFile*, Config::Resource::File::DEFAULT_COUNT >;

            public:

                            CFileManager(/*CVKEngine* pEngine*/);
                            CFileManager(const CFileManager&) = delete;
                            CFileManager(CFileManager&&) = delete;
                            ~CFileManager();

                Result      Create(const SFileManagerDesc& Desc);
                void        Destroy();

                FilePtr     Load(const SFileDesc& Desc);
                void        FreeFile(FilePtr* pFileInOut);

            protected:

                FilePtr     _CreateFile(const SFileDesc& Desc);
                Result      _LoadFromFile(FilePtr* pInOut);

            protected:

                CVKEngine*              m_pEngine;
                Desc                    m_Desc;
                FileBuffer              m_FileBuffer;
                Memory::CFreeListPool   m_FileAllocator;
                Threads::SyncObject     m_SyncObj;
        };
    } // Core
} // VKE