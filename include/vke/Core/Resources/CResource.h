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
        struct SDesc;
        class CResource;

        using CreateCallback = std::function< void(const void*, void*) >;

        struct SDesc
        {
            cstr_t          pName = nullptr;
            cstr_t          pFileName = nullptr;
            void*           pUserData = nullptr;
            uint16_t        nameLen = 0;
            uint16_t        fileNameLen = 0;
        };

        struct SCreateDesc
        {
            CreateCallback  pfnCallback = nullptr;   
            uint16_t        stages = StageBits::FULL_LOAD;
            bool            async = false;
        };

        class VKE_API CResource : public Core::CObject
        {
            public:

                CResource(){}
                ~CResource() {}

            protected:

                STATE       m_resourceState = States::UNKNOWN;
        };
    } // Resources

    using ResourceRefPtr = Utils::TCObjectSmartPtr< Resources::CResource >;
    using ResourcePtr = Utils::TCWeakPtr< Resources::CResource >;
    using ResourceStates = Resources::States;
    using ResourceStageBits = Resources::StageBits;
    using SResourceCreateDesc = Resources::SCreateDesc;
    using SResourceDesc = Resources::SDesc;
} // VKE