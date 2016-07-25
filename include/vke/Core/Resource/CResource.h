#pragma once

#include "Core/CObject.h"
#include "Core/Utils/TCSmartPtr.h"

namespace VKE
{
    namespace Resource
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
                CREATE      = VKE_SET_BIT(1),
                LOAD        = VKE_SET_BIT(2),
                PREPARE     = VKE_SET_BIT(3),
                UNLOAD      = VKE_SET_BIT(4),
                INVALID     = VKE_SET_BIT(5),
                FULL_LOAD   = CREATE | LOAD | PREPARE | UNLOAD,
            };
        };

        struct SCreateInfo;
        class CResource;

        using CreateCallback = std::function< void(const SCreateInfo*, CResource*) >;

        struct SCreateInfo
        {
            cstr_t          pName = nullptr;
            cstr_t          pFileName = nullptr;
            CreateCallback  pfnCallback = nullptr;
            uint16_t        nameLen = 0;
            uint16_t        fileNameLen = 0;
            uint16_t        stages = StageBits::FULL_LOAD;
            bool            runAsync = false;
        };

        class VKE_API CResource : public Core::CObject
        {
            public:

            CResource(CManager*) {}
            virtual ~CResource() {}
        };
    } // Resource

    using ResourceRefPtr = Utils::TCObjectSmartPtr< Resource::CResource >;
    using ResourcePtr = Utils::TCWeakPtr< Resource::CResource >;
    using ResourceStates = Resource::States;
    using ResourceStageBits = Resource::StageBits;
} // VKE