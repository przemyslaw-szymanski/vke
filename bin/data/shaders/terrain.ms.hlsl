#define DEBUG 0

struct VertexOut
{
    float4 PositionHS : SV_Position;
    float3 Color : COLOR0;
    //float3 PositionVS : POSITION0;
    //float3 Normal : NORMAL0;
    //uint MeshletIndex : COLOR0;
};

struct Vertex
{
    float2 Position;
    //float2 Texcoords;
};

struct Meshlet
{
    float2 position;
    float pad1;
    float pad2;
    float3 Color;
    float pad3;
};

struct STileData
{
    int2 Position;
    uint2 HeightmapOffset;
    uint indices;
};

struct SFrustum
{
    float4 near;
    float4 far;
    float4 rightSlope;
    float4 leftSlope;
    float4 topSlope;
    float4 bottomSlope;
};

struct SCamera
{
    float4x4 mtxViewProj;
    float3 position;
    float pad1;
    float3 direction;
    float pad2;
    float4 FrustumPlanes[6];
    float3 lightPos;
    float lightRadius;
    float lightDir;
    float lightAttenuation;
    float3 lightColor;
};

StructuredBuffer<Vertex> Vertices : register(t0, space0);
//StructuredBuffer<uint> Indices : register(t1, space0);
#if DEBUG
StructuredBuffer<uint4> Triangles : register(t1, space0);
#else
StructuredBuffer<uint> Triangles : register(t1, space0);
#endif
StructuredBuffer<Meshlet> Meshlets : register(t2, space0);
StructuredBuffer<STileData> TileData : register(t3, space0);

StructuredBuffer<float4> Debug : register(t4, space0);

ConstantBuffer<SCamera> Camera : register(b0, space1);

#ifndef CALC_MESHLET_POS
#define CALC_MESHLET_POS 1
#endif

struct SPackedPayloadElement
{
    uint uint0;
    uint2 position;
};

struct SPayloadElement
{
    uint groupSize;
    uint lod;
    int2 position;
};

struct SPayload
{
    float2 position;
    uint mesletIndexOffset;
    uint tileId;
    uint lod;
#if CALC_MESHLET_POS
    float2 meshletPositions[ TASK_THREADGROUP_SIZE ];
    uint meshletIndices[TASK_THREADGROUP_SIZE];
#endif
};

uint3 UnpackPrimitive(uint primitive)
{
    // Unpacks a 10 bits per index triangle from a 32-bit uint.
    return uint3(primitive & 0x3FF, (primitive >> 10) & 0x3FF, (primitive >> 20) & 0x3FF);
}

uint4 UnpackIndices(uint value)
{
    return uint4(value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF, (value >> 24) & 0xFF);
}

uint2 GroupIDToUint2(uint gid, uint width)
{
    return uint2( gid % width, gid / width );
}

uint PackPayload(uint groupSize, uint lod)
{
    uint ret = ((groupSize & 0xFF) | ((lod << 8)));
    return ret;
}

SPayloadElement UnpackPayloadGroupSizeLod(uint p)
{
    SPayloadElement Out;
    Out.groupSize = (p & 0xFF);
    Out.lod = (p >> 8) & 0xFF;
    //Out.position = p.position;
    return Out;
}

SPayloadElement UnpackPayload(SPayload p, uint id)
{
    SPayloadElement Ret;// = UnpackPayloadGroupSizeLod(p.data[id]);
    Ret.position = p.position;
    return Ret;
}

groupshared SPayload g_Payload;


#define TASK_TG_SIZE (MESHLET_COUNT_PER_ROW*MESHLET_COUNT_PER_ROW)
#define MESHLET_GROUP_PER_ROW_COUNT (MESHLET_COUNT_PER_ROW / TASK_THREADGROUP_SIZE)

uint CalcTileLOD(float2 tilePos2D)
{
    uint ret = 0;
    float3 tilePos = float3(tilePos2D.x, 0, tilePos2D.y);
    float distToCam = distance(tilePos, Camera.position);
    uint factor = clamp(distToCam / 10, 0, 4);
    ret = pow(2, factor);
    return ret;
}

uint2 CalcSubTilePos(uint subTileId)
{
    return uint2(
    (subTileId % SUBTILE_WORKGROUP_SIZE),
    (subTileId / SUBTILE_WORKGROUP_SIZE));

}

bool IsVisible(float2 meshletPos, float meshletSize)
{
    float4 center = float4(
        meshletPos.x + meshletSize * 0.5,
        0,
        meshletPos.y - meshletSize * 0.5,
        1 );
    float radius = distance(center, float4(meshletPos.x, 0, meshletPos.y,1));
    for (int i = 0; i < 6; ++i)
    {
        if(dot(center, Camera.FrustumPlanes[i]) < -radius)
        {
            return true;
        }
    }
    return true;
}

[NumThreads(TASK_THREADGROUP_SIZE, 1, 1)]
void TerrainTS(
    in uint gtid : SV_GroupThreadID,
    in uint dtid : SV_DispatchThreadID,
    in uint gid : SV_GroupID )
{
    uint groupSize = TASK_THREADGROUP_SIZE;
    uint meshId = gtid.x;

    uint subTileId = gid / SUBTILE_WORKGROUP_SIZE;
    uint2 subTilePos = CalcSubTilePos(subTileId);
    uint lodFactor = pow(2, subTileId); //CalcTileLOD( g_Payload.position );
    const uint subTileGroupSize = SUBTILE_WORKGROUP_SIZE / lodFactor;
    const uint totalSubTileGroups = subTileGroupSize * 3; // subtile count
    subTileId = gid / subTileGroupSize + 0;
    const uint subTileGroupId = gid % subTileGroupSize;
    uint meshletGroupId = gid % subTileGroupSize;
    uint meshletId = gtid;
    uint globalMeshletId = dtid % SUBTILE_MESHLET_COUNT;
    
    
    // Tile position
    g_Payload.position = float2(subTilePos.x, subTilePos.y) * SUBTILE_SIZE;
    g_Payload.mesletIndexOffset = TASK_THREADGROUP_SIZE * meshletGroupId;
    g_Payload.tileId = subTileId;
    g_Payload.lod = lodFactor;
    groupSize /= lodFactor;
    const float meshletSize = MESHLET_SIZE * lodFactor;
#if CALC_MESHLET_POS
    float2 meshletGroupBasePos = 0;
    uint2 basePosIndex = 0;
    //if (gid < totalSubTileGroups)
    bool visible = false;
    if(true)
    {
        const float meshletDistance = MESHLET_DISTANCE * lodFactor;
        const float xOffset = TASK_THREADGROUP_SIZE * MESHLET_DISTANCE;
        const float yOffset = -SUBTILE_SIZE;
        basePosIndex.x = (subTileGroupId % (MESHLET_GROUP_PER_ROW_COUNT));
        basePosIndex.y = ((subTileGroupId) / MESHLET_GROUP_PER_ROW_COUNT);
        meshletGroupBasePos.x = xOffset * basePosIndex.x;
        meshletGroupBasePos.y = basePosIndex.y * meshletDistance + meshletDistance;
        const float2 meshletPos = float2(
            meshletGroupBasePos.x + (meshletId * meshletDistance) + g_Payload.position.x,
            meshletGroupBasePos.y + (yOffset) + g_Payload.position.y);
        g_Payload.meshletPositions[meshletId] = meshletPos;
        g_Payload.meshletIndices[meshletId] = globalMeshletId;
        visible = IsVisible(meshletPos, meshletSize);
    }
    else
    {
        groupSize = 0;
    }
    
#endif
    
    DispatchMesh(groupSize, 1, 1, g_Payload);
}

#define VC 64
#define PC 98

[NumThreads(MESH_THREADGROUP_SIZE, 1, 1)]
[OutputTopology("triangle")]
void TerrainMS(
    uint gtid : SV_GroupThreadID,
    in uint gid : SV_GroupID,
    in uint gidx : SV_GroupIndex,
    in uint dtid : SV_DispatchThreadID,
    in payload SPayload PackedPayload,
    out indices uint3 tris[TOTAL_PRIMITIVE_COUNT],
    out vertices VertexOut verts[TOTAL_VERTEX_COUNT]
)
{
    SetMeshOutputCounts(TOTAL_VERTEX_COUNT, TOTAL_PRIMITIVE_COUNT);
    
    const uint vertexLoop = (TOTAL_VERTEX_COUNT + MESH_THREADGROUP_SIZE - 1) / MESH_THREADGROUP_SIZE;
    const uint primLoop = (TOTAL_PRIMITIVE_COUNT + MESH_THREADGROUP_SIZE - 1) / MESH_THREADGROUP_SIZE;
    const uint meshletId = gid + PackedPayload.mesletIndexOffset;
    
    //SPayloadElement Payload = UnpackPayload(PackedPayload, meshletId);
    
    //Debug[dtid] = uint4(PackedPayload.position.x, PackedPayload.position.y, 0, 0);
    
    const uint lod = PackedPayload.lod;
    const float msize = (VERTEX_COUNT_PER_ROW - 1) * VERTEX_DISTANCE;
    const float2 meshletSize = float2(msize, msize) * lod;
    const float2 tilePosition = PackedPayload.position;
    Meshlet Mesh = Meshlets[meshletId];
    float2 meshletPos = float2(0, 0);
    uint globalMeshletId = 0;
    
#if CALC_MESHLET_POS
    meshletPos = PackedPayload.meshletPositions[gid];
    globalMeshletId = PackedPayload.meshletIndices[gid];
#else
    meshletPos = Mesh.position;
#endif

    const float2 vertexDistance = VERTEX_DISTANCE * float2(lod, lod);

    // left, right, top, bottom
    const uint4 isBorder = uint4(
        globalMeshletId % MESHLET_COUNT_PER_ROW == 0,
        globalMeshletId % MESHLET_COUNT_PER_ROW == MESHLET_COUNT_PER_ROW-1,
        globalMeshletId / MESHLET_COUNT_PER_ROW == 0,
        globalMeshletId / MESHLET_COUNT_PER_ROW == MESHLET_COUNT_PER_ROW-1);
    
    //if (meshletId == 0)
    //{
    //    Debug[meshletId + PackedPayload.tileId] = float4(
    //    meshletPos.y,
    //    meshletPos.x + meshletSize.x,
    //    posMin.y,
    //    posMax.y);
    //}
    //meshletPos += tilePosition * 1;
    
    //Debug[gid] = float4(meshletPos.x, meshletPos.y, tilePosition.x, tilePosition.y);
    
    bool aVertexMultipliers[VERTEX_COUNT_PER_ROW];
    for (int i = 1; i < VERTEX_COUNT_PER_ROW-1; ++i)
    {
        aVertexMultipliers[i] = 0;
    }
    aVertexMultipliers[0] = aVertexMultipliers[VERTEX_COUNT_PER_ROW - 1] = 1;
    
    for (uint pid = 0; pid < primLoop; ++pid)
    {
        uint id = gtid + pid * MESH_THREADGROUP_SIZE;
        if (id < TOTAL_PRIMITIVE_COUNT)
        {
        
            tris[id] = UnpackPrimitive(Triangles[id]);
        }
    }
    for (int vid = 0; vid < vertexLoop; ++vid)
    {
        uint id = gtid + vid * MESH_THREADGROUP_SIZE;
        if (id < TOTAL_VERTEX_COUNT)
        {
            uint4 idxValues = UnpackIndices(TileData[id].indices);
            float2 vPos = Vertices[id].Position.xy;
            const float positiveY = -vPos.y;
            const uint2 isOddPosition = uint2(
                vPos.x % 2 == 1,
                positiveY % 2 == 1);
            if (any(isBorder))
            {
                // Positions are calculated:
                // 0 - top vertex
                // -N - bottom vertex
                // Therefore we have to convert [-N, 0] range to [0, N]
                vPos.x += aVertexMultipliers[positiveY] * 1 * isOddPosition.x;
                vPos.y += aVertexMultipliers[vPos.x] * 1 * isOddPosition.y;
            }
            vPos = vPos * vertexDistance;
            float2 pos = (meshletPos + vPos);
            
            verts[id].PositionHS = mul(Camera.mtxViewProj, float4(pos.x, 0, pos.y, 1));
            //verts[id].PositionHS = float4(-1, 1,0,0) + float4(pos.x, pos.y, 0, 1);
            verts[id].Color = Mesh.Color.rgb;
        }
    }
}