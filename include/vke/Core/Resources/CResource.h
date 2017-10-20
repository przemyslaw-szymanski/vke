#pragma once

#include "Core/CObject.h"
#include "Core/Utils/TCSmartPtr.h"

namespace VKE
{
    namespace Resources
    {
        class CManager;

        struct States
        {
            enum STATE
            {
                UNKNOWN,
                CREATED,
                LOADED,
                PREPARED,
                UNLOADED,
                INVALIDATED,
                _MAX_COUNT
            };
        };
        using STATE = States::STATE;

        struct StageBits
        {
            enum
            {
                UNKNOWN     = 0x00000000,
                CREATE      = VKE_BIT(1),
                LOAD        = VKE_BIT(2),
                PREPARE     = VKE_BIT(3),
                UNLOAD      = VKE_BIT(4),
                INVALID     = VKE_BIT(5),
                FULL_LOAD   = CREATE | LOAD | PREPARE | UNLOAD,
            };
        };

        struct SCreateDesc;
        class CResource;

        using CreateCallback = std::function< void(const SCreateDesc*, CResource*) >;

        struct SCreateDesc
        {
            cstr_t          pName = nullptr;
            cstr_t          pFileName = nullptr;
            CreateCallback  pfnCallback = nullptr;
            void*           pUserData = nullptr;
            void*           pTypeDesc = nullptr;
            uint16_t        nameLen = 0;
            uint16_t        fileNameLen = 0;
            uint16_t        stages = StageBits::FULL_LOAD;
            bool            runAsync = false;
        };

        class VKE_API CResource : public Core::CObject
        {
            friend class CManager;
            public:

                CResource();
                explicit CResource(CManager*);
                CResource(CManager*, handle_t);
                virtual ~CResource();

                CManager&   GetManager() const { return *m_pMgr; }
                
                handle_t    GetHandle() const { return m_handle; }

                void Invalidate();

                void Update();

                vke_force_inline
                bool IsValid() const { return m_isValid; }

            protected:

                CManager*   m_pMgr;
                handle_t    m_handle;
                uint32_t    m_isValid : 1;
        };
    } // Resources

    using ResourceRefPtr = Utils::TCObjectSmartPtr< Resources::CResource >;
    using ResourcePtr = Utils::TCWeakPtr< Resources::CResource >;
    using ResourceStates = Resources::States;
    using ResourceStageBits = Resources::StageBits;
} // VKE