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
            m_vVisibleSpheres.Clear();
            m_vVisibleAABBs.Clear();

            for( uint32_t i = 0; i < m_vBoundingSpheres.GetCount(); ++i )
            {
                bool visible = Frustum.Intersects( m_vBoundingSpheres[ i ] );
                if( visible )
                {
                    m_vVisibleSpheres.PushBack( i );
                    
                }
            }
            for( uint32_t i = 0; i < m_vVisibleSpheres.GetCount(); ++i )
            {
                const uint32_t idx = m_vVisibleSpheres[ i ]; // cache miss here
                const Math::CAABB& AABB = m_vAABBs[idx];
                const bool visible = Frustum.Intersects( AABB );
                if( visible )
                {
                    m_vVisibleAABBs.PushBack( idx );
                }
            }
        }

    } // Scene
} // VKE
