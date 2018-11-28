#pragma once
#include "Core/Resources/Common.h"
#include "Core/VKEConfig.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Resources/CFile.h"
#include "Core/Memory/CFreeListPool.h"
#include "Core/Managers/CResourceManager.h"

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

        struct SFileCreateDesc
        {
            SCreateResourceDesc Create;
            SFileDesc           File;
        };

        class VKE_API CFileManager
        {
            friend class CVKEngine;

            using Desc = SFileManagerDesc;
            using CFile = Resources::CFile;
            //using FileBuffer = Utils::TSFreePool< CFile*, CFile*, Config::Resource::File::DEFAULT_COUNT >;
            using FileBuffer = Core::TSResourceBuffer< CFile*, CFile*, Config::Resource::File::DEFAULT_COUNT >;

            friend class CFile;

            public:

                            CFileManager(/*CVKEngine* pEngine*/);
                            CFileManager(const CFileManager&) = delete;
                            CFileManager(CFileManager&&) = delete;
                            ~CFileManager();

                Result      Create(const SFileManagerDesc& Desc);
                void        Destroy();

                FilePtr     LoadFile(const SFileCreateDesc& Desc);

            protected:

                void        _FreeFile(CFile* pFileInOut);
                FilePtr     _CreateFile(const SFileCreateDesc& Desc);
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