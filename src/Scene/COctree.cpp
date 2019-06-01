#include "Scene/COctree.h"
#include "Scene/CScene.h"

namespace VKE
{
    namespace Scene
    {
        COctree::COctree( CScene* pScnee ) :
            m_pScene( pScnee )
        {

        }

        COctree::~COctree()
        {

        }

        void COctree::_Destroy()
        {
            m_vNodes.Clear();
            m_vAABBs.Clear();
        }

        Result COctree::_Create( const SOctreeDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            
            // Create root node
            m_vNodes.Reserve( 512 ); // 8 * 8 * 8
            m_vAABBs.Reserve( 512 );

            Math::CAABB AABB( Math::CVector3::ZERO, Desc.vec3MaxSize );
            SOctreeNode Root;
            Root.m_handle = 0;
            Root.m_parentNode = 0;

            m_vNodes.PushBack( Root );
            m_vAABBs.PushBack( AABB );

            return ret;
        }

        void COctree::Build()
        {

        }

        void COctree::FrustumCull( const Math::CFrustum& Frustum )
        {
            _FrustumCull( Frustum, m_vNodes[0] );
        }

        void COctree::_FrustumCull( const Math::CFrustum& Frustum, const SOctreeNode& Node )
        {
            const auto& AABB = m_vAABBs[Node.m_handle];
            Math::CBoundingSphere Sphere;
            AABB.CalcSphere( &Sphere );

            if( Frustum.Intersects( Sphere ) )
            {
                if( Frustum.Intersects( m_vAABBs[Node.m_handle] ) )
                {
                    _FrustumCullObjects( Frustum, Node );

                    const uint32_t childCount = Node.m_vChildAABBs.GetCount();
                    SOctreeNode::NodeArray vVisibles( childCount );

                    for( uint32_t i = 0; i < childCount; ++i )
                    {
                        const auto& AABB = Node.m_vChildAABBs[ i ];
                        AABB.CalcSphere( &Sphere );
                        if( Frustum.Intersects( Sphere ) )
                        {
                            if( Frustum.Intersects( Node.m_vChildAABBs[ i ] ) )
                            {
                                vVisibles.PushBack( i );
                            }
                        }
                    }
                    for( uint32_t i = 0; i < vVisibles.GetCount(); ++i )
                    {
                        const auto idx = Node.m_vChildNodes[ vVisibles[ i ] ];
                        const auto& Child = m_vNodes[ idx ];
                        _FrustumCull( Frustum, Child );
                    }
                }
            } 
        }

        void COctree::_FrustumCullObjects( const Math::CFrustum& Frustum, const SOctreeNode& Node )
        {
            const uint32_t count = Node.m_vObjectAABBs.GetCount();
            for( uint32_t i = 0; i < count; ++i )
            {
                Node.m_vpObjectBits[ i ]->visible = Frustum.Intersects( Node.m_vObjectAABBs[ i ] );
            }
        }

        handle_t COctree::AddObject( const Math::CAABB& AABB, UObjectBits* pBits )
        {
            UObjectHandle Ret;
            uint32_t level = 0;
            SNodeData Data;
            Data.AABB = AABB;
            AABB.CalcMinMax( &Data.MinMax );

            Ret.nodeIndex = _CreateNode( &m_vNodes[0], Data, &level );
            auto& Node = m_vNodes[ Ret.nodeIndex ];
            Ret.objectIndex = Node.m_vObjectAABBs.PushBack( AABB );
            Node.m_vpObjectBits.PushBack( pBits );
            return Ret.handle;
        }

        bool InBounds( const Math::CVector4& V, const Math::CVector4& Bounds )
        {
            Math::CVector4 vec4Tmp1, vec4Tmp2, vec4NegBounds;
            Math::CVector4::LessOrEquals( V, Bounds, &vec4Tmp1 );
            Math::CVector4::Mul( Bounds, Math::CVector4::NEGATIVE_ONE, &vec4Tmp2 );
            Math::CVector4::LessOrEquals( vec4Tmp2, V, &vec4Tmp2 );
            Math::CVector4::And( vec4Tmp1, vec4Tmp2, &vec4Tmp1 );
            const auto res = Math::CVector4::MoveMask( vec4Tmp1 ) & 0x7;
            return ( res == 0x7 ) != 0;
        }

        struct OctreeNodeIndices
        {
            enum INDEX
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

        OCTREE_NODE_INDEX CalcChildIndex( const bool* pLeftBottomNear )
        {
            /*static const bool LEFT = 1;
            static const bool RIGHT = 0;
            static const bool TOP = 0;
            static const bool BOTTOM = 1;
            static const bool NEAR = 0;
            static const bool FAR = 1;*/

            static const OCTREE_NODE_INDEX aRets[2][2][2] =
            {
                // RIGHT
                { 
                    { OctreeNodeIndices::RIGHT_TOP_NEAR, OctreeNodeIndices::RIGHT_TOP_FAR }, // TOP
                    { OctreeNodeIndices::RIGHT_BOTTOM_NEAR, OctreeNodeIndices::RIGHT_BOTTOM_FAR } // BOTTOM
                },
                // LEFT
                { 
                    { OctreeNodeIndices::LEFT_TOP_NEAR, OctreeNodeIndices::LEFT_TOP_FAR }, // TOP
                    { OctreeNodeIndices::LEFT_BOTTOM_NEAR, OctreeNodeIndices::LEFT_BOTTOM_FAR } // BOTTOM
                }
            };

            return aRets[pLeftBottomNear[0]][pLeftBottomNear[1]][pLeftBottomNear[2]];
        }

        handle_t COctree::_CreateNode( SOctreeNode* pCurrent, const SNodeData& Data, uint32_t* pCurrLevel )
        {
            handle_t ret = pCurrent->m_handle;
            // Finish here
            if( *pCurrLevel == m_Desc.maxDepth )
            {
                Threads::ScopedLock l( m_NodeSyncObject );
                
            }

            const auto& CurrentAABB = m_vAABBs[pCurrent->m_handle];
            
            

            // Check which child node should constains the AABB
            SOctreeNode::UNodeMask NodeMask;
            SOctreeNode::UPositionMask PosMask;
            
            // Check children overlapping
            Math::CVector4 CurrentNodeCenter, ObjectCenter;
            CurrentAABB.CalcCenter( &CurrentNodeCenter );
            Data.AABB.CalcCenter( &ObjectCenter );
            Math::CVector4 V, ObjectExtents, vecTmp1, vecTmp2;
            Data.AABB.CalcExtents( &ObjectExtents );
            Math::CVector4::Sub( CurrentNodeCenter, ObjectCenter, &V );
            /*
            // Test if less than or equal
            XMVECTOR vTemp1 = _mm_cmple_ps(V,Bounds);
            // Negate the bounds
            XMVECTOR vTemp2 = _mm_mul_ps(Bounds,g_XMNegativeOne);
            // Test if greater or equal (Reversed)
            vTemp2 = _mm_cmple_ps(vTemp2,V);
            // Blend answers
            vTemp1 = _mm_and_ps(vTemp1,vTemp2);
            // x,y and z in bounds? (w is don't care)
            return (((_mm_movemask_ps(vTemp1)&0x7)==0x7) != 0);
            */
            Math::CVector4::LessOrEquals( V, ObjectExtents, &vecTmp1 );
            Math::CVector4 vecSaturated;
            Math::CVector4::ConvertUintToFloat( vecTmp1, &vecSaturated );
            Math::CVector4::Saturate( vecSaturated, &vecSaturated );
            uint32_t aResults[4];
            vecSaturated.ConvertToUInts( aResults );
            Math::CVector4::Mul( ObjectExtents, Math::CVector4::NEGATIVE_ONE, &vecTmp2 );
            Math::CVector4::LessOrEquals( vecTmp2, V, &vecTmp2 );
            Math::CVector4::And( vecTmp1, vecTmp2, &vecTmp1 );
            const bool overlappingChildren = ((Math::CVector4::MoveMask( vecTmp1 ) & 0x7) == 0x7) != 0;

            // Select child index
            OCTREE_NODE_INDEX childIdx = CalcChildIndex( reinterpret_cast< bool* >( aResults ) );

            if( !overlappingChildren )
            {
                const auto& vChildAABBs = pCurrent->m_vChildAABBs;
                const auto count = vChildAABBs.GetCount();
                bool foundChildNode = false;

                // Find best fit child
                for( uint32_t i = 0; i < pCurrent->m_vChildAABBs.GetCount(); ++i )
                {
                    const auto& TmpAABB = pCurrent->m_vChildAABBs[i];
                    if( TmpAABB.Contains( Data.AABB ) )
                    {
                        const auto childIdx = pCurrent->m_vChildNodes[i];
                        ++(*pCurrLevel);
                        foundChildNode = true;
                        ret = _CreateNode( &m_vNodes[childIdx], Data, pCurrLevel );
                        break;
                    }
                }

                // if not found child node fit
                if( !foundChildNode )
                {

                }
            }

            return ret;
        }

    } // Scene
} // VKE
