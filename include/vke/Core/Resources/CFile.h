#pragma once
#include "Common.h"
#include "Core/CObject.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Resources/CResource.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    struct SFileDesc
    {
        SResourceDesc   Base;
    };

    struct SFileInitInfo
    {
        using DataType = uint8_t;
        using DataBuffer = Utils::TCDynamicArray< DataType, 1 >;

        handle_t    hFile = 0;
        uint8_t*    pData = nullptr;
        uint32_t    dataSize = 0;
        DataBuffer  Buffer;
    };

    namespace Core
    {
        class CFileManager;
    };

    namespace Resources
    {
        class CFile : public CResource
        {
            friend class Core::CFileManager;

            protected:

                

            public:

                Result      Init(const SFileInitInfo& Info);
                void        Release();

            protected:

                SFileDesc       m_Desc;
                SFileInitInfo   m_InitInfo;
        };
    } // Resources
    using FilePtr = Utils::TCWeakPtr< Resources::CFile >;
    using FileSmartPtr = Utils::TCObjectSmartPtr< Resources::CFile >;
} // VKE