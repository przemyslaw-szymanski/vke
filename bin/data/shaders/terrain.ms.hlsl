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
    float4 Position;
    //float2 Texcoords;
};

struct Meshlet
{
    float2 Position;
    float pad1;
    float pad2;
    float3 Color;
    float pad3;
};

struct Triangle
{
    uint3 v;
};

StructuredBuffer<Vertex> Vertices : register(t0, space0);
//StructuredBuffer<uint> Indices : register(t1, space0);
StructuredBuffer<Triangle> Triangles : register(t1, space0);
StructuredBuffer<Meshlet> Meshlets : register(t2, space0);

[NumThreads(98, 1, 1)]
[OutputTopology("triangle")]
void TerrainMS(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out indices uint3 tris[98],
    out vertices VertexOut verts[64]
)
{
    const float4 aVerts[4] =
    {
        float4(-0.5, 0.5, 0, 1),
        float4(0.5, 0.5, 0, 1),
        float4(-0.5, -0.5, 0, 1),
        float4(0.5, -0.5, 0, 1)
    };
    
    const uint3 aIndices[2] =
    {
        uint3( 3,1,0 ),
        uint3( 0, 2, 3 )
    };
    
    SetMeshOutputCounts(64, 18);

    if(gtid < 18)
    {
        tris[gtid] = Triangles[gtid].v;
    }
    if (gtid < 64)
    {
        float2 pos = Vertices[gtid].Position.xy;
        verts[gtid].PositionHS = float4(pos.x, pos.y, 0, 1);
        verts[gtid].Color = Meshlets[gid].Color.rgb;
    }
}
