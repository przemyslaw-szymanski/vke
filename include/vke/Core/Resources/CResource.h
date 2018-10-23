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
                UNKNOWN         = 0,
                CREATED         = VKE_BIT( 1 ),
                INITIALIZED     = VKE_BIT( 2 ),
                LOADED          = VKE_BIT( 3 ),
                PREPARED        = VKE_BIT( 4 ),
                UNLOADED        = VKE_BIT( 5 ),
                INVALIDATED     = VKE_BIT( 6 ),
                _MAX_COUNT = 7
            };
        };
        using STATE = States::STATE;

        struct StageBits
        {
            enum
            {
                UNKNOWN     = 0x00000000,
                CREATE      = VKE_BIT(1),
                INIT        = VKE_BIT(2),
                LOAD        = VKE_BIT(3),
                PREPARE     = VKE_BIT(4),
                UNLOAD      = VKE_BIT(5),
                INVALID     = VKE_BIT(6),
                FULL_LOAD   = CREATE | INIT | LOAD | PREPARE,
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

            SDesc() {}

            SDesc(const SDesc& Other)
            {
                this->operator=( Other );
            }

            SDesc& operator=(const SDesc& Other)
            {
                pName = Other.pName;
                pFileName = Other.pFileName;
                pUserData = Other.pUserData;
                nameLen = Other.nameLen;
                fileNameLen = Other.fileNameLen;
                return *this;
            }
        };

        struct SCreateDesc
        {
            CreateCallback  pfnCallback = nullptr;   
            uint16_t        stages = StageBits::FULL_LOAD;
            bool            async = false;

            SCreateDesc() {}

            SCreateDesc(const SCreateDesc& Other)
            {
                this->operator=( Other );
            }

            SCreateDesc& operator=(const SCreateDesc& Other)
            {
                pfnCallback = Other.pfnCallback;
                stages = Other.stages;
                async = Other.async;
                return *this;
            }
        };

        class VKE_API CResource : virtual public Core::CObject
        {
            using SyncObject = Threads::SyncObject;
            public:

                vke_force_inline CResource() {}
                vke_force_inline CResource(uint32_t baseRefCount) : Core::CObject(baseRefCount) {}
                vke_force_inline virtual ~CResource() {}

                uint32_t        GetResourceState() const { return m_resourceState; }

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

                uint32_t    m_resourceState = 0;
        };
    } // Resources

    using ResourceRefPtr = Utils::TCObjectSmartPtr< Resources::CResource >;
    using ResourcePtr = Utils::TCWeakPtr< Resources::CResource >;
    using ResourceStates = Resources::States;
    using ResourceStageBits = Resources::StageBits;
    using SResourceCreateDesc = Resources::SCreateDesc;
    using SResourceDesc = Resources::SDesc;
} // VKE