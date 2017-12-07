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

        class VKE_API CResource : virtual public Core::CObject
        {
            public:

                vke_force_inline CResource() {}
                vke_force_inline CResource(uint32_t baseRefCount) : Core::CObject(baseRefCount) {}
                vke_force_inline virtual ~CResource() {}

                const hash_t&   GetResourceHash() const { return m_resourceHash; }
                const STATE&    GetResourceState() const { return m_resourceState; }

                template<typename T>
                static hash_t   CalcHash(const T& base)
                {
                    return std::hash< T >{}( base );
                }

                static hash_t   CalcHash(const SDesc& Desc)
                {
                    return CalcHash( Desc.pFileName ) ^ ( CalcHash( Desc.pName ) << 1 );
                }

            protected:

                STATE       m_resourceState = States::UNKNOWN;
                hash_t      m_resourceHash = 0;
        };
    } // Resources

    using ResourceRefPtr = Utils::TCObjectSmartPtr< Resources::CResource >;
    using ResourcePtr = Utils::TCWeakPtr< Resources::CResource >;
    using ResourceStates = Resources::States;
    using ResourceStageBits = Resources::StageBits;
    using SResourceCreateDesc = Resources::SCreateDesc;
    using SResourceDesc = Resources::SDesc;
} // VKE