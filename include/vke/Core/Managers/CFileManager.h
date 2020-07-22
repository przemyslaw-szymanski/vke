#pragma once
#include "Core/Resources/Common.h"
#include "Core/VKEConfig.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Resources/CFile.h"
#include "Core/Memory/CFreeListPool.h"
#include "Core/Managers/CResourceManager.h"

namespace VKE
{
    namespace Core
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
            using CFile = Core::CFile;
            //using FileBuffer = Utils::TSFreePool< CFile*, CFile*, Config::Resource::File::DEFAULT_COUNT >;
            //using FileBuffer = Core::TSResourceBuffer< FileRefPtr, CFile*, Config::Resource::File::DEFAULT_COUNT >;
            using FileBuffer = Core::TSUniqueResourceBuffer< FileRefPtr, hash_t, Config::Resource::File::DEFAULT_COUNT >;

            friend class CFile;

            public:

                            CFileManager(/*CVKEngine* pEngine*/);
                            CFileManager(const CFileManager&) = delete;
                            CFileManager(CFileManager&&) = delete;
                            ~CFileManager();

                Result      Create(const SFileManagerDesc& Desc);
                void        Destroy();

                FilePtr     LoadFile(const SLoadFileInfo& Desc);

            protected:

                void        _FreeFile(CFile* pFileInOut);
                FilePtr     _CreateFile(const SLoadFileInfo& Desc);
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