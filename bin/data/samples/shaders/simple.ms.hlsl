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
    //float3 PositionVS : POSITION0;
    //float3 Normal : NORMAL0;
    //uint MeshletIndex : COLOR0;
};

[NumThreads(4, 1, 1)]
[OutputTopology("triangle")]
void main(
    uint gtid : SV_GroupThreadID,
    uint gid : SV_GroupID,
    out indices uint3 tris[2],
    out vertices VertexOut verts[4]
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
    
    SetMeshOutputCounts(4, 2);

    if(gtid < 2)
    {
        tris[gtid] = aIndices[gtid];
    }
    if (gtid < 4)
    {
        verts[gtid].PositionHS = aVerts[gtid];
    }
}
