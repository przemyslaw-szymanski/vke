#pragma once

#include "Scene/Common.h"
#include "Core/Math/CAABB.h"
#include "Core/Math/CBoundingSphere.h"
#include "Core/Math/CFrustum.h"
#include "Core/Utils/TCDynamicArray.h"

namespace VKE
{
    namespace Scene
    {
        class CScene;

        struct SOctreeNode
        {
            using NodeArray = Utils::TCDynamicArray< uint32_t, 8 >;
            template<uint32_t COUNT >
            using TAABBArray = Utils::TCDynamicArray< Math::CAABB, COUNT >;
            template<uint32_t COUNT >
            using TBoundingSphereArray = Utils::TCDynamicArray< Math::CBoundingSphere, COUNT >;
            template<uint32_t COUNT >
            using TBoolArray = Utils::TCDynamicArray< bool, COUNT >;
            template<uint32_t COUNT >
            using TBoolPtrArray = Utils::TCDynamicArray< bool*, COUNT >;
            template<uint32_t COUNT >
            using THandleArray = Utils::TCDynamicArray< handle_t, COUNT >;
            template<uint32_t COUNT>
            using TObjBitsArray = Utils::TCDynamicArray< UObjectBits*, 1 >;

            union UNodeMask
            {
                struct
                {
                    uint8_t     topLeftFar : 1;
                    uint8_t     topRightFar : 1;
                    uint8_t     topLeftNear : 1;
                    uint8_t     topRightNear : 1;
                    uint8_t     bottomLeftFar : 1;
                    uint8_t     bottomRightFar : 1;
                    uint8_t     bottomLeftNear : 1;
                    uint8_t     bottomghtNear : 1;
                };
                uint8_t         mask = 0;
            };

            union UPositionMask
            {
                struct
                {
                    uint8_t     top : 1;
                    uint8_t     left : 1;
                    uint8_t     far : 1;
                };
                uint8_t         mask = 0;
            };

            uint32_t                    m_parentNode;
            uint32_t                    m_handle;

            TAABBArray< 8 >             m_vChildAABBs;
            NodeArray                   m_vChildNodes;
            NodeArray                   m_vNeighbourNodes;

            TAABBArray< 1 >             m_vObjectAABBs;
            TObjBitsArray< 1 >          m_vpObjectBits;
        };

        class COctree
        {
            friend class CScene;
            using NodeArray = Utils::TCDynamicArray< SOctreeNode, 1 >;
            using AABBArray = Utils::TCDynamicArray< Math::CAABB, 1 >;
            using SphereArray = Utils::TCDynamicArray< Math::CBoundingSphere, 1 >;
            using BoolArray = Utils::TCDynamicArray< bool, 1 >;
            using UintArray = Utils::TCDynamicArray< uint32_t, 1 >;

            union UObjectHandle
            {
                /*struct 
                {
                    uint64_t    index1 : 4;
                    uint64_t    index2 : 4;
                    uint64_t    index3 : 4;
                    uint64_t    index4 : 4;
                    uint64_t    index5 : 4;
                    uint64_t    index6 : 4;
                    uint64_t    index7 : 4;
                    uint64_t    index8 : 4;
                    uint64_t    index : 32;
                };*/
                struct
                {
                    uint32_t    nodeIndex;
                    uint32_t    objectIndex;
                };
                handle_t        handle = 0;
                
            };

            public:

                COctree( CScene* pScnee );
                ~COctree();

                void        FrustumCull( const Math::CFrustum& Frustum );
                void        Build();

                handle_t    AddObject( const Math::CAABB& AABB, UObjectBits* pBits );

            protected:

                struct SNodeData
                {
                    SNodeData() = default;
                    ~SNodeData() {}

                    Math::CAABB             AABB;
                    Math::CAABB::SMinMax    MinMax;
                };

                Result      _Create( const SOctreeDesc& Desc );
                void        _Destroy();

                void        _FrustumCull( const Math::CFrustum& Frustum, const SOctreeNode& Node );
                void        _FrustumCullObjects( const Math::CFrustum& Frustum, const SOctreeNode& Node );
                handle_t    _CreateNode( SOctreeNode* pParent, const SNodeData& Data, uint32_t* pCurrLevel );

            protected:

                SOctreeDesc         m_Desc;
                CScene*             m_pScene;
                NodeArray           m_vNodes;
                AABBArray           m_vAABBs;
                UintArray           m_vVisibleAABBs;
                Threads::SyncObject m_NodeSyncObject;
        };
    } // Scene
} // VKE
