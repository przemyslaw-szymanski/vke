#pragma once
#include "Core/VKECommon.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Memory/TCFreeListManager.h"

namespace VKE
{
    enum
    {
        RB_ADD_TO_BUFFER = true,
        RB_DO_NOT_ADD_TO_BUFFER = false
    };

    namespace Core
    {

        template
        <
            class ResourceType,
            class FreeResourceType,
            uint32_t BASE_RESOURCE_COUNT
        >
        struct TSResourceBuffer
        {
            using HashType = hash_t;
            using Map = vke_hash_map< HashType, ResourceType >;
            using FreeMap = vke_hash_map< HashType, FreeResourceType >;
            //using Pool = Utils::TCDynamicArray< ResourceType, BASE_RESOURCE_COUNT >;
            using MapIterator = typename Map::iterator;
            using MapConstIterator = typename Map::const_iterator;
            using FreeIterator = typename FreeMap::iterator;
            using FreeConstIterator = typename FreeMap::const_iterator;
            using Pool = Utils::TSFreePool< ResourceType, FreeResourceType, BASE_RESOURCE_COUNT >;
            

            Pool    Buffer;
            Map     mAllocatedHashes;
            FreeMap mFreeHashes;

            bool Add( const ResourceType& Res, const HashType& hash, const MapConstIterator& Itr )
            {
                bool res = false;
                if( Buffer.vPool.PushBack( Res ) != Utils::INVALID_POSITION )
                {
                    mAllocatedHashes.insert( Itr, { hash, Res } );
                    res = true;
                }
                return res;
            }

            template<class MapType, class ItrType>
            bool Find( MapType& mHashes, const HashType& hash, ItrType* pOut )
            {
                auto Itr = mHashes.lower_bound( hash );
                *pOut = Itr;
                bool ret = false;
                if( Itr != mHashes.end() && !( mHashes.key_comp()( hash, Itr->first ) ) )
                {
                    ret = true;
                }
                return ret;
            }

            ResourceType& Find( const HashType& hash )
            {
                auto Itr = mAllocatedHashes.find( hash );
                return Itr->second;
            }

            bool FindAllocated( const HashType& hash, MapIterator* pOut )
            {
                return Find( mAllocatedHashes, hash, pOut );
            }

            bool FindFree( const HashType& hash, FreeIterator* pOut )
            {
                return Find( mFreeHashes, hash, pOut );
            }

            void Free(const HashType& hash)
            {
                MapIterator Itr;
                //VKE_ASSERT( !FindFree( hash, &Itr ), "The same resource can not be freed more than once." );
                if( FindAllocated( hash, &Itr ) )
                {
                    Free( static_cast< FreeResourceType >( Itr->second ) );
                    //mFreeHashes.insert( { hash, static_cast< FreeResourceType >( Itr->second ) } );
                }
            }

            void Free(const FreeResourceType& Res )
            {
                Buffer.vFreeElements.PushBack( Res );
            }

            bool Get( const HashType& hash, ResourceType* pResOut, MapIterator* pItrOut )
            {
                bool ret = false;
                // Find if there was allocated
                if( FindAllocated( hash, pItrOut ) )
                {
                    *pResOut = ( *pItrOut )->second;
                    ret = true;
                }
                else
                {
                    // If there is no such resource created
                    // Try to get last free
                    if( Buffer.vFreeElements.PopBack( pResOut ) )
                    {
                        ret = true;
                    }
                }
                return ret;
            }

            void Clear()
            {
                Buffer.Clear();
                mAllocatedHashes.clear();
            }
        };

        template<
            class Key,
            class Value,
            uint32_t DEFAULT_COUNT
        >
        class TSMultimap
        {
            using Vector = Utils::TCDynamicArray< Value, DEFAULT_COUNT >;
            using Map = vke_hash_map< Key, Vector >;
            Map m_Map;

            public:

                using Iterator = typename Map::iterator;
                using ConstIterator = typename Map::const_iterator;

                Iterator Begin()
                {
                    return m_Map.begin();
                }

                Iterator End()
                {
                    return m_Map.end();
                }

                uint32_t Insert( const Key& key, const Value& value )
                {
                    return m_Map[ key ].PushBack( value );
                }

                Iterator Find( const Key& key )
                {
                    return m_Map.find( key );
                }

                Vector& At( const Key& key )
                {
                    return m_Map[ key ];
                }

                Vector& operator[]( const Key& key )
                {
                    return At( key );
                }

                bool TryPop( Value* pOut )
                {
                    for( auto& Itr = m_Map.begin(); Itr != m_Map.end(); ++Itr )
                    {
                        if( Itr->second.PopBack( pOut ) )
                        {
                            return true;
                        }
                    }
                    return false;
                }

                bool Remove( const Iterator& Itr )
                {
                    m_Map.erase( Itr );
                    return true;
                }
        };

        template<
            class ResourceT,
            class FreeResourceT,
            uint32_t DEFAULT_COUNT = 8
        >
        struct TSMultimapResourcePoolFunctions
        {
            using ResourceType = ResourceT;
            using FreeResourceType = FreeResourceT;
            using Container = TSMultimap< handle_t, ResourceType, DEFAULT_COUNT >;
            using FreeContainer = TSMultimap< handle_t, FreeResourceType, DEFAULT_COUNT >;

            template<class ContainerType, class ResourceType>
            static bool TryToReuse( ContainerType* pContainer, const handle_t& hRes, ResourceType* pOut )
            {
                return Find( pContainer, hRes, pOut );
            }

            template<class ContainerType, class ResourceType>
            static bool Add( ContainerType* pContainer, const handle_t& hRes, const ResourceType& Res )
            {
                return pContainer->Insert( hRes, Res );
            }

            template<class ContainerType, class ResourceType>
            static bool Find( ContainerType* pContainer, const handle_t& hRes, ResourceType* pOut )
            {
                auto Iterator = pContainer->Find( hRes );
                if( Iterator != pContainer->End() )
                {
                    *pOut = Iterator->second.Back();
                    return true;
                }
                return false;
            }

            template<class ContainerType, class ResourceType>
            static bool TryPop( ContainerType* pContainer, ResourceType* pOut )
            {
                return pContainer->TryPop( pOut );
            }

            template<class ContainerType, class ResourceType>
            static bool Remove( ContainerType* pContainer, const handle_t& hRes, ResourceType* pOut )
            {
                auto Iterator = pContainer->Find( hRes );
                if( Iterator != pContainer->End() )
                {
                    *pOut = Iterator->second.Back();
                    return pContainer->Remove( Iterator );
                }
                return false;
            }
        };

        template
        <
            class OpFunctions
        >
        struct TSResourcePool
        {
            using Container = typename OpFunctions::Container;
            using FreeContainer = typename OpFunctions::FreeContainer;
            using ResourceType = typename OpFunctions::ResourceType;
            using FreeResourceType = typename OpFunctions::FreeResourceType;
            
            Container      Resources;
            FreeContainer  FreeResources;

            bool TryToReuse( handle_t hResource, FreeResourceType* pOut )
            {
                return OpFunctions::TryToReuse( &FreeResources, hResource, pOut );
            }

            bool Add( handle_t hResource, const ResourceType& Res )
            {
                return OpFunctions::Add( &Resources, hResource, Res );
            }

            void AddFree( handle_t hResource, const FreeResourceType& Res )
            {
                OpFunctions::Add( &FreeResources, hResource, Res );
            }

            bool Find( handle_t hResource, ResourceType* pOut )
            {
                return OpFunctions::Find( &Resources, hResource, pOut );
            }

            bool FindFree( handle_t hResource, FreeResourceType* pOut )
            {
                return OpFunctions::Find( &FreeResources, hResource, pOut );
            }

            bool GetFree( FreeResourceType* pOut )
            {
                return OpFunctions::TryPop( &FreeResources, pOut );
            }

            bool Remove( handle_t hResource, ResourceType* pOut )
            {
                return OpFunctions::Remove( &Resources, hResource, pOut );
            }
        };

        template
        <
            class ResourceType,
            class FreeResourceType,
            uint32_t DEFAULT_COUNT = 8
        >
        using TSMultimapResourceBuffer = TSResourcePool< TSMultimapResourcePoolFunctions< ResourceType, FreeResourceType, DEFAULT_COUNT > >;

        class VKE_API CResourceManager
        {
            
        };

    } // Core
} // VKE