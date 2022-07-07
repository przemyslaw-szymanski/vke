
#define DEBUG 1

struct SCameraData
{
    float4x4 mtxViewProj;
};

struct SLightData
{
    float3 vec3Position;
    float radius;
    float3 vec3Direction;
    float attenuation;
    float3 vec3Color;
    float pad1;
};


struct SPerFrameConstantBuffer
{
    SCameraData Camera;
    SLightData  MainLight;
};
ConstantBuffer<SPerFrameConstantBuffer> SceneData;

struct SPerFrameTerrainConstantBuffer
{
    float4x4    mtxViewProj;
    SLightData  Light;
    float2      vec2TerrainSize;
    float2      vec2TerrainHeight;
    uint        tileRowVertexCount;
};
ConstantBuffer<SPerFrameTerrainConstantBuffer> FrameData : register(b0, space0);

struct SPerTileConstantBuffer
{
    float4  vec4Position;
    float4  vec4Color;
    float2  f2TexcoordOffset;
    uint    vertexShift;
    float   tileSize;
    uint    textureIdx;
    uint    topVertexShift; // vertex move to highest lod to create stitches
    uint    bottomVertexShift;
    uint    leftVertexShift;
    uint    rightVertexShift;
    uint    heightmapIndex;
};
ConstantBuffer<SPerTileConstantBuffer> TileData : register(b0, space1);


//Texture2D HeightmapTexture : register(t1, space1);
Texture2D HeightmapTextures[] : register(t1, space1);
Texture2D HeightmapNormalTexture : register(t2, space1);

//layout(location = 0) in float3 iPosition;
struct SVertexShaderInput
{
    float3 f3Position : SV_Position;
};
//layout(location = 0) out float4 oColor;
//layout(location = 1) out float2 oTexcoord;
struct SPixelShaderInput
{
    float4  f4Position : SV_Position;
    float4  f4Color : COLOR0;
    float3  f3Position : POSITION1;
    float3  f3Normal : NORMAL0;
    float2  f2Texcoord : TEXCOORD0;
};

struct SVertexShift
{
    uint top;
    uint bottom;
    uint left;
    uint right;
};

SVertexShift UnpackVertexShift(uint vertexShift)
{
    SVertexShift s;
    s.top = (vertexShift >> 24u) & 0xFF;
    s.bottom = (vertexShift >> 16u) & 0xFF;
    s.left = (vertexShift >> 8u) & 0xFF;
    s.right = (vertexShift) & 0xFF;
    return s;
}

float SampleToRange(float v, float2 vec2Range)
{
    float range = vec2Range.y * 2 - vec2Range.x * 2;
    return vec2Range.x * 2 + v * range;
}

uint2 VertexIndexToPosition(uint vertexIndex, uint tileVertexCount)
{
    return uint2( vertexIndex % tileVertexCount, vertexIndex / tileVertexCount );
}
//#define BASE_TILE_SIZE = 32.0;
//#define TILE_VERTEX_COUNT = 32.0;
//#define BASE_VERTEX_DISTANCE 0.5
// For vertex distance smaller than 1 it is need to increase vertex position
static const float BASE_VERTEX_DISTANCE_MULTIPLIER = (1.0 / BASE_VERTEX_DISTANCE);

float3 CalcStitches(float3 iPos, SVertexShift Shift)
{
    float3 f3Ret = iPos;
    if (iPos.z == 0.0f && Shift.top > 0)
    {
        f3Ret.x -= iPos.x % (Shift.top / BASE_VERTEX_DISTANCE_MULTIPLIER);
    }
    else if (iPos.z == -BASE_TILE_SIZE && Shift.bottom > 0)
    {
        f3Ret.x -= iPos.x % (Shift.bottom / BASE_VERTEX_DISTANCE_MULTIPLIER);
    }
    if (iPos.x == 0.0f && Shift.left > 0)
    {
        f3Ret.z -= iPos.z % (Shift.left / BASE_VERTEX_DISTANCE_MULTIPLIER);
    }
    else if (iPos.x == BASE_TILE_SIZE && Shift.right > 0)
    {
        f3Ret.z -= iPos.z % (Shift.right / BASE_VERTEX_DISTANCE_MULTIPLIER);
    }
    return f3Ret;
}

int2 CalcUnnormalizedTexcoords(float3 f3Pos, uint2 u2TextureSize)
{
    const uint2 u2HalfSize = u2TextureSize * 0.5;
    return int2(f3Pos.x + u2HalfSize.x, f3Pos.z + u2HalfSize.y);
}

float2 CalcNormalizedTexcoords(int2 i2UnnormalizedTexcoords, uint2 u2TextureSize)
{
    return float2( i2UnnormalizedTexcoords ) / float2( u2TextureSize );
}

float3 CalcVertexPositionXZ(float3 f3Pos, uint uTileSize, float3 f3CornerPosition)
{
    float3 f3Ret = f3Pos;
    // There is only one vertex buffer used with the highest lod.
    // Highest lod is the smallest, most dense drawcall
    // tileRowVertexCount is configured in an app (TerrainDesc)
    float vertexDistance = (uTileSize / TILE_VERTEX_COUNT) * BASE_VERTEX_DISTANCE_MULTIPLIER;
    // For each lod vertex position must be scaled by vertex distance
    f3Ret *= vertexDistance;
    // Move to a desired position
    f3Ret += f3CornerPosition;
    return f3Ret;
}

float CalcPositionY(int3 i3UnnormalizedTexcoords, float2 f2HeightMinMax, Texture2D Heightmap)
{
    float fHeight = Heightmap.Load( i3UnnormalizedTexcoords ).r;
    float fRet = SampleToRange(fHeight, f2HeightMinMax);
    //return 0;
    return fRet;
}

int3 CalcTexcoords(float3 f3VertexPos, float2 f2TextureOffset, float tileSize)
{
    // Convert object space position to texture space
    // Terrain tile vertices are placed along negative Z
    // Negative Z in object space in a positive Y in texture space
    float step = tileSize / BASE_TILE_SIZE;
    float2 f2Offset = float2( f3VertexPos.x, -f3VertexPos.z ) * step + (f2TextureOffset + float2(0,0));
    return int3(f2Offset,0);
}

void CalcPosition(float3 f3Pos, Texture2D Heightmap, out float3 f3PosOut, out int3 i3TC)
{
    i3TC = CalcTexcoords( f3Pos, TileData.f2TexcoordOffset, TileData.tileSize );
    f3PosOut = CalcVertexPositionXZ( f3Pos, uint( TileData.tileSize ), TileData.vec4Position.xyz );

    f3PosOut.y = CalcPositionY( i3TC, FrameData.vec2TerrainHeight, Heightmap );
    //return f3Pos;
}

void CalcPosition(float3 f3Pos, int3 i3Texcoords, Texture2D Heightmap, out float3 f3PosOut )
{
    //int3 i3TcOut = CalcTexcoords( f3Pos, TileData.f2TexcoordOffset, TileData.tileSize );
    //f3PosOut = CalcVertexPositionXZ( f3Pos, uint( TileData.tileSize ), TileData.vec4Position.xyz );
    f3PosOut = f3Pos;
    f3PosOut.y = CalcPositionY( i3Texcoords, FrameData.vec2TerrainHeight, Heightmap );
}

int2 Clamp(int2 min, int2 max, int2 v)
{
    int2 ret;
    ret.x = clamp(min.x, max.x, v.x);
    ret.y = clamp(min.y, max.y, v.y);
    return ret;
}

#define CENTER          0
#define LEFT            1
#define LEFT_TOP        2
#define TOP             3
#define RIGHT_TOP       4
#define RIGHT           5
#define LEFT_BOTTOM     6
#define BOTTOM          7
#define RIGHT_BOTTOM    8

struct SPositions
{
    int3    i3CenterTC;
    float3  f3Center;
    int3    i3LeftTC;
    float3  f3Left;
    int3    i3RightTC;
    float3  f3Right;
    int3    i3LeftTopTC;
    float3  f3LeftTop;
    int3    i3RightBottomTC;
    float3  f3RightBottom;
    int3    i3TopTC;
    float3  f3Top;
    int3    i3BottomTC;
    float3  f3Bottom;
};

struct SPositions2
{
    float3 f3Positions[9];
#if DEBUG
    int3   i3Texcoords[9];
    float4 f4Color;
#endif
};

float3 CalcNormal(float3 f3V1, float3 f3V2, float3 f3V3)
{
    float3 f3Ret;
    /*float3 f3U = f3V2 - f3V1;
    float3 f3V = f3V3 - f3V1;
    f3Ret.x = f3U.y * f3V.z - f3U.z * f3V.y;
    f3Ret.y = f3U.z * f3V.x - f3U.x * f3V.z;
    f3Ret.z = f3U.x * f3V.y - f3U.y * f3V.x;*/
    //if( f3Ret.y < 0 ) f3Ret.y *= -1.0;
    f3Ret = cross( f3V1 - f3V2, f3V3 - f3V1 );
    return normalize( f3Ret );
}



int3 CalcTexOffset(float3 f3VertexPos, float step, float2 f2TextureOffset)
{
    return int3( float2( f3VertexPos.x, -f3VertexPos.z ) * step + f2TextureOffset, 0 );
}

float3 CalcNormal(SPositions2 Positions, float3 f3ObjSpaceCenterPos)
{
    float3 f3Center = Positions.f3Positions[ CENTER ];
    float3 f3Ret = float3(0,0,0);
    // f3Ret = CalcNormal( f3Center, Positions.f3Positions[ TOP ], Positions.f3Positions[ LEFT_TOP ] );
    // f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT_TOP ], Positions.f3Positions[ LEFT ] );
    // f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT ], Positions.f3Positions[ LEFT_BOTTOM ] );
    // f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT_BOTTOM ], Positions.f3Positions[ BOTTOM ] );
    // f3Ret += CalcNormal( f3Center, Positions.f3Positions[ BOTTOM ], Positions.f3Positions[ RIGHT_BOTTOM ] );
    // f3Ret += CalcNormal( f3Center, Positions.f3Positions[ RIGHT_BOTTOM ], Positions.f3Positions[ RIGHT ] );
    // f3Ret += CalcNormal( f3Center, Positions.f3Positions[ RIGHT ], Positions.f3Positions[ RIGHT_TOP ] );
    // f3Ret += CalcNormal( f3Center, Positions.f3Positions[ RIGHT_TOP ], Positions.f3Positions[ TOP ] );
    // const int div = 8;

    f3Ret = CalcNormal( f3Center, Positions.f3Positions[ TOP ], Positions.f3Positions[ LEFT_TOP ] );
    f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT_TOP ], Positions.f3Positions[ LEFT ] );
    f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT ], Positions.f3Positions[ BOTTOM ] );
    f3Ret += CalcNormal( f3Center, Positions.f3Positions[ BOTTOM ], Positions.f3Positions[ RIGHT_BOTTOM ] );
    f3Ret += CalcNormal( f3Center, Positions.f3Positions[ RIGHT_BOTTOM ], Positions.f3Positions[ RIGHT ] );
    f3Ret += CalcNormal( f3Center, Positions.f3Positions[ RIGHT ], Positions.f3Positions[ TOP ] );
    const int div = 1;

    f3Ret /= div;

    return normalize( f3Ret );
}
#define PV(v) Ret.f3Positions[v].x, Ret.f3Positions[v].y, Ret.f3Positions[v].z
SPositions2 CalcPositions(float3 f3WorldSpaceCenter, float3 f3ObjSpaceCenter, int3 i3CenterTC, Texture2D Heightmap)
{
    SPositions2 Ret;
    uint2 texSize = uint2(2049, 2049);
    Heightmap.GetDimensions(texSize.x, texSize.y);
    //texSize -= uint2(2,2);
    float vstep = (TileData.tileSize / TILE_VERTEX_COUNT) * BASE_VERTEX_DISTANCE_MULTIPLIER;
    float tstep = TileData.tileSize / BASE_TILE_SIZE;
    //int3 i3Min = MaxVec( i3CenterTC, i3CenterTC - tstep );
    int3 i3Min = i3CenterTC - tstep;
    //int3 i3Max = MinVec( int3( texSize.x-1, texSize.y-1, 0 ), i3CenterTC + tstep );
    int3 i3Max = i3CenterTC + tstep;

    int print = 0; int left = 0; int right = 0;
    if(i3Min.x < 0) { i3Min.x = i3CenterTC.x - 0; print = 0; left = 1; }
    if(i3Min.y < 0) { i3Min.y = i3CenterTC.y - 0; print = 0; }
    if(i3Max.x > texSize.x) { i3Max.x = i3CenterTC.x + 0; print = 0; right = 1; }
    if(i3Max.y > texSize.y) { i3Max.y = i3CenterTC.y + 0; print = 0; }

    //Ret.f3Center = f3WorldSpaceCenter;
    // CalcPosition( f3Center + float3( -vstep, 0, 0 ), i3CenterTC + int3( -tstep, 0, 0 ), Heightmap, Ret.f3Left );
    // CalcPosition( f3Center + float3( -vstep, 0, +vstep ), i3CenterTC + int3( -tstep, -tstep, 0 ), Heightmap, Ret.f3LeftTop );
    // CalcPosition( f3Center + float3( +vstep, 0, 0 ), i3CenterTC + int3( +tstep, 0, 0 ), Heightmap, Ret.f3Right );
    // CalcPosition( f3Center + float3( +vstep, 0, -vstep ), i3CenterTC + int3( +tstep, +tstep, 0 ), Heightmap, Ret.f3RightBottom );
    // CalcPosition( f3Center + float3( 0, 0, +vstep ), i3CenterTC + int3( 0, -tstep, 0 ), Heightmap, Ret.f3Top );
    // CalcPosition( f3Center + float3( 0, 0, -vstep ), i3CenterTC + int3( -tstep, +tstep, 0 ), Heightmap, Ret.f3Bottom );
    ///

    
    Ret.f3Positions[CENTER] = f3WorldSpaceCenter;
    CalcPosition( f3WorldSpaceCenter + float3( 0, 0, +vstep ), int3( i3CenterTC.x, i3Min.y, 0 ), Heightmap, Ret.f3Positions[ TOP ] );
    CalcPosition( f3WorldSpaceCenter + float3( 0, 0, -vstep ), int3( i3CenterTC.x, i3Max.y, 0 ), Heightmap, Ret.f3Positions[ BOTTOM ] );
    CalcPosition( f3WorldSpaceCenter + float3( -vstep, 0, 0 ), int3( i3Min.x, i3CenterTC.y, 0 ), Heightmap, Ret.f3Positions[ LEFT ] );
    CalcPosition( f3WorldSpaceCenter + float3( +vstep, 0, 0 ), int3( i3Max.x, i3CenterTC.y, 0 ), Heightmap, Ret.f3Positions[ RIGHT ] );
    CalcPosition( f3WorldSpaceCenter + float3( -vstep, 0, +vstep ), int3( i3Min.x, i3Min.y, 0 ), Heightmap, Ret.f3Positions[ LEFT_TOP ] );
    CalcPosition( f3WorldSpaceCenter + float3( +vstep, 0, +vstep ), int3( i3Max.x, i3Min.y, 0 ), Heightmap, Ret.f3Positions[ RIGHT_TOP ] );
    CalcPosition( f3WorldSpaceCenter + float3( -vstep, 0, -vstep ), int3( i3Min.x, i3Max.y, 0 ), Heightmap, Ret.f3Positions[ LEFT_BOTTOM ] );
    CalcPosition( f3WorldSpaceCenter + float3( +vstep, 0, -vstep ), int3( i3Max.x, i3Max.y, 0 ), Heightmap, Ret.f3Positions[ RIGHT_BOTTOM ] );

#if DEBUG
    Ret.f4Color = float4(0.1,0.2,0.3,0.4);
    if(left == 1)
        Ret.f4Color = Heightmap.Load( int3( i3Min.x, i3CenterTC.y, 0 ) );
    else if(right ==1)
        Ret.f4Color = Heightmap.Load( int3( i3Max.x, i3CenterTC.y, 0 ) );
    else
        Ret.f4Color = Heightmap.Load( int3( i3CenterTC.x, i3CenterTC.y, 0 ) );
#endif
    return Ret;
}

float3 CalcNormal2(float3 f3WorldSpaceCenter, float3 f3ObjSpaceCenter, int3 i3CenterTC, Texture2D Heightmap)
{
    SPositions2 Positions;
    uint2 texSize = uint2(2049, 2049);
    Heightmap.GetDimensions(texSize.x, texSize.y);
    //texSize -= uint2(2,2);
    float vstep = (TileData.tileSize / TILE_VERTEX_COUNT) * BASE_VERTEX_DISTANCE_MULTIPLIER;
    float tstep = TileData.tileSize / BASE_TILE_SIZE;
    //int3 i3Min = MaxVec( i3CenterTC, i3CenterTC - tstep );
    int3 i3Min = i3CenterTC - tstep;
    //int3 i3Max = MinVec( int3( texSize.x-1, texSize.y-1, 0 ), i3CenterTC + tstep );
    int3 i3Max = i3CenterTC + tstep;

    int print = 0; int2 edges = int2(0,0);
    if(i3Min.x < 0) { i3Min.x = i3CenterTC.x - 0; print = 0; edges.x = -1; }
    if(i3Min.y < 0) { i3Min.y = i3CenterTC.y - 0; print = 0; edges.y = 1; }
    if(i3Max.x >= texSize.x) { i3Max.x = i3CenterTC.x + 0; print = 0; edges.x = 1; }
    if(i3Max.y >= texSize.y) { i3Max.y = i3CenterTC.y + 0; print = 0; edges.y = -1; }
    edges = int2(0,0);

    Positions.f3Positions[CENTER] = f3WorldSpaceCenter;
    CalcPosition( f3WorldSpaceCenter + float3( 0, 0, +vstep ), int3( i3CenterTC.x, i3Min.y, 0 ), Heightmap, Positions.f3Positions[ TOP ] );
    CalcPosition( f3WorldSpaceCenter + float3( 0, 0, -vstep ), int3( i3CenterTC.x, i3Max.y, 0 ), Heightmap, Positions.f3Positions[ BOTTOM ] );
    CalcPosition( f3WorldSpaceCenter + float3( -vstep, 0, 0 ), int3( i3Min.x, i3CenterTC.y, 0 ), Heightmap, Positions.f3Positions[ LEFT ] );
    CalcPosition( f3WorldSpaceCenter + float3( +vstep, 0, 0 ), int3( i3Max.x, i3CenterTC.y, 0 ), Heightmap, Positions.f3Positions[ RIGHT ] );
    CalcPosition( f3WorldSpaceCenter + float3( -vstep, 0, +vstep ), int3( i3Min.x, i3Min.y, 0 ), Heightmap, Positions.f3Positions[ LEFT_TOP ] );
    CalcPosition( f3WorldSpaceCenter + float3( +vstep, 0, +vstep ), int3( i3Max.x, i3Min.y, 0 ), Heightmap, Positions.f3Positions[ RIGHT_TOP ] );
    CalcPosition( f3WorldSpaceCenter + float3( -vstep, 0, -vstep ), int3( i3Min.x, i3Max.y, 0 ), Heightmap, Positions.f3Positions[ LEFT_BOTTOM ] );
    CalcPosition( f3WorldSpaceCenter + float3( +vstep, 0, -vstep ), int3( i3Max.x, i3Max.y, 0 ), Heightmap, Positions.f3Positions[ RIGHT_BOTTOM ] );

    float3 f3Ret = float3(1,1,1);
    if(edges.x == 0 && edges.y == 0)
    {
        f3Ret = CalcNormal(Positions, f3ObjSpaceCenter);
    }
    else
    {
        if(edges.x < 0) // left edge
        {
            if(edges.y < 0) // left bottom
            {
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ TOP ], Positions.f3Positions[ LEFT_TOP ] );
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT_TOP ], Positions.f3Positions[ LEFT ] );
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT ], Positions.f3Positions[ LEFT_BOTTOM ] );
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT_BOTTOM ], Positions.f3Positions[ BOTTOM ] );
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ BOTTOM ], Positions.f3Positions[ RIGHT_BOTTOM ] );
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ RIGHT_BOTTOM ], Positions.f3Positions[ RIGHT ] );
                f3Ret = CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ RIGHT_TOP ], Positions.f3Positions[ RIGHT ] );
                //f3Ret = CalcNormal( f3Center, Positions.f3Positions[ RIGHT_TOP ], Positions.f3Positions[ TOP ] );
            }
            else if(edges.y > 0) // left top
            {
                f3Ret = CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ BOTTOM ], Positions.f3Positions[ RIGHT_BOTTOM ] );
            }
            else // left edge
            {
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ TOP ], Positions.f3Positions[ LEFT_TOP ] );
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT_TOP ], Positions.f3Positions[ LEFT ] );
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT ], Positions.f3Positions[ LEFT_BOTTOM ] );
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ LEFT_BOTTOM ], Positions.f3Positions[ BOTTOM ] );
                f3Ret = CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ BOTTOM ], Positions.f3Positions[ RIGHT_BOTTOM ] );
                f3Ret += CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ RIGHT_BOTTOM ], Positions.f3Positions[ RIGHT ] );
                f3Ret += CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ RIGHT_TOP ], Positions.f3Positions[ RIGHT ] );
                f3Ret /= 3;
                f3Ret = normalize(f3Ret);
                //f3Ret += CalcNormal( f3Center, Positions.f3Positions[ RIGHT_TOP ], Positions.f3Positions[ TOP ] );
            }
        }
        else // right edge
        {
            if(edges.y < 0) // right bottom
            {
                f3Ret = CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ LEFT_TOP ], Positions.f3Positions[ TOP ] );
                f3Ret += CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ LEFT ], Positions.f3Positions[ LEFT_TOP ] );
                f3Ret = normalize(f3Ret / 2);
            }
            else if(edges.y > 0) // right top
            {
                f3Ret = CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ BOTTOM ], Positions.f3Positions[ RIGHT ] );
            }
            else // right edge
            {
                f3Ret = CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ TOP ], Positions.f3Positions[ LEFT_TOP ] );
                f3Ret += CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ LEFT_TOP ], Positions.f3Positions[ LEFT ] );
                f3Ret += CalcNormal( f3WorldSpaceCenter, Positions.f3Positions[ LEFT ], Positions.f3Positions[ BOTTOM ] );
                f3Ret = normalize(f3Ret / 3);
            }
        }
    }

    return f3Ret;
}

float3 CalcNormal3(float3 f3WorldSpaceCenter, float3 f3ObjSpaceCenter, int3 i3CenterTC, Texture2D Heightmap)
{
    float4 f4Heights;
    float size = FrameData.vec2TerrainHeight.y - FrameData.vec2TerrainHeight.x;
    uint2 texSize = uint2(2049, 2049);
    Heightmap.GetDimensions(texSize.x, texSize.y);
    //texSize -= uint2(2,2);
    float vstep = (TileData.tileSize / TILE_VERTEX_COUNT) * BASE_VERTEX_DISTANCE_MULTIPLIER;
    float tstep = TileData.tileSize / BASE_TILE_SIZE;
    tstep = 1;
    int3 i3Min = i3CenterTC - tstep;
    int3 i3Max = i3CenterTC + tstep;

    /*if(i3Min.x < 0) { i3Min.x = 0; }
    if(i3Min.y < 0) { i3Min.y = 0; }
    if(i3Max.x >= texSize.x) { i3Max.x = texSize.x - 1; }
    if(i3Max.y >= texSize.y) { i3Max.y = texSize.y - 1; }*/

    // f4Heights[0] = Heightmap.Load( i3CenterTC + int3( 0, -1, 0 ) ).r * size;
    // f4Heights[1] = Heightmap.Load( i3CenterTC + int3( -1, 0, 0 ) ).r  * size;
    // f4Heights[2] = Heightmap.Load( i3CenterTC + int3( 1, 0, 0 ) ).r  * size;
    // f4Heights[3] = Heightmap.Load( i3CenterTC + int3( 0, 1, 0 ) ).r  * size;
    f4Heights[0] = Heightmap.Load( int3( i3CenterTC.x, i3Min.y, 0 ) ).r * size;
    f4Heights[1] = Heightmap.Load( int3( i3Min.x, i3CenterTC.y, 0 ) ).r * size;
    f4Heights[2] = Heightmap.Load( int3( i3Max.x, i3CenterTC.y, 0 ) ).r * size;
    f4Heights[3] = Heightmap.Load( int3( i3CenterTC.x, i3Max.y, 0 ) ).r * size;

    float3 f3Normal;
    f3Normal.z = f4Heights[3] - f4Heights[0];
    f3Normal.x = f4Heights[1] - f4Heights[2];
    f3Normal.y = 2;
    return normalize(f3Normal);
}

float3 CalcLight( int3 i3Texcoords, float3 f3PositionWS, inout float3 f3NormalWS)
{
    float3 ret;
    float3 f3Normal = HeightmapNormalTexture.Load( i3Texcoords ).rgb;
    //f4Normal = mul(FrameData.mtxViewProj, f4Normal);
    f3Normal = f3NormalWS;
    float3 lightDir = normalize( FrameData.Light.vec3Position - f3PositionWS );
    
    ret = saturate( dot( lightDir, f3Normal ) );
    f3NormalWS = f3Normal;
    return ret;
}

SPixelShaderInput vs_main(in SVertexShaderInput IN)
{
    SPixelShaderInput OUT;

    float4x4 mtxMVP = FrameData.mtxViewProj;
    //Texture2D Heightmap = HeightmapTextures[TileData.textureIdx];
    Texture2D Heightmap = HeightmapTextures[0];
    //Texture2D Heightmap = HeightmapTexture;
    
    //uint2 texSize;
    //Heightmap.GetDimensions(texSize.x, texSize.y);
    float4 c = float4(1, 1, 1, 1);
    // Vertex shift is packed in order: top, bottom, left, right
    SVertexShift Shift = UnpackVertexShift(TileData.vertexShift);
    Shift.top = TileData.topVertexShift;
    Shift.bottom = TileData.bottomVertexShift;
    Shift.left = TileData.leftVertexShift;
    Shift.right = TileData.rightVertexShift;

    // Calc vertex offset in object space
    float3 f3ObjSpacePos = CalcStitches(IN.f3Position, Shift);

    //iPos = CalcVertexPositionXZ(iPos, TileData.tileSize, TileData.vec4Position.xyz);
    // Calc world space position height
    //iPos.y = CalcPositionY( Positions.i3CenterTC, FrameData.vec2TerrainHeight, Heightmap);
    int3 i3CenterTC;
    float3 f3WorldSpacePos;
    CalcPosition( f3ObjSpacePos, Heightmap, f3WorldSpacePos, i3CenterTC ); 

    // Calc texture offset in object space
    //SPositions2 Positions = CalcPositions(f3WorldSpacePos, f3ObjSpacePos, i3CenterTC, Heightmap);
    float3 f3Normal = CalcNormal3( f3WorldSpacePos, f3ObjSpacePos, i3CenterTC, Heightmap );

    // Calc world projection space position
    OUT.f4Position = mul(mtxMVP, float4(f3WorldSpacePos, 1.0));
    OUT.f3Position = f3WorldSpacePos;
    //OUT.f2Texcoord = float2( float2(Positions.i3CenterTC.xy) / texSize );
    OUT.f2Texcoord = float2( i3CenterTC.xy ) / float2(2049, 2049);
    
    //OUT.f3Normal = CalcNormal( Positions, f3ObjSpacePos );
    OUT.f3Normal = f3Normal;
    
    OUT.f4Color.rgb = CalcLight( i3CenterTC, OUT.f3Position, OUT.f3Normal );
    #if DEBUG
        OUT.f4Color *= TileData.vec4Color;
    #endif

    return OUT;
}

struct SHullShaderInput
{
    float4  f4Color : COLOR0;
    float3  f3PositionWS : POSITION0;
    float3  f3Normal : NORMAL0;
    uint2   i2Texcoords : TEXCOORD0;
};

struct SDomainShaderInput
{
    float3  f3PositionWS : POSITION0;
};

SHullShaderInput vs_main_tess(in SVertexShaderInput IN)
{
    SHullShaderInput OUT;

    float4x4 mtxMVP = FrameData.mtxViewProj;
    //Texture2D Heightmap = HeightmapTextures[TileData.textureIdx];
    Texture2D Heightmap = HeightmapTextures[0];
    //Texture2D Heightmap = HeightmapTexture;
    
    //uint2 texSize;
    //Heightmap.GetDimensions(texSize.x, texSize.y);
    float4 c = float4(1, 1, 1, 1);
    // Vertex shift is packed in order: top, bottom, left, right
    SVertexShift Shift = UnpackVertexShift(TileData.vertexShift);
    Shift.top = TileData.topVertexShift;
    Shift.bottom = TileData.bottomVertexShift;
    Shift.left = TileData.leftVertexShift;
    Shift.right = TileData.rightVertexShift;

    // Calc vertex offset in object space
    float3 f3ObjSpacePos = CalcStitches(IN.f3Position, Shift);

    //iPos = CalcVertexPositionXZ(iPos, TileData.tileSize, TileData.vec4Position.xyz);
    // Calc world space position height
    //iPos.y = CalcPositionY( Positions.i3CenterTC, FrameData.vec2TerrainHeight, Heightmap);
    int3 i3CenterTC;
    float3 f3WorldSpacePos;
    CalcPosition( f3ObjSpacePos, Heightmap, f3WorldSpacePos, i3CenterTC ); 

    // Calc texture offset in object space
    //SPositions2 Positions = CalcPositions(f3WorldSpacePos, f3ObjSpacePos, i3CenterTC, Heightmap);
    float3 f3Normal = CalcNormal3( f3WorldSpacePos, f3ObjSpacePos, i3CenterTC, Heightmap );

    // Calc world projection space position
    //OUT.f4Position = mul(mtxMVP, float4(f3WorldSpacePos, 1.0));
    OUT.f3PositionWS = f3WorldSpacePos;
    //OUT.f2Texcoord = float2( float2(Positions.i3CenterTC.xy) / texSize );
    //OUT.f2Texcoord = float2( i3CenterTC.xy ) / float2(2049, 2049);
    
    //OUT.f3Normal = CalcNormal( Positions, f3ObjSpacePos );
    OUT.f3Normal = f3Normal;
    OUT.i2Texcoords = i3CenterTC.xy;
    
    OUT.f4Color.rgb = CalcLight( i3CenterTC, OUT.f3PositionWS, OUT.f3Normal );
    #if DEBUG
        OUT.f4Color *= TileData.vec4Color;
    #endif

    return OUT;
}

struct STrianglePatchConstants
{
    float edgeFactors[3] : SV_TessFactor;
    float insideFactor : SV_InsideTessFactor;
    float4 f4Color : COLOR0;
    float3 f3Normal : NORMAL0;
    int2 i2Texcoords : TEXCOORD0;
};
struct SQuadPatchConstants
{
    float edgeFactors[4] : SV_TessFactor;
    float insideFactors[2] : SV_InsideTessFactor;
    float4 f4Color : COLOR0;
    float3 f3Normal : NORMAL0;
    int2 i2Texcoords : TEXCOORD0;
};

#if CONTROL_POINTS == 3

STrianglePatchConstants hs_PatchConstantFunc(InputPatch< SHullShaderInput, 3 > Patch, uint patchId : SV_PrimitiveID )
{
    STrianglePatchConstants OUT;
    float factor = 2;
    OUT.edgeFactors[0] = factor;
    OUT.edgeFactors[1] = factor;
    OUT.edgeFactors[2] = factor;
    OUT.insideFactor = factor;
    OUT.f4Color = Patch[ patchId ].f4Color;
    OUT.f3Normal = Patch[ patchId ].f3Normal;
    OUT.i2Texcoords = Patch[ patchId ].i2Texcoords;
    return OUT;
}
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("hs_PatchConstantFunc")]
SDomainShaderInput hs_main(in InputPatch< SHullShaderInput, 3 > Patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    SDomainShaderInput OUT;
    OUT.f3PositionWS = Patch[ pointId ].f3PositionWS;
    // OUT.f4Color = Patch[ pointId ].f4Color;
    // OUT.f3Normal = Patch[ pointId ].f3Normal;
    // OUT.i2Texcoord = Patch[pointId].i2Texcoord;

    return OUT;
}
[domain("tri")]
SPixelShaderInput ds_main(
    in STrianglePatchConstants Constants,
    float3 f3Coords : SV_DomainLocation,
    const OutputPatch< SDomainShaderInput, 3 > TrianglePatch)
{
    SPixelShaderInput OUT;
    float3 f3PositionWS = f3Coords.x * TrianglePatch[0].f3PositionWS +
        f3Coords.y * TrianglePatch[1].f3PositionWS +
        f3Coords.z * TrianglePatch[2].f3PositionWS;

    OUT.f4Position = mul(FrameData.mtxViewProj, float4(f3PositionWS, 1.0));
    OUT.f4Color = Constants.f4Color;

    return OUT;
}
#elif CONTROL_POINTS == 4

SQuadPatchConstants hs_PatchConstantFunc(InputPatch< SHullShaderInput, 4 > Patch, uint patchId : SV_PrimitiveID )
{
    SQuadPatchConstants OUT;
    float factor = 2;
    OUT.edgeFactors[0] = factor;
    OUT.edgeFactors[1] = factor;
    OUT.edgeFactors[2] = factor;
    OUT.edgeFactors[3] = factor;
    OUT.insideFactors[0] = factor;
    OUT.insideFactors[1] = factor;
    OUT.f4Color = Patch[ patchId ].f4Color;
    OUT.f3Normal = Patch[ patchId ].f3Normal;
    OUT.i2Texcoords = Patch[ patchId ].i2Texcoords;
    return OUT;
}
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("hs_PatchConstantFunc")]
SDomainShaderInput hs_main(in InputPatch< SHullShaderInput, 4 > Patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    SDomainShaderInput OUT;
    OUT.f3PositionWS = Patch[ pointId ].f3PositionWS;
    // OUT.f4Color = Patch[ pointId ].f4Color;
    // OUT.f3Normal = Patch[ pointId ].f3Normal;
    // OUT.i2Texcoord = Patch[pointId].i2Texcoord;

    return OUT;
}

float3 Bilerp(float3 v[4], float2 uv)
{
    float3 v1 = lerp(v[0], v[1], uv.x);
    float3 v2 = lerp(v[3], v[2], uv.x);
    return lerp( v1, v2, uv.y );
}

int2 Bilerp(int2 v[4], float2 uv)
{
    int2 v1 = lerp(v[0], v[1], uv.x);
    int2 v2 = lerp(v[3], v[2], uv.x);
    return lerp( v1, v2, uv.y );
}

#define BILERP(Patch, var, UV, output) \
    output = lerp( lerp( Patch[0].var, Patch[1].var, UV.x ), lerp( Patch[3].var, Patch[2].var, UV.x ), UV.y);

[domain("quad")]
SPixelShaderInput ds_main(
    in SQuadPatchConstants Constants,
    float2 f2Coords : SV_DomainLocation,
    const OutputPatch< SDomainShaderInput, 4 > QuadPatch)
{
    SPixelShaderInput OUT;
    float3 f3PositionWS;
    BILERP(QuadPatch, f3PositionWS, f2Coords, f3PositionWS);

    OUT.f4Position = mul(FrameData.mtxViewProj, float4(f3PositionWS, 1.0));
    //OUT.f4Position = mul( float4(f3Pos1, 1.0), FrameData.mtxViewProj);
    OUT.f4Color = Constants.f4Color;

    return OUT;
}
#endif

SamplerState VertexFetchSampler : register(s3, space1);


float4 LoadColor(SPixelShaderInput IN)
{
    float4 color = HeightmapTextures[0].Sample(VertexFetchSampler, IN.f2Texcoord);
    //return float4( IN.f2Texcoord.x, IN.f2Texcoord.y, 0, 1 );
    return IN.f4Color;
    //return float4(IN.f3Normal, 1);
}

float4 ps_main0(in SPixelShaderInput IN) : SV_TARGET0
{
    return LoadColor( IN );
}

float4 ps_main1(in SPixelShaderInput IN) : SV_TARGET0
{
    return LoadColor( IN );
}

float4 ps_main2(in SPixelShaderInput IN) : SV_TARGET0
{
    return LoadColor( IN );
}

float4 ps_main3(in SPixelShaderInput IN) : SV_TARGET0
{
    return LoadColor( IN );
}
