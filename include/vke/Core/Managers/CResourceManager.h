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

        template
        <
            class Key,
            class Value,
            uint32_t DEFAULT_COUNT
        >
        struct TSMultimap
        {
            using Vector = Utils::TCDynamicArray< Value, DEFAULT_COUNT >;
            using ContainerType = vke_hash_map< Key, Vector >;
            using IterateCallbackType = std::function<bool(const Key&, Vector*)>;

            ContainerType Container;

            using Iterator = typename ContainerType::iterator;
            using ConstIterator = typename ContainerType::const_iterator;

            Iterator Begin()
            {
                return Container.begin();
            }

            Iterator End()
            {
                return Container.end();
            }

            uint32_t Insert( const Key& key, const Value& value )
            {
                return Container[key].PushBack( value );
            }

            Iterator Find( const Key& key )
            {
                return Container.find( key );
            }

            Vector& At( const Key& key )
            {
                return Container[key];
            }

            Vector& operator[]( const Key& key )
            {
                return At( key );
            }

            bool TryPop( Value* pOut )
            {
                for( auto& Itr = Container.begin(); Itr != Container.end(); ++Itr )
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
                Container.erase( Itr );
                return true;
            }

            void Clear()
            {
                Container.clear();
            }

            void Iterate( IterateCallbackType&& Callback )
            {
                for( auto& Itr : Container )
                {
                    bool res = Callback( Itr.first, &Itr->second );
                    if( !res )
                    {
                        break;
                    }
                }
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
            using ContainerType = TSMultimap< handle_t, ResourceType, DEFAULT_COUNT >;
            using FreeContainerType = TSMultimap< handle_t, FreeResourceType, DEFAULT_COUNT >;
            using ResourceIterateCallbackType = typename ContainerType::IterateCallbackType;
            using FreeContainerIterateCallbackType = typename FreeContainerType::IterateCallbackType;

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

            template<class ContainerType>
            static void Clear( ContainerType* pContainer )
            {
                pContainer->Clear();
            }

            template<class ContainerType, class CallbackType>
            static void Clear( ContainerType* pContainer, CallbackType&& Callback )
            {
                pContainer->Iterate( Callback );
            }
        };

        template
        <
            class Key,
            class Value
        >
        struct TSHashMap
        {
            using ContainerType = vke_hash_map< Key, Value >;
            using Iterator = typename ContainerType::iterator;
            using ConstIterator = typename ContainerType::const_iterator;
            using IterateCallbackType = std::function<bool(const Key&, Value*)>;

            ContainerType Container;

            Iterator Find(const Key& key, Value* pOut )
            {
                Iterator Itr = Container.find( key );
                if( Itr != Container.end() )
                {
                    *pOut = Itr->second;
                }
                return Itr;
            }

            bool Insert( const Key& key, const Value& value )
            {
                return Container.insert( ContainerType::value_type( key, value ) ).second;
            }

            bool TryPop( Value* pOut )
            {
                Iterator Itr = Container.begin();
                if( Itr != Container.end() )
                {
                    *pOut = Itr->second;
                    Container.erase( Itr );
                    return true;
                }
                return false;
            }

            Iterator End()
            {
                return Container.end();
            }

            void Clear()
            {
                Container.clear();
            }

            bool Remove( const Key& key, Value* pOut )
            {
                Iterator Itr = Find( key, pOut );
                return Itr != End();
            }

            void Iterate( IterateCallbackType&& Callback )
            {
                for( auto& Itr : Container )
                {
                    bool res = Callback( Itr->first, &Itr->second );
                    if( !res )
                    {
                        break;
                    }
                }
            }
        };


        template
        <
            class ResourceT,
            class FreeResourceT
        >
        struct TSMapResourcePoolFunctions
        {
            using ResourceType = ResourceT;
            using FreeResourceType = FreeResourceT;
            using ContainerType = typename TSHashMap< handle_t, ResourceType >;
            using FreeContainerType = typename TSHashMap< handle_t, FreeResourceType >;
            using ResourceIterateCallbackType = typename ContainerType::IterateCallbackType;
            using FreeResourceIterateCallbackType = typename FreeContainerType::IterateCallbackType;

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
                    *pOut = Iterator->second;
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
                    *pOut = Iterator->second;
                    return pContainer->Remove( Iterator );
                }
                return false;
            }

            template<class ContainerType>
            static void Clear(ContainerType* pContainer)
            {
                pContainer->Clear();
            }

            template<class ContainerType, class CallbackType>
            static void Iterate( ContainerType* pContainer, CallbackType&& Callback )
            {
                pContainer->Iterate( Callback );
            }
        };

        template
        <
            class Value,
            uint32_t DEFAULT_ELEMENT_COUNT
        >
        struct TSVector
        {
            using ContainerType = Utils::TCDynamicArray< Value, DEFAULT_ELEMENT_COUNT >;
            using Iterator = typename ContainerType::Iterator;
            using ConstIterator = typename ContainerType::ConstIterator;
            using IterateCallbackType = std::function<bool( const uint32_t&, Value* )>;

            ContainerType Container;

            Iterator Find( const Value& v, Value* pOut )
            {
                return Container.Find( v );
            }

            bool Insert( const handle_t&, const Value& value )
            {
                return Container.PushBack( value ) != Utils::INVALID_POSITION;
            }

            bool TryPop( Value* pOut )
            {
                return Container.PopBack( pOut );
            }

            Iterator End()
            {
                return Container.End();
            }

            void Clear()
            {
                Container.Clear();
            }

            void Iterate(IterateCallbackType&& Callback)
            {
                for( uint32_t i = 0; i < Container.GetCount(); ++i )
                {
                    bool res = Callback( i, &Container[i] );
                    if( res == false )
                    {
                        break;
                    }
                }
            }
        };


        template
        <
            class OpFunctions
        >
        struct TSResourcePool
        {
            using ContainerType = typename OpFunctions::ContainerType;
            using FreeContainerType = typename OpFunctions::FreeContainerType;
            using ResourceType = typename OpFunctions::ResourceType;
            using FreeResourceType = typename OpFunctions::FreeResourceType;
            using ResourceIterateCallback = typename ContainerType::IterateCallbackType;
            using FreeResourceIterateCallback = typename FreeContainerType::IterateCallbackType;
            
            ContainerType      Resources;
            FreeContainerType  FreeResources;

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

            void Clear()
            {
                OpFunctions::Clear( &Resources );
                OpFunctions::Clear( &FreeResources );
            }

            void ResourceIterate( ResourceIterateCallback&& Callback )
            {
                OpFunctions::Iterate( &Resources, Callback );
            }

            void FreeResourceIteraete( FreeResourceIterateCallback&& Callback )
            {
                OpFunctions::Iterate( &FreeResources, Callback );
            }
        };

        template
        <
            class ResourceType,
            class FreeResourceType,
            uint32_t DEFAULT_COUNT = 8
        >
        using TSMultimapResourceBuffer = TSResourcePool< TSMultimapResourcePoolFunctions< ResourceType, FreeResourceType, DEFAULT_COUNT > >;

        template
        <
            class ResourceType,
            class FreeResourceType
        >
        using TSMapResourceBuffer = TSResourcePool< TSMapResourcePoolFunctions< ResourceType, FreeResourceType > >;

        template
        <
            class ResourceT,
            class FreeResourceT,
            uint32_t DEFAULT_COUNT = 1024
        >
        struct TSVectorResourceBuffer
        {
            using ResourceType = ResourceT;
            using FreeResourceType = FreeResourceT;
            using FreeResourceBufferType = Utils::TCDynamicArray< FreeResourceType, DEFAULT_COUNT >;
            using ResourceBufferType = TSHashMap< handle_t, ResourceType >;

            ResourceBufferType      Resources;
            FreeResourceBufferType  FreeResources;

            // If hResource == NULL_HANDLE it means get any
            // If hResource != NULL_HANDLE try to find matching resource
            bool TryToReuse( handle_t hResource, FreeResourceType* pOut )
            {
                bool ret = false;
                if( hResource != NULL_HANDLE )
                {
                    ret = FindFree( hResource, pOut );
                }
                else
                {
                    ret = FreeResources.PopBack( pOut );
                }
                return ret;
            }

            bool Add( handle_t hResource, const ResourceType& Res )
            {
                return OpFunctions::Add( &Resources, hResource, Res );
            }

            void AddFree( handle_t hResource, const FreeResourceType& Res )
            {
                //OpFunctions::Add( &FreeResources, hResource, Res );
                FreeResources.PushBack( Res );
            }

            bool Find( handle_t hResource, ResourceType* pOut )
            {
                //return OpFunctions::Find( &Resources, hResource, pOut );
                return Resources.Find( hResource, pOut ) != Resources.End();
            }

            bool FindFree( handle_t hResource, FreeResourceType* pOut )
            {
                bool ret = false;
                for( uint32_t i = 0; i < FreeResources.GetCount(); ++i )
                {
                    if( FreeResources[i]->GetHandle() == hResource )
                    {
                        *pOut = FreeResources[i];
                        ret = true;
                        break;
                    }
                }
                return ret;
            }

            bool GetFree( FreeResourceType* pOut )
            {
                return FreeResources.PopBack( pOut );
            }

            bool Remove( handle_t hResource, ResourceType* pOut )
            {
                //return OpFunctions::Remove( &Resources, hResource, pOut );
                return Resources.Remove( hResource, pOut );
            }

            void Clear()
            {
                //OpFunctions::Clear( &Resources );
                //OpFunctions::Clear( &FreeResources );
                Resources.Clear();
                FreeResources.Clear();
            }
        };

        class VKE_API CResourceManager
        {
            
        };

    } // Core
} // VKE