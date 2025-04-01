#pragma once
#include "Core/Math/Math.h"
#include "Core/Resources/CImage.h"
#include "Scene/Common.h"
#include "Scene/Terrain/CTerrainQuadTree.h"
#include "Scene/Terrain/ITerrainRenderer.h"

namespace VKE
{
    namespace RenderSystem
    {
        class CDeviceContext;
    }
    namespace Scene
    {
        class ITerrainRenderer;
        class CTerrainVertexFetchRenderer;
        class CCamera;
        class CTerrain;
        struct STerrainRootNodeDesc
        {
            Math::CVector3 vecPosition;
        };

        namespace Terrain
        {
            class CQuadTree;
        }
        

        struct SLoadTerrainTileInfo
        {
            using NameArray = Utils::TCDynamicArray< ResourceName >;

            ExtentI32 Position;
            ResourceName Heightmap;
            ResourceName HeightmapNormal;
            NameArray vSplatmaps;
            NameArray vDiffuseTextures;
            NameArray vDiffuseNormalTextures;
            NameArray vSpecularTextures;
            NameArray vDisplacementTextures;
        };

        

        class VKE_API CTerrain
        {
            friend class CScene;
            friend class CTerrainQuadTree;
            friend class ITerrainRenderer;
            friend class CTerrainVertexFetchRenderer;
            friend class CTerrainMeshShadingRenderer;

            struct TextureTypes
            {
                enum TYPE
                {
                    HEIGHTMAP,
                    HEIGHTMAP_NORMAL,
                    DIFFUSE,
                    DIFFUSE_NORMAL,
                    SPLAT,
                    _MAX_COUNT
                };
            };

          public:
            static const uint8_t MAX_TEXTURE_COUNT = 2; // max texture count per root node
            using TextureHandleArray = Utils::TCDynamicArray<RenderSystem::TextureHandle, MAX_TEXTURE_COUNT>;
            using TexturePtrArray = Utils::TCDynamicArray<std::pair< RenderSystem::TextureRefPtr, uint32_t >, MAX_TEXTURE_COUNT >;
            using TextureViewArray = Utils::TCDynamicArray<RenderSystem::TextureViewHandle, MAX_TEXTURE_COUNT>;
            using TextureArrayArray = Utils::TCDynamicArray<TextureHandleArray, 1>;
            using TextureViewArrayArray = Utils::TCDynamicArray<TextureViewArray, 1>;

          public:
            CTerrain( CScene* pScene )
                : m_pScene( pScene )
            {
            }
            CScene* GetScene() const
            {
                return m_pScene;
            }
            const STerrainDesc& GetDesc() const
            {
                return m_Desc;
            }
            bool CheckDesc( const STerrainDesc& Desc ) const;
            void Update2( RenderSystem::CommandBufferPtr );
            void Update( RenderSystem::CommandBufferPtr );
            void Render( RenderSystem::CommandBufferPtr );
            handle_t CreateRoot( const STerrainRootNodeDesc& );
            void DestroyRoot( const handle_t& );
            static ExtentU16 CalcTextureCount( const STerrainDesc& Desc );

            void SetLODTreshold( float value );

            Result LoadTile( const SLoadTerrainTileInfo&, RenderSystem::CommandBufferPtr );
            void LoadTileAsync( const SLoadTerrainTileInfo&, RenderSystem::CommandBufferPtr );

          protected:
            Result _Create( const STerrainDesc& Desc, RenderSystem::CommandBufferPtr );
            Result _Resize();
            void _Destroy();
            void _DestroyRenderer( ITerrainRenderer** );
            RenderSystem::PipelinePtr _GetPipelineForLOD( uint8_t );
            Result _LoadTextures( RenderSystem::CDeviceContext* pCtx );
            Result _LoadTileTexture( RenderSystem::CDeviceContext* pCtx,
                const STerrainUpdateBindingData&,
                cstr_t pFileName, cstr_t pResourceName,
                TextureHandleArray* pvTextures, TextureViewArray* pvTexViews,
                TexturePtrArray* pvPendingTextures);
            Result _SplitTexture( RenderSystem::CDeviceContext* pCtx );
            Result _CreateDummyResources( RenderSystem::CommandBufferPtr );
            void _GetBindingDataForRootNode( const uint32_t& rootNodeIdx, STerrainUpdateBindingData* pOut );

          protected:
            STerrainDesc m_Desc;
            uint32_t m_maxTileCount;
            uint32_t m_maxVisibleTiles;
            uint16_t m_tileVertexCount; // num of vertices (in one row) of
                                        // highest lod tile
            uint32_t m_halfSize;        // terrain half size
            Math::CVector3 m_vecExtents;
            Math::CVector3 m_avecCorners[ 4 ];
            CTerrainQuadTree m_QuadTree;
            Terrain::CQuadTree* m_pQuadTree = nullptr;
            CTerrainQuadTree::STerrainInfo m_TerrainInfo;
            TextureHandleArray m_vDummyTextures;
            TextureViewArray m_vDummyTexViews;
            TextureHandleArray m_vHeightmapTextures;
            TextureHandleArray m_vHeightmapNormalTextures;
            TextureHandleArray m_vSplatmapTextures;
            TextureViewArray m_vSplatmapTexViews;
            TextureViewArray m_vHeightmapTexViews;
            TextureViewArray m_vHeightmapNormalTexViews;
            TextureHandleArray m_avTextures[ TextureTypes::_MAX_COUNT ];
            TexturePtrArray m_avpPendingTextures[ TextureTypes::_MAX_COUNT ];
            TextureViewArray m_avTextureViews[ TextureTypes::_MAX_COUNT ];
            // RenderSystem::TextureHandle
            // m_ahHeightmapTextures[MAX_HEIGHTMAP_TEXTURE_COUNT];
            // RenderSystem::TextureViewHandle m_ahHeightmapTextureViews[
            // MAX_HEIGHTMAP_TEXTURE_COUNT ]; RenderSystem::TextureViewHandle
            // m_hHeigtmapTexView = INVALID_HANDLE;
            RenderSystem::SamplerHandle m_hHeightmapSampler = INVALID_HANDLE;
            CScene* m_pScene;
            ITerrainRenderer* m_pRenderer = nullptr;
            uint32_t m_loadedTextureCount = 0;
        };
    } // namespace Scene
} // namespace VKE