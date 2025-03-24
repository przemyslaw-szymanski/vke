//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#define ROOT_SIG "CBV(b0), \
                  RootConstants(b1, num32bitconstants=2), \
                  SRV(t0), \
                  SRV(t1), \
                  SRV(t2), \
                  SRV(t3)"

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

struct SCamera
{
    float4x4 mtxViewProj;
    float3 position;
    float pad1;
    float3 direction;
    float pad2;
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
    
    
    
    // Tile position
    g_Payload.position = float2(subTilePos.x, subTilePos.y) * SUBTILE_SIZE;
    g_Payload.mesletIndexOffset = TASK_THREADGROUP_SIZE * meshletGroupId;
    g_Payload.tileId = subTileId;
    g_Payload.lod = lodFactor;
    groupSize /= lodFactor;
#if CALC_MESHLET_POS
    float2 meshletGroupBasePos = 0;
    uint2 basePosIndex = 0;
    //if (gid < totalSubTileGroups)
    if(true)
    {
        const float meshletDistance = MESHLET_DISTANCE * lodFactor;
        const float xOffset = TASK_THREADGROUP_SIZE * MESHLET_DISTANCE;
        const float yOffset = -SUBTILE_SIZE;
        basePosIndex.x = (subTileGroupId % (MESHLET_GROUP_PER_ROW_COUNT));
        basePosIndex.y = ((subTileGroupId) / MESHLET_GROUP_PER_ROW_COUNT);
        meshletGroupBasePos.x = xOffset * basePosIndex.x;
        meshletGroupBasePos.y = basePosIndex.y * meshletDistance + meshletDistance;
        g_Payload.meshletPositions[meshletId] = float2(
            meshletGroupBasePos.x + (meshletId * meshletDistance) + g_Payload.position.x,
            meshletGroupBasePos.y + (yOffset) + g_Payload.position.y
        );
    }
    else
    {
        groupSize = 0;
    }
    //if (gid==0 || gid == 128)
    //{
    //    Debug[gid/128] = float4(
    //g_Payload.meshletPositions[0].x,
    //g_Payload.meshletPositions[0].y,
    //g_Payload.position.x,
    //meshletGroupBasePos.y);
    //}
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
    const float2 meshletSize = float2(MESHLET_SIZE, MESHLET_SIZE);
    const uint lod = PackedPayload.lod;
    const float2 tilePosition = PackedPayload.position;
    Meshlet Mesh = Meshlets[meshletId];
    float2 meshletPos = float2(0, 0);
    
#if CALC_MESHLET_POS
    meshletPos = PackedPayload.meshletPositions[gid];
#else
    meshletPos = Mesh.position;
#endif
    if(meshletId == 0)
    {
        Debug[meshletId + PackedPayload.tileId] = float4(
        meshletPos.x,
        meshletPos.y,
        tilePosition.x,
        tilePosition.y);
    }
    //meshletPos += tilePosition * 1;
    
    //Debug[gid] = float4(meshletPos.x, meshletPos.y, tilePosition.x, tilePosition.y);
    
    for (uint i = 0; i < primLoop; ++i)
    {
        uint id = gtid + i * MESH_THREADGROUP_SIZE;
        if (id < TOTAL_PRIMITIVE_COUNT)
        {
        
            tris[id] = UnpackPrimitive(Triangles[id]);
        }
    }
    for (i = 0; i < vertexLoop; ++i)
    {
        uint id = gtid + i * MESH_THREADGROUP_SIZE;
        if (id < TOTAL_VERTEX_COUNT)
        {
            uint4 idxValues = UnpackIndices(TileData[id].indices);
            float2 vPos = Vertices[id].Position.xy * VERTEX_DISTANCE * float2(lod, lod);
            float2 pos = (meshletPos + vPos);
            
            verts[id].PositionHS = mul(Camera.mtxViewProj, float4(pos.x, 0, pos.y, 1));
            //verts[id].PositionHS = float4(-1, 1,0,0) + float4(pos.x, pos.y, 0, 1);
            verts[id].Color = Mesh.Color.rgb;
        }
    }
}