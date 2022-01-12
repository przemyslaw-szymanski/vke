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
            uint32_t BASE_RESOURCE_COUNT = 32
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
            using Pool = Utils::TSFreePool< ResourceType, uint32_t, BASE_RESOURCE_COUNT >;


            Pool    Buffer;
            Map     mAllocatedHashes;
            FreeMap mFreeHashes;

            bool Add( const HashType& hash, const ResourceType& Res, const MapConstIterator& Itr )
            {
                bool res = false;
                if( Buffer.vPool.PushBack( Res ) != INVALID_POSITION )
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
                    Free( ( Itr->second->GetHandle() ) );
                    //mFreeHashes.insert( { hash, static_cast< FreeResourceType >( Itr->second ) } );
                }
            }

            void Free(uint32_t handle)
            {
                Buffer.vFreeElements.PushBack( handle );
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
                    uint32_t handle;
                    if( Buffer.vFreeElements.PopBack( &handle ) )
                    {
                        *pResOut = Buffer.vPool[handle];
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
            class ResourceType,
            typename HandleType,
            uint32_t DEFAULT_COUNT = 32
        >
        struct TSUniqueResourceBuffer
        {
            using ContainerType = vke_hash_map< HandleType, ResourceType >;
            using FreeContainerType = Utils::TCDynamicArray< HandleType, DEFAULT_COUNT >;
            //using FreeContainerType = vke_hash_map< HandleType, FreeResourceType >;
            using Iterator = typename ContainerType::iterator;
            using ConstIterator = typename ContainerType::const_iterator;

            ContainerType mContainer;
            FreeContainerType vFreeContainer;

            bool GetFree( ResourceType* pOut )
            {
                HandleType handle;
                bool ret = vFreeContainer.PopBack( &handle );
                if( ret )
                {
                    ret = Find( handle, pOut );
                }
                return ret;
            }

            bool Add( const HandleType& handle, const ResourceType& res )
            {
                bool ret = true;
                mContainer[handle] = res;
                return ret;
            }

            bool Add( const HandleType& hash, const ResourceType& Res, const ConstIterator& Itr )
            {
                bool ret = true;
                mContainer.insert( Itr, { hash, Res } );
                return ret;
            }

            bool Remove( const HandleType& handle, ResourceType* pOut )
            {
                bool ret = false;
                auto Itr = mContainer.find( handle );
                if( Itr != mContainer.end() )
                {
                    *pOut = Itr;
                    mContainer.erase( Itr );
                    ret = true;
                }
                return ret;
            }

            bool Remove( const HandleType& handle )
            {
                bool ret = false;
                auto Itr = mContainer.find( handle );
                if( Itr != mContainer.end() )
                {
                    mContainer.erase( Itr );
                    ret = true;
                }
                return ret;
            }

            bool TryToReuse( const HandleType& handle, ResourceType* pOut )
            {
                bool ret = false;
                auto Itr = mContainer.find( handle );
                if( Itr != mContainer.end() )
                {
                    ret = true;
                    *pOut = Itr->second;
                }
                return ret;
            }

            bool Find( const HandleType& handle, ResourceType* pOut )
            {
                bool ret = false;
                auto Itr = mContainer.find( handle );
                if( Itr != mContainer.end() )
                {
                    ret = true;
                    *pOut = Itr->second;
                }
                return ret;
            }

            bool Find( const HandleType& handle, ResourceType* pOut, Iterator* pItrOut )
            {
                auto Itr = mContainer.lower_bound( handle );
                *pItrOut = Itr;
                bool ret = false;
                if( Itr != mContainer.end() && !( mContainer.key_comp()( handle, Itr->first ) ) )
                {
                    *pOut = Itr.second;
                    ret = true;
                }
                return ret;
            }

            void Free( const HandleType& handle )
            {
                vFreeContainer.PushBack( handle );
            }

            void Clear()
            {
                mContainer.clear();
                vFreeContainer.Clear();
            }

            void ClearFull()
            {
                mContainer.clear();
                vFreeContainer.ClearFull();
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

            Key             LastUsedKey;
            Value*          pLastUsedValue = nullptr;
            ContainerType   Container;

            bool Find(const Key& key, Value* pOut )
            {
                bool ret = false;
                if( LastUsedKey != key )
                {
                    auto Itr = Container.find( key );
                    if( Itr != Container.end() )
                    {
                        LastUsedKey = key;
                        pLastUsedValue = &Itr->second;
                        *pOut = Itr->second;
                        ret = true;
                    }
                }
                else
                {
                    *pOut = *pLastUsedValue;
                    ret = true;
                }
                return ret;
            }

            Iterator Find( const Key& key )
            {
                return Container.find( key );
            }

            Iterator FindPlace( const Key& key )
            {
                return Container.find( key );
            }

            bool Find( const Key& key, Value* pOut, Iterator* pItrOut )
            {
                bool ret = false;
                if( LastUsedKey == key )
                {
                    ret = true;
                    *pOut = *pLastUsedValue;
                }
                else
                {
                    auto Itr = FindPlace( key );
                    *pItrOut = Itr;
                    if( Itr != Container.end() && !(Container.key_eq()(key, Itr->first)) )
                    {
                        LastUsedKey = key;
                        pLastUsedValue = &Itr->second;
                        *pOut = Itr->second;
                        ret = true;
                    }
                }
                return ret;
            }

            void Insert( const Iterator& Itr, const Key& key, const Value& value )
            {
                Container.insert( Itr, ContainerType::value_type( key, value ) );
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

            /*bool Remove( const Key& key, Value* pOut )
            {
                bool ret;
                if( LastUsedItr->first == key )
                {
                    ret = true;
                    *pOut = LastUsedItr->second;
                    Container.erase( LastUsedItr );
                    LastUsedItr = {};
                }
                else
                {
                    Iterator Itr = Find( key, pOut );
                    ret = Itr != End();
                    if( ret )
                    {
                        *pOut = Itr->second;
                        Container.erase( Itr );
                    }
                }
                return ret;
            }*/

            bool Remove( const Key& key )
            {
                bool ret;
                {
                    Value v;
                    Iterator Itr;
                    ret = Find( key, &v, &Itr );
                    if( ret )
                    {
                        Container.erase( Itr );
                        LastUsedKey = {};
                        pLastUsedValue = nullptr;
                    }
                }
                return ret;
            }

            void Remove( const Iterator& Itr )
            {
                Container.erase( Itr );
            }

            Value& operator[](const Key& key)
            {
                return Container[ key ];
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

            /*bool Insert( const handle_t&, const Value& value )
            {
                return Container.PushBack( value ) != Utils::INVALID_POSITION;
            }*/

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

            bool Remove( const handle_t& hResource, ResourceType* pOut )
            {
                return OpFunctions::Remove( &Resources, hResource, pOut );
            }

            bool Remove( const handle_t& hResource )
            {
                ResourceType Res;
                return OpFunctions::Remove( &Resources, hResource, &Res );
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
        class TCMultimapResourceBuffer
        {
            public:

                using ContainerType = TSMultimap< handle_t, ResourceType, DEFAULT_COUNT >;
                using FreeContainerType = TSMultimap< handle_t, FreeResourceType, DEFAULT_COUNT >;

                uint32_t Add( const handle_t& handle, const ResourceType& res )
                {
                    return Resources.Insert( handle, res );
                }

                ContainerType       Resources;
                FreeContainerType   FreeResources;
        };

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
            using ResourceBufferType = TSHashMap< handle_t, ResourceType >;
            using ResourceIterator = typename ResourceBufferType::Iterator;

            struct SFreeResource
            {
                ResourceIterator    Iterator;
                FreeResourceType    Resource;
            };

            using FreeResourceBufferType = Utils::TCDynamicArray< ResourceIterator, DEFAULT_COUNT >;

            ResourceBufferType      Resources;
            FreeResourceBufferType  FreeResources;

            ResourceType operator[](const handle_t& handle)
            {
                ResourceType Ret = (ResourceType)0;
                Find(handle, &Ret);
                return Ret;
            }

            bool Reuse( const handle_t& hFind, const handle_t& hNew, ResourceType* pOut )
            {
                bool ret = false;
                ResourceIterator Itr = Resources.End();
                if( hFind != INVALID_HANDLE )
                {
                    uint32_t idx = FindFree( hFind, &Itr );
                    if( idx != INVALID_POSITION )
                    {
                        FreeResources.RemoveFast( idx );
                    }
                }
                else
                {
                    FreeResources.PopBack( &Itr );
                }
                if( Itr != Resources.End() )
                {
                    ret = true;
                    *pOut = Itr->second;
                    if( hFind != hNew )
                    {
                        Resources.Remove( Itr );
                        ret = Add( hNew, *pOut );
                    }
                }
                return ret;
            }

            bool Add( handle_t hResource, const ResourceType& Res )
            {
                //return OpFunctions::Add( &Resources, hResource, Res );
                return Resources.Insert( hResource, Res );
            }

            void AddFree( handle_t key )
            {
                auto Itr = Resources.Find( key );
                FreeResources.PushBack( Itr );
            }

            bool Find( handle_t hResource, ResourceType* pOut )
            {
                //return OpFunctions::Find( &Resources, hResource, pOut );
                return Resources.Find( hResource, pOut );
            }

            bool FindFree( handle_t hResource, FreeResourceType* pOut )
            {
                bool ret = false;
                for( uint32_t i = 0; i < FreeResources.GetCount(); ++i )
                {
                    if( FreeResources[i]->GetHandle().handle == hResource )
                    {
                        *pOut = FreeResources[i];
                        ret = true;
                        break;
                    }
                }
                return ret;
            }

            uint32_t FindFree( const handle_t& hResource, ResourceIterator* pOut )
            {
                uint32_t ret = INVALID_POSITION;
                for( uint32_t i = 0; i < FreeResources.GetCount(); ++i )
                {
                    if( FreeResources[ i ]->first == hResource )
                    {
                        *pOut = FreeResources[ i ];
                        ret = i;
                        break;
                    }
                }
                return ret;
            }

            ResourceIterator GetFree( FreeResourceType* pOut )
            {
                //SFreeResource Tmp;
                //Tmp.Iterator = Resources.End();
                ResourceIterator Ret = Resources.End();
                if( FreeResources.PopBack( &Ret ) )
                {
                    *pOut = Ret->second;
                }
                return Ret;
            }

            bool IsValid( const ResourceIterator& Itr )
            {
                return Itr != Resources.End();
            }

            void Remove( const ResourceIterator& Itr )
            {
                Resources.Remove( Itr );
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