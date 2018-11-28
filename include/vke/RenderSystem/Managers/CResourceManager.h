#pragma once
#include "RenderSystem/Common.h"
#include "Core/Utils/TCDynamicArray.h"
#include "RenderSystem/Resources/CTexture.h"
namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
        class CAPIResourceManager;

        template
        <
            class ResourceType,
            class FreeResourceType,
            uint32_t BASE_RESOURCE_COUNT,
            typename HashType
        >
        struct TSResourceBuffer
        {
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

            void Free( const HashType& hash )
            {
                MapIterator Itr;
                //VKE_ASSERT( !FindFree( hash, &Itr ), "The same resource can not be freed more than once." );
                if( FindAllocated( &Itr ) )
                {
                    Buffer.vFreeElements( static_cast< FreeResourceType >( Itr->second ) );
                    //mFreeHashes.insert( { hash, static_cast< FreeResourceType >( Itr->second ) } );
                }
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

            ResourceType& GetFree( const HashType& hash )
            {
                FreeIterator Itr;
                FindFree( hash, &Itr );
                if( Itr != mFreeHashes.end() )
                {
                    mFreeHashes.erase( Itr );
                    return Itr->second;
                }
                return ResourceType();
            }

            template<typename ... ARGS>
            bool Get(const HashType& hash, ResourceType* pResOut, MapIterator* pItrOut, Memory::IAllocator* pAllocator,
                ARGS... args)
            {
                bool ret = Get( hash, pResOut, pItrOut );
                if( !ret && VKE_SUCCEEDED( Memory::CreateObject( pAllocator, pResOut, args ) ) )
                {
                    ret = true;
                }
                return ret;
            }
        };

        template
        <
            class ResourceType,
            class FreeResourceType,
            typename HashType
        >
        struct TSResourceMap
        {
            using Map = vke_hash_map< HashType, ResourceType >;
            using FreeMap = vke_hash_map< HashType, FreeResourceType >;
            //using Pool = Utils::TCDynamicArray< ResourceType, BASE_RESOURCE_COUNT >;
            using MapIterator = typename Map::iterator;
            using MapConstIterator = typename Map::const_iterator;
            using FreeIterator = typename FreeMap::iterator;
            using FreeConstIterator = typename FreeMap::const_iterator;

            Map     mAllocated;
            FreeMap mFreed;

            void Add( const HashType& hash, const ResourceType& Res, const MapConstIterator& Itr )
            {
                mAllocated.insert( Itr, Res );
            }

            template<class MapType, class ItrType>
            bool Find( MapType& mHashes, const HashType& hash, ItrType* pOut )
            {
                auto Itr = mHashes.lower_bound( hash );
                *pOut = Itr;
                bool ret = false;
                if( Itr != mHashes.end() && !(mHashes.key_comp()(hash, Itr->first)) )
                {
                    ret = true;
                }
                return ret;
            }

            ResourceType& Find( const HashType& hash )
            {
                auto Itr = mAllocated.find( hash );
                return Itr->second;
            }

            bool FindAllocated( const HashType& hash, MapIterator* pOut )
            {
                return Find( mAllocated, hash, pOut );
            }

            bool FindFree( const HashType& hash, FreeIterator* pOut )
            {
                return Find( mFreed, hash, pOut );
            }

            void Free( const HashType& hash )
            {
                MapIterator Itr = mAllocated.find( hash );
                VKE_ASSERT( Itr != mAllocated.end(), "" );
                VKE_ASSERT( mFreed.find( hash ) == mFreed.end(), "Resource is already freed." );
                mFreed.insert( hash, Itr->second );
                mAllocated.erase( Itr );
            }

            ResourceType& GetFree( const HashType& hash )
            {
                FreeIterator FreeItr = mFreed.find( hash );
                if( FreeItr != mFreed.end() )
                {
                    MapIterator Itr = mAllocated.lower_bound( hash );
                    mAllocated.insert( Itr, FreeItr->second );
                    mFreed.erase( FreeItr );
                    return Itr->second;
                }
                return ResourceType();
            }
        };


        namespace Managers
        {

            /*class VKE_API CResourceManager
            {
                friend class CDeviceContext;
                friend class CAPIResourceManager;

                    using TextureBuffer = Utils::TSFreePool< TextureSmartPtr, CTexture* >;

                    Memory::IAllocator*     m_pTextureAllocator = &HeapAllocator;

                public:

                                        CResourceManager(CDeviceContext* pCtx);
                                        ~CResourceManager();

                    Result              Create(const SResourceManagerDesc& Desc);
                    void                Destroy();

                    TexturePtr          CreateTexture(const STextureDesc& Desc);
                    void                FreeTexture(TexturePtr* ppTextureInOut);
                    void                DestroyTexture(TexturePtr* ppTextureInOut);
                    void                DestroyTextures();

                    Result              CreateTextureView(const STextureViewDesc& Desc, TexturePtr* ppTexInOut);
                    Result              CreateTextureView(TexturePtr* ppTexInOut);

                protected:

                    

                protected:

                    CDeviceContext*         m_pCtx;
                    CAPIResourceManager*    m_pAPIResMgr;
                    CommandBufferPtr        m_pInitialCommandBuffer;
                    TextureBuffer           m_Textures;
            };*/
        } // Managers
    } // RenderSystem
} // VKE