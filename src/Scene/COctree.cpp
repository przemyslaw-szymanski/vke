#include "Scene/COctree.h"

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

        }

        Result COctree::_Create( const SOctreeDesc& Desc )
        {
            Result ret = VKE_OK;
            m_Desc = Desc;
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
            if( Frustum.Intersects( Node.m_BoundingSphere ) )
            {
                if( Frustum.Intersects( Node.m_AABB ) )
                {
                    _FrustumCullObjects( Frustum, Node );

                    const uint32_t childCount = Node.m_vChildBoundingSpheres.GetCount();
                    SOctreeNode::NodeArray vVisibles( childCount );

                    for( uint32_t i = 0; i < childCount; ++i )
                    {
                        if( Frustum.Intersects( Node.m_vChildBoundingSpheres[ i ] ) )
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
                *(Node.m_vpObjectVisibles[ i ]) = Frustum.Intersects( Node.m_vObjectAABBs[ i ] );
            }
        }

        handle_t COctree::AddObject( const Math::CAABB& AABB, bool* pVisible )
        {
            UObjectHandle Ret;
            Ret.nodeIndex = _CreateNode( &m_vNodes[0], AABB );
            auto& Node = m_vNodes[ Ret.nodeIndex ];
            Ret.objectIndex = Node.m_vObjectAABBs.PushBack( AABB );
            Node.m_vpObjectVisibles.PushBack( pVisible );
            return Ret.handle;
        }

        uint32_t COctree::_CreateNode( SOctreeNode* pParent, const Math::CAABB& AABB,
                                       uint32_t* pCurrLevel )
        {
            for( uint32_t i = 0; i < pParent->m_vChildAABBs.GetCount(); ++i )
            {

            }
        }

    } // Scene
} // VKE
