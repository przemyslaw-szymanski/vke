#pragma once
#include "Core/VKECommon.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
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
                if( Buffer.vPool.PushBack( Ref ) != Utils::INVALID_POSITION )
                {
                    mAllocatedHashes.insert( Itr, Res );
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
                if( FindAllocated( &Itr ) )
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
                    if( Buffer.vFreeElements.PopBack( &pResOut ) )
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


        class VKE_API CResourceManager
        {
            
        };

    } // Core
} // VKE