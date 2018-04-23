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

    namespace Core
    {
        class CFileManager;
    };

    namespace Resources
    {
        class VKE_API CFile final : public CResource
        {
            friend class Core::CFileManager;
            using CFileManager = Core::CFileManager;

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

                static hash_t   CalcHash(const SFileDesc& Desc);

                Result          Init(const SFileDesc& Info);
                void            Release();

                const DataType* GetData() const;
                uint32_t        GetDataSize() const;

                template<typename T>
                const T*        GetData() const { return reinterpret_cast< const T* >( GetData() ); }

                cstr_t          GetExtension() const { return m_pFileExtension; }

            protected:

                SFileDesc       m_Desc;
                CFileManager*   m_pMgr;
                cstr_t          m_pFileExtension = nullptr;
                SData           m_Data;
        };
    } // Resources
    using FilePtr = Utils::TCWeakPtr< Resources::CFile >;
    using FileRefPtr = Utils::TCObjectSmartPtr< Resources::CFile >;
} // VKE