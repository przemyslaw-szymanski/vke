#include "Scene/COctree.h"
#include "Scene/CScene.h"

namespace VKE
{
    namespace Scene
    {
        SOctreeNode::~SOctreeNode()
        {
            //m_vObjectAABBs.Destroy();
            //m_vpObjectBits.Destroy();
        }

        //void SOctreeNode::CalcAABB( const COctree* pOctree, Math::CAABB* pOut ) const
        //{
        //    static const Math::CVector4 aCenterVectors[ 9 ] =
        //    {
        //        Math::CVector4( -1.0f, 1.0f, 1.0f, 1.0f ),
        //        Math::CVector4( 1.0f, 1.0f, 1.0f, 1.0f ),
        //        Math::CVector4( -1.0f, 1.0f, -1.0f, 1.0f ),
        //        Math::CVector4( 1.0f, 1.0f, -1.0f, 1.0f ),

        //        Math::CVector4( -1.0f, -1.0f, 1.0f, 1.0f ),
        //        Math::CVector4( 1.0f, -1.0f, 1.0f, 1.0f ),
        //        Math::CVector4( -1.0f, -1.0f, -1.0f, 1.0f ),
        //        Math::CVector4( 1.0f, -1.0f, -1.0f, 1.0f ),
        //        Math::CVector4( 0.0f, 0.0f, 0.0f, 0.0f )
        //    };

        //    // Divide root size /2 this node level times
        //    const uint8_t level = m_handle.level;
        //    VKE_ASSERT2( level > 0, "" );
        //    //if( level > 0 )
        //    {
        //        const OCTREE_NODE_POSITION_INDEX index = static_cast< const OCTREE_NODE_POSITION_INDEX >( m_handle.bit );
        //        Math::CVector4 vecExtents;
        //        vecExtents.x = static_cast<float>( static_cast< uint32_t >( pOctree->m_vecMaxSize.x ) >> level );
        //        vecExtents.y = static_cast<float>( static_cast< uint32_t >( pOctree->m_vecMaxSize.y ) >> level );
        //        vecExtents.z = static_cast<float>( static_cast< uint32_t >( pOctree->m_vecMaxSize.z ) >> level );
        //        vecExtents.w = 0;

        //        Math::CVector4 vecCenter;
        //        // vecCenter = direction * distance + position
        //        Math::CVector4::Mad( aCenterVectors[ index ], vecExtents, vecParentCenter, &vecCenter );

        //        // vecExtentSize = vecPercentSize * vecExtentSize + vecExtentSize
        //        Math::CVector4::Mad( pOctree->m_vecExtraSize, vecExtents, vecExtents, &vecExtents );
        //        *pOut = Math::CAABB( Math::CVector3( vecCenter ), Math::CVector3( vecExtents ) );
        //    }
        //    /*else
        //    {
        //        Math::CVector4 vecExtents;
        //        Math::CVector4::Mad( Info.vecExtraSize, Info.vecMaxSize, Info.vecMaxSize, &vecExtents );
        //        *pOut = Math::CAABB( Math::CVector3( Info.vecParentCenter ), Math::CVector3( vecExtents ) );
        //    }*/
        //}

        float CalcNodeSize(float rootSize, uint8_t level)
        {
            float ret = (float)((uint32_t)rootSize >> level);
            return ret;
        }
        
        void CalcNodeCenter( const float rootSize, const Math::CVector4& vecParentCenter,
            const SOctreeNode::UNodeHandle& Handle, Math::CVector3* pOut )
        {
            static const Math::CVector4 aCenterVectors[9] =
            {
                Math::CVector4( -1.0f, 1.0f, 1.0f, 1.0f ),
                Math::CVector4( 1.0f, 1.0f, 1.0f, 1.0f ),
                Math::CVector4( -1.0f, 1.0f, -1.0f, 1.0f ),
                Math::CVector4( 1.0f, 1.0f, -1.0f, 1.0f ),

                Math::CVector4( -1.0f, -1.0f, 1.0f, 1.0f ),
                Math::CVector4( 1.0f, -1.0f, 1.0f, 1.0f ),
                Math::CVector4( -1.0f, -1.0f, -1.0f, 1.0f ),
                Math::CVector4( 1.0f, -1.0f, -1.0f, 1.0f ),
                Math::CVector4( 0.0f, 0.0f, 0.0f, 0.0f )
            };
            const OCTREE_NODE_POSITION_INDEX index = static_cast< const OCTREE_NODE_POSITION_INDEX >( Handle.bit );
            Math::CVector4 vecCenter;
            const float size = CalcNodeSize( rootSize, Handle.level );
            Math::CVector4 vecExtents( size );
            // vecCenter = direction * distance + position
            Math::CVector4::Mad( aCenterVectors[ index ], vecExtents, vecParentCenter, &vecCenter );
            *pOut = Math::CVector3( vecCenter );
        }

        void SOctreeNode::CalcAABB( const COctree* pOctree, Math::CAABB* pOut ) const
        {
            if( m_handle.level > 0 )
            {
                float size = (float)((uint32_t)(pOctree->m_vecMaxSize.x) >> m_handle.level);
                size += size * pOctree->m_vecExtraSize.x;
                size *= 0.5f;
                *pOut = Math::CAABB( m_vecCenter, Math::CVector3( size ) );
            }
            else
            {
                *pOut = pOctree->m_RootAABB;
            }
        }

        uint32_t SOctreeNode::AddObject( const SObjectData& Data )
        {
            uint32_t ret = UNDEFINED_U32;
            for( uint32_t i = 0; i < m_vObjData.GetCount(); ++i )
            {
                if( m_vObjData[ i ].Handle.handle == 0 )
                {
                    ret = i;
                    m_vObjData[ i ] = Data;
                    break;
                }
            }
            if( ret == UNDEFINED_U32 )
            {
                ret = m_vObjData.PushBack( Data );
            }
            return ret;
        }

        COctree::COctree( CScene* pScnee ) :
            m_pScene( pScnee )
        {

        }

        COctree::~COctree()
        {

        }

        void COctree::_Destroy()
        {

        }

        Result COctree::_Create( const SOctreeDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
            m_Desc.maxDepth = Math::Min( Desc.maxDepth, VKE_CALC_MAX_VALUE_FOR_BITS( SOctreeNode::NODE_LEVEL_BIT_COUNT ) );
            m_vecExtraSize = Math::CVector4( m_Desc.extraSizePercent );
            m_vecMaxSize = Math::CVector4( m_Desc.vec3MaxSize );
            m_vecMinSize = Math::CVector4( m_Desc.vec3MinSize );
            
            // Create root node
            m_vNodes.Reserve( 512 ); // 8 * 8 * 8
            //m_vNodeInfos.Reserve( 512 );

            SOctreeNode Root;
            Root.m_parentNode = 0;
            Root.m_handle.handle = 0;
            Root.m_vecCenter = Desc.vec3Center;

            m_RootAABB = Math::CAABB( Desc.vec3Center, Desc.vec3MaxSize );

            m_vNodes.PushBack( Root );


            return ret;
        }

        void COctree::Build()
        {

        }

        void COctree::FrustumCull( const Math::CFrustum& Frustum )
        {
            _FrustumCull( Frustum, m_vNodes[0], m_RootAABB );
        }

        void COctree::_FrustumCull( const Math::CFrustum& Frustum, const SOctreeNode& Node, const Math::CAABB& NodeAABB )
        {
            if( Frustum.Intersects( NodeAABB ) )
            {
                _FrustumCullObjects( Frustum, Node );

                Math::CAABB ChildAABB;

                for( uint32_t i = 0; i < Node.m_vChildNodes.GetCount(); ++i )
                {
                    const SOctreeNode& ChildNode = m_vNodes[ Node.m_vChildNodes[ i ] ];
                    ChildNode.CalcAABB( this, &ChildAABB );
                    _FrustumCull( Frustum, ChildNode, ChildAABB );
                }
            }
        }

        void COctree::_FrustumCullObjects( const Math::CFrustum& Frustum, const SOctreeNode& Node )
        {
            const uint32_t count = Node.m_vObjData.GetCount();
            for( uint32_t i = 0; i < count; ++i )
            {
                const auto& Curr = Node.m_vObjData[i];
                const bool visible = Frustum.Intersects( Curr.AABB );
                m_pScene->_SetObjectVisible( Curr.Handle, visible );
            }
        }

        COctree::UObjectHandle COctree::AddObject( const Math::CAABB& AABB, const Scene::UObjectHandle& handle )
        {
            UObjectHandle hRet;
            uint8_t level = 0;
            SNodeData Data;
            Data.AABB = AABB;
            AABB.CalcMinMax( &Data.MinMax );

            NodeHandle hNode = _CreateNode( &m_vNodes[0], m_RootAABB, Data, &level );
            hRet.hNode = hNode.handle;
            auto& Node = m_vNodes[ hNode.index ];
            hRet.index = Node.m_vObjData.PushBack( { AABB, handle } );
            return hRet;
        }

        COctree::UObjectHandle COctree::_AddObject( const NodeHandle& hNode, const Math::CAABB& AABB,
                                                    const Scene::UObjectHandle& handle )
        {
            auto& Node = m_vNodes[ hNode.index ];
            UObjectHandle hRet;
            hRet.hNode = hNode.handle;
            hRet.index = Node.AddObject( { AABB, handle } );
            return hRet;
        }

        COctree::UObjectHandle COctree::_UpdateObject( const handle_t& hGraph,
                                                       const Scene::UObjectHandle& hObj, const Math::CAABB& AABB )
        {
            UObjectHandle Handle;
            Handle.handle = hGraph;
            SOctreeNode::UNodeHandle hNode;
            hNode.handle = Handle.hNode;
            auto& CurrNode = m_vNodes[hNode.index];
            // Check if new AABB fits into current node
            auto hTmpNode = _CreateNodeForObject( AABB );
            if( hTmpNode.handle != hNode.handle )
            {
                CurrNode.m_vObjData[ Handle.index ].Handle.handle = 0; // invalidate this object
                Handle = _AddObject( hTmpNode, AABB, hObj );
            }
            return Handle;
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

        OCTREE_NODE_POSITION_INDEX CalcChildIndex( const Math::CVector4& vecNodeCenter,
                                          const Math::CVector4& vecObjectCenter )
        {
            OCTREE_NODE_POSITION_INDEX ret;
            // objectAABB.center <= OctreeAABB.center
            Math::CVector4 vecTmp1;
            Math::CVector4::LessOrEquals( vecObjectCenter, vecNodeCenter, &vecTmp1 );
            // Compare result is not a float value, make it 0-1 range
            bool aResults[ 4 ];
            vecTmp1.ConvertCompareToBools( aResults );

            /*static const bool LEFT = 1;
            static const bool RIGHT = 0;
            static const bool TOP = 0;
            static const bool BOTTOM = 1;
            static const bool NEAR = 0;
            static const bool FAR = 1;*/

            static const OCTREE_NODE_POSITION_INDEX aRets[2][2][2] =
            {
                // RIGHT
                { 
                    { OctreeNodePositionIndices::RIGHT_TOP_FAR, OctreeNodePositionIndices::RIGHT_TOP_NEAR }, // TOP
                    { OctreeNodePositionIndices::RIGHT_BOTTOM_FAR, OctreeNodePositionIndices::RIGHT_BOTTOM_NEAR } // BOTTOM
                },
                // LEFT
                { 
                    { OctreeNodePositionIndices::LEFT_TOP_FAR, OctreeNodePositionIndices::LEFT_TOP_NEAR }, // TOP
                    { OctreeNodePositionIndices::LEFT_BOTTOM_FAR, OctreeNodePositionIndices::LEFT_BOTTOM_NEAR } // BOTTOM
                }
            };

            ret = aRets[ aResults[0] ][ aResults[ 1 ] ][ aResults[ 2 ] ];
            return ret;
        }

        COctree::NodeHandle COctree::_CreateNode( SOctreeNode* pCurrent, const Math::CAABB& CurrentAABB,
            const SNodeData& Data, uint8_t* pCurrLevel )
        {
            NodeHandle ret = pCurrent->m_handle;
            // Finish here
            /*Math::CAABB CurrentAABB;
            SOctreeNode::SCalcAABBInfo Info;
            Info.vecExtraSize = m_vecExtraSize;
            Info.vecMaxSize = m_vecMaxSize;
            Info.vecParentCenter = Math::CVector4( m_vNodeInfos[ pCurrent->m_parentNode ].vecCenter );
       
            pCurrent->CalcAABB( Info, &CurrentAABB );*/

            if( *pCurrLevel < m_Desc.maxDepth &&
                CurrentAABB.Extents > ( m_Desc.vec3MinSize ) )
            {
                // Check which child node should constains the AABB
                SOctreeNode::UNodeMask NodeMask;
                SOctreeNode::UPositionMask PosMask;

                Math::CVector4 vecNodeCenter;
                CurrentAABB.CalcCenter( &vecNodeCenter );

                // Check children overlapping
                const bool overlappingChildren = Data.AABB.Contains( vecNodeCenter ) != Math::IntersectResults::OUTSIDE;
                if( !overlappingChildren )
                {
                    Math::CVector4 vecObjCenter;
                    Data.AABB.CalcCenter( &vecObjCenter );

                    // Select child index
                    OCTREE_NODE_POSITION_INDEX childIdx = CalcChildIndex( vecNodeCenter, vecObjCenter );
                    // If this child doesn't exist
                    const bool childExists = VKE_GET_BIT( pCurrent->m_childNodeMask.mask, childIdx );
                    if( !childExists )
                    {
                        ++( *pCurrLevel );
                        Math::CAABB ChildAABB;
                        const auto hNode = _CreateNewNode( pCurrent, CurrentAABB, childIdx, *pCurrLevel, &ChildAABB );
                        {
                            pCurrent->m_vChildNodes.PushBack( hNode.index );
                            pCurrent->m_childNodeMask.mask |= VKE_BIT( childIdx );
                        }
                        VKE_ASSERT2( pCurrent->m_vChildNodes.GetCount() <= 8, "" );
                        SOctreeNode& ChildNode = m_vNodes[ hNode.index ];
                        ret = _CreateNode( &ChildNode, ChildAABB, Data, pCurrLevel );
                    }
                }
            }

            return ret;
        }

        COctree::NodeHandle COctree::_CreateNodeForObject( const Math::CAABB& AABB )
        {
            uint8_t level = 0;
            SNodeData Data;
            Data.AABB = AABB;
            AABB.CalcMinMax( &Data.MinMax );
            return _CreateNode( &m_vNodes[ 0 ], m_RootAABB, Data, &level );
        }

        COctree::NodeHandle COctree::_CreateNewNode( const SOctreeNode* pParent,
            const Math::CAABB& ParentAABB, OCTREE_NODE_POSITION_INDEX idx, uint8_t level, Math::CAABB* pOut )
        {
            static const Math::CVector4 aCenterVectors[8] =
            {
                Math::CVector4( -1.0f, 1.0f, 1.0f, 1.0f ),
                Math::CVector4( 1.0f, 1.0f, 1.0f, 1.0f ),
                Math::CVector4( -1.0f, 1.0f, -1.0f, 1.0f ),
                Math::CVector4( 1.0f, 1.0f, -1.0f, 1.0f ),

                Math::CVector4( -1.0f, -1.0f, 1.0f, 1.0f ),
                Math::CVector4( 1.0f, -1.0f, 1.0f, 1.0f ),
                Math::CVector4( -1.0f, -1.0f, -1.0f, 1.0f ),
                Math::CVector4( 1.0f, -1.0f, -1.0f, 1.0f ),
            };

            static const Math::CVector4 HALF( 0.5f );
            const Math::CVector4 EXTRA_SIZE( m_Desc.extraSizePercent );
            NodeHandle hRet;
            {
                Threads::ScopedLock l( m_NodeSyncObject );
                hRet.index = m_vNodes.PushBack( {} );
                VKE_ASSERT2( m_vNodes.GetCount() < VKE_CALC_MAX_VALUE_FOR_BITS( SOctreeNode::BUFFER_INDEX_BIT_COUNT ), "" );
            }
            SOctreeNode& Node = m_vNodes[ hRet.index ];
            hRet.bit = idx;
            hRet.level = level;
            Node.m_handle = hRet;
            Node.m_parentNode = pParent->m_handle.index;
            CalcNodeCenter( m_vecMaxSize.x, Math::CVector4( pParent->m_vecCenter ), hRet, &Node.m_vecCenter );

            Node.CalcAABB( this, pOut );

            return hRet;
        }

    } // Scene
} // VKE
