#pragma once
#include "Core/Resources/Common.h"
#include "Core/VKEConfig.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Resources/CFile.h"

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

        };

        class CFileManager
        {
            friend class CVKEngine;

            using Desc = SFileManagerDesc;
            using CFile = Resources::CFile;
            using FileBuffer = Utils::TSFreePool< CFile*, CFile*, Config::Resource::File::DEFAULT_COUNT >;

            public:

                            CFileManager(CVKEngine* pEngine);
                            ~CFileManager();

                Result      Create(const SFileManagerDesc& Desc);
                void        Destroy();

                FilePtr     Load(cstr_t pPath);

            protected:

                CVKEngine*      m_pEngine;
                Desc            m_Desc;
                FileBuffer      m_Files;
        };
    } // Core
} // VKE