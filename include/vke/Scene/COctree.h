#pragma once

#include "Scene/Common.h"
#include "Core/Math/Math.h"
#include "Core/Utils/TCDynamicArray.h"
#include "Core/Utils/TCContainerBase.h"

namespace VKE
{
    namespace Scene
    {
        class CScene;

        struct OctreeNodeIndices
        {
            enum INDEX : uint8_t
            {
                LEFT_TOP_FAR,
                RIGHT_TOP_FAR,
                LEFT_TOP_NEAR,
                RIGHT_TOP_NEAR,

                LEFT_BOTTOM_FAR,
                RIGHT_BOTTOM_FAR,
                LEFT_BOTTOM_NEAR,
                RIGHT_BOTTOM_NEAR,
                _MAX_COUNT
            };
        };
        using OCTREE_NODE_INDEX = OctreeNodeIndices::INDEX;

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

            static const uint8_t    CHILD_NODE_INDEX_BIT_COUNT = 3;
            static const uint8_t    NODE_LEVEL_BIT_COUNT = 3;
            static const uint8_t    BUFFER_INDEX_BIT_COUNT = 32 - CHILD_NODE_INDEX_BIT_COUNT - NODE_LEVEL_BIT_COUNT;

            SOctreeNode() {}
            ~SOctreeNode();

            union UNodeMask
            {
                struct
                {
                    uint8_t     rightBottomNear : 1;
                    uint8_t     rightBottomFar : 1;
                    uint8_t     rightTopNear : 1;
                    uint8_t     rightTopFar : 1;
                    uint8_t     leftBottomNear : 1;
                    uint8_t     leftBottomFar : 1;
                    uint8_t     leftTopNear : 1;
                    uint8_t     leftTopFar : 1;
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

            union UNodeHandle
            {
                struct
                {
                    uint32_t    index   : BUFFER_INDEX_BIT_COUNT;
                    uint32_t    bit     : NODE_LEVEL_BIT_COUNT;
                    uint32_t    level   : CHILD_NODE_INDEX_BIT_COUNT;
                };
                uint32_t        handle = 0;
            };

            struct SNodeInfo
            {
                Math::CVector3  vecCenter;
                UNodeHandle     handle;
            };

            struct SCalcAABBInfo
            {
                Math::CVector4      vecExtraSize;
                Math::CVector4      vecMaxSize;
                Math::CVector4      vecParentCenter;
            };

            void CalcAABB( const SCalcAABBInfo& Info, Math::CAABB* pOut ) const;

            uint32_t                    m_parentNode;
            UNodeHandle                 m_handle;

            //TAABBArray< 8 >             m_vChildAABBs;
            NodeArray                   m_vChildNodes;
            //NodeArray                   m_vNeighbourNodes;

            TAABBArray< 1 >             m_vObjectAABBs;
            TObjBitsArray< 1 >          m_vpObjectBits;

            UNodeMask                   m_childNodeMask;
        };

        class COctree
        {
            friend class CScene;
            using NodeArray = Utils::TCDynamicArray< SOctreeNode, 1 >;
            using AABBArray = Utils::TCDynamicArray< Math::CAABB, 1 >;
            using SphereArray = Utils::TCDynamicArray< Math::CBoundingSphere, 1 >;
            using BoolArray = Utils::TCDynamicArray< bool, 1 >;
            using UintArray = Utils::TCDynamicArray< uint32_t, 1 >;
            using NodeInfoArray = Utils::TCDynamicArray< SOctreeNode::SNodeInfo, 1 >;
            
            using NodeHandle = SOctreeNode::UNodeHandle;

            union UObjectHandle
            {
                UObjectHandle() {}
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
                    NodeHandle  hNode;
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

                void        _FrustumCull( const Math::CFrustum& Frustum, const SOctreeNode& Node, const Math::CAABB& NodeAABB );
                void        _FrustumCullObjects( const Math::CFrustum& Frustum, const SOctreeNode& Node );
                NodeHandle  _CreateNode( SOctreeNode* pCurrent, const Math::CAABB& CurrentAABB,
                                         const SNodeData& Data, uint8_t* pCurrLevel );
                NodeHandle  _CreateNewNode( const SOctreeNode* pParent, const Math::CAABB& ParentAABB,
                                            OCTREE_NODE_INDEX idx, uint8_t level, Math::CAABB* pOut );

            protected:

                SOctreeDesc         m_Desc;
                CScene*             m_pScene;
                Math::CVector4      m_vecExtraSize;
                Math::CVector4      m_vecMaxSize;
                Math::CVector4      m_vecMinSize;
                Math::CAABB         m_RootAABB;

                NodeArray           m_vNodes;
                NodeInfoArray       m_vNodeInfos;
                UintArray           m_vVisibleAABBs;
                Threads::SyncObject m_NodeSyncObject;
        };
    } // Scene
} // VKE
