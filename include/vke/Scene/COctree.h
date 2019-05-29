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


            uint32_t    m_parentNode;
            NodeArray   m_vChildNodes;
            NodeArray   m_vNeighbourNodes;
        };

        class COctree
        {
            friend class CScene;
            using NodeArray = Utils::TCDynamicArray< SOctreeNode, 1 >;
            using AABBArray = Utils::TCDynamicArray< Math::CAABB, 1 >;
            using SphereArray = Utils::TCDynamicArray< Math::CBoundingSphere, 1 >;
            using BoolArray = Utils::TCDynamicArray< bool, 1 >;
            using UintArray = Utils::TCDynamicArray< uint32_t, 1 >;

            public:

                COctree( CScene* pScnee );
                ~COctree();

                void        FrustumCull( const Math::CFrustum& Frustum );
                void        Build();

            protected:

                Result      _Create( const SOctreeDesc& Desc );
                void        _Destroy();

            protected:

                SOctreeDesc m_Desc;
                CScene*     m_pScene;
                NodeArray   m_vNodes;
                SphereArray m_vBoundingSpheres;
                AABBArray   m_vAABBs;
                UintArray   m_vVisibleSpheres;
                UintArray   m_vVisibleAABBs;
        };
    } // Scene
} // VKE
