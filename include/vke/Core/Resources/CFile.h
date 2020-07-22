#pragma once
#include "Common.h"
#include "Core/CObject.h"
#include "Core/Utils/TCSmartPtr.h"
#include "Core/Resources/CResource.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace Core
    {
        class CFileManager;

        class VKE_API CFile
        {
            friend class Core::CFileManager;
            using CFileManager = Core::CFileManager;

            VKE_DECL_BASE_OBJECT( handle_t );
            VKE_DECL_BASE_RESOURCE();

            public:

                using DataType = uint8_t;

            protected:

                struct SData
                {
                    using DataBuffer = Utils::TCDynamicArray< DataType, 1 >;
                    DataBuffer  vBuffer;
                    DataType*   pData = nullptr;
                    uint32_t    dataSize = 0;
                };

            public:

                                CFile(CFileManager* pMgr);
                                ~CFile();

                void            operator delete(void*);

                static hash_t   CalcHash(const SFileInfo& Desc);

                Result          Init(const SFileInfo& Info);
                void            Release();

                const DataType* GetData() const;
                uint32_t        GetDataSize() const;

                template<typename T>
                const T*        GetData() const { return reinterpret_cast< const T* >( GetData() ); }

                cstr_t          GetExtension() const { return m_pFileExtension; }

            protected:

                SFileInfo       m_Desc;
                CFileManager*   m_pMgr;
                cstr_t          m_pFileExtension = nullptr;
                SData           m_Data;
        };

        using FilePtr = Utils::TCWeakPtr< CFile >;
        using FileRefPtr = Utils::TCObjectSmartPtr< CFile >;
    } // Core
} // VKE