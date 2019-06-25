#pragma once

#include "Core/CObject.h"
#include "Core/Utils/TCSmartPtr.h"

namespace VKE
{
    namespace Core
    {
        class CManager;

        struct ResourceStates
        {
            enum STATE : uint16_t
            {
                UNKNOWN     = 0x0,
                CREATED     = VKE_BIT( 1 ),
                INITIALIZED = VKE_BIT( 2 ),
                LOADED      = VKE_BIT( 3 ),
                PREPARED    = VKE_BIT( 4 ),
                UNLOADED    = VKE_BIT( 5 ),
                INVALIDATED = VKE_BIT( 6 ),
                INVALID     = VKE_BIT( 7 ),
                _MAX_COUNT  = 8
            };
        };
        using RESOURCES_STATES = uint16_t;

        struct ResourceStages
        {
            enum STAGE
            {
                UNKNOWN     = 0x0,
                CREATE      = VKE_BIT( 1 ),
                INIT        = VKE_BIT( 2 ),
                LOAD        = VKE_BIT( 3 ),
                PREPARE     = VKE_BIT( 4 ),
                UNLOAD      = VKE_BIT( 5 ),
                INVALID     = VKE_BIT( 6 ),
                _MAX_COUNT  = 7,
                FULL_LOAD   = CREATE | INIT | LOAD | PREPARE,
            };
        };
        using RESOURCE_STAGES = uint8_t;

        struct SCreateDesc;
        struct SDesc;
        class CResource;

        using CreateCallback = std::function< void( const void*, void* ) >;

        struct SDesc
        {
            cstr_t          pName = nullptr;
            cstr_t          pFileName = nullptr;
            void*           pUserData = nullptr;
            uint16_t        nameLen = 0;
            uint16_t        fileNameLen = 0;

            /*SDesc()
            {}

            SDesc( const SDesc& Other )
            {
                this->operator=( Other );
            }

            SDesc& operator=( const SDesc& Other )
            {
                pName = Other.pName;
                pFileName = Other.pFileName;
                pUserData = Other.pUserData;
                nameLen = Other.nameLen;
                fileNameLen = Other.fileNameLen;
                return *this;
            }*/
        };

        struct STaskResult
        {
            Result  result = VKE_ENOTREADY;
            void*   pData = nullptr;

            void Wait()
            {
                while( result == VKE_ENOTREADY )
                {
                    Platform::ThisThread::Pause();
                }
            }

            template<class T>
            Result Get( T* pOut )
            {
                Wait();
                *pOut = Cast< T >( pData );
                return result;
            }

            template<class T>
            static T Cast( void* pData )
            {
                return *reinterpret_cast< T* >( pData );
            }
        };

        struct SCreateDesc
        {
            CreateCallback  pfnCallback = nullptr;
            STaskResult*    pResult = nullptr;
            void*           pOutput = nullptr;
            RESOURCE_STAGES stages = ResourceStages::CREATE | ResourceStages::INIT | ResourceStages::PREPARE;
            bool            async = true;

            SCreateDesc()
            {}

            SCreateDesc( const SCreateDesc& Other )
            {
                this->operator=( Other );
            }

            /*SCreateDesc& operator=(const SCreateDesc& Other)
            {
                pfnCallback = Other.pfnCallback;
                stages = Other.stages;
                async = Other.async;
                pResult = Other.pResult;
                return *this;
            }*/
        };

        struct SBindMemoryInfo
        {
            handle_t    hMemory;
            uint32_t    offset;
        };

        class VKE_API CResource : virtual public Core::CObject
        {
            using SyncObject = Threads::SyncObject;
            public:

                vke_force_inline CResource()
                {}
                vke_force_inline CResource( uint32_t baseRefCount ) : Core::CObject( baseRefCount )
                {}
                vke_force_inline virtual ~CResource()
                {}

                uint32_t        GetResourceState() const
                {
                    return m_resourceState;
                }

                template<typename T>
                static hash_t   CalcHash( const T& base )
                {
                    return std::hash< T >{}( base );
                }

                static hash_t   CalcHash( const SDesc& Desc )
                {
                    return CalcHash( Desc.pFileName ) ^ ( CalcHash( Desc.pName ) << 1 );
                }

                bool vke_force_inline IsReady() const { return m_resourceState & ResourceStates::PREPARED; }
                bool vke_force_inline IsInvalid() const { return m_resourceState & ResourceStates::INVALID; }
                bool vke_force_inline IsLoaded() const { return m_resourceState & ResourceStates::LOADED; }
                bool vke_force_inline IsUnLoaded() const { return m_resourceState & ResourceStates::UNLOADED; }
                bool vke_force_inline IsCreated() const { return m_resourceState & ResourceStates::CREATED; }

            protected:

                RESOURCE_STAGES    m_resourceState = 0;
        };

        using ResourceRefPtr = Utils::TCObjectSmartPtr< CResource >;
        using ResourcePtr = Utils::TCWeakPtr< CResource >;
        using ResourceStates = ResourceStates;
        using ResourceStages = ResourceStages;
        using SCreateResourceDesc = SCreateDesc;
        using SResourceDesc = SDesc;
    } // Core
} // VKE