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

        struct SFileInfo
        {
            cstr_t          pName = "Unknown";
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

        struct SCreateResourceInfo
        {
            CreateCallback  pfnCallback = nullptr;
            STaskResult*    pResult = nullptr;
            void*           pOutput = nullptr;
            RESOURCE_STAGES stages = ResourceStages::CREATE | ResourceStages::INIT | ResourceStages::PREPARE;
            bool            async = true;
        };

        struct SLoadFileInfo
        {
            SFileInfo           FileInfo;
            SCreateResourceInfo CreateInfo;
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

        struct SBindMemoryInfo
        {
            handle_t    hMemory;
            uint32_t    offset;
        };

#define VKE_DECL_BASE_RESOURCE() \
    public: vke_force_inline ::VKE::Core::RESOURCES_STATES GetResourceState() const { return m_resourceStates; } \
    protected: vke_force_inline void _AddResourceState(const VKE::Core::RESOURCES_STATES& state) { m_resourceStates |= state; } \
    protected: vke_force_inline void _SetResourceState(const VKE::Core::RESOURCES_STATES& state) { m_resourceStates = state; } \
    public: vke_force_inline bool IsStateSet(const ::VKE::Core::RESOURCES_STATES& state) const { return m_resourceStates & state; } \
    public: vke_force_inline bool IsReady() const { return IsStateSet( ::VKE::Core::ResourceStates::PREPARED ); } \
    public: vke_force_inline bool IsInvalid() const { return IsStateSet( ::VKE::Core::ResourceStates::INVALID ); } \
    public: vke_force_inline bool IsLoaded() const { return IsStateSet( ::VKE::Core::ResourceStates::LOADED ); } \
    public: vke_force_inline bool IsUnloaded() const { return IsStateSet( ::VKE::Core::ResourceStates::UNLOADED ); } \
    public: vke_force_inline bool IsCreated() const { return IsStateSet( ::VKE::Core::ResourceStates::CREATED ); } \
    protected: ::VKE::Core::RESOURCES_STATES m_resourceStates = 0

        class VKE_API CResource
        {
            using SyncObject = Threads::SyncObject;
            public:

                vke_force_inline CResource()
                {}
                vke_force_inline virtual ~CResource()
                {}

                uint32_t        GetResourceState() const
                {
                    return m_resourceState;
                }

                static hash_t   CalcHash(cstr_t pString)
                {
                    return std::hash< cstr_t >{}(pString);
                }

                template<typename T>
                static hash_t   CalcHash( const T& base )
                {
                    return std::hash< T >{}( base );
                }

                static hash_t   CalcHash( const SFileInfo& Desc )
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

    } // Core
} // VKE