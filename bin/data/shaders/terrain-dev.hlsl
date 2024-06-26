
#define DEBUG 1

#define uint16_t min16uint
#define uint32_t uint
#define uint16x2_t min16uint2
#define uint16x3_t min16uint3
#define uint16x4_t min16uint4
#define float16_t min16float
#define float16x2_t min16float2
#define float16x3_t min16float3
#define float16x4_t min16float4

#ifndef MAX_SPLATMAP_SET_COUNT
#   define MAX_SPLATMAP_SET_COUNT 1
#endif

#ifndef ENABLE_TRIPLANAR_MAPPING
#   define ENABLE_TRIPLANAR_MAPPING 1
#endif // ENABLE_TRIPLANAR_MAPPING

struct SCameraData
{
    float4x4 mtxViewProj;
    float3 position;
    float3 direction;
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
ConstantBuffer<SPerFrameConstantBuffer> SceneData : register(b0, space0);

struct SPerFrameTerrainConstantBuffer
{
    float2      vec2TerrainSize;
    float2      vec2TerrainHeight;
    uint        tileRowVertexCount;
};
ConstantBuffer<SPerFrameTerrainConstantBuffer> FrameData : register(b0, space1);

struct STileData
{
    float4  f4Position;
    float4  vec4Color;
    float2  f2TexcoordOffset;
    uint    vertexShift;
    float   tileSize;
    uint    textureIdx;
    uint    heightmapIndex;
    uint    topVertexShift; // vertex move to highest lod to create stitches
    uint    bottomVertexShift;
    uint    leftVertexShift;
    uint    rightVertexShift;
    uint32_t aSplatMapIndices[MAX_SPLATMAP_SET_COUNT]; // 4x uint8 for 4 splatmaps
    uint32_t aaTextureIndices[ MAX_SPLATMAP_SET_COUNT * 4 ][ 2 ]; // 4 indices for every of 4 splatmaps
};
#if INSTANCING_MODE
StructuredBuffer< STileData > g_InstanceData : register(t1, space1);
#else
ConstantBuffer< STileData > TileData : register(b0, space1);
#endif

SamplerState BilinearSampler : register(s2, space1);

//Texture2D HeightmapTexture : register(t1, space1);
Texture2D HeightmapTextures[] : register(t3, space1);
Texture2D HeightmapNormalTexture : register(t4, space1);
Texture2D SplatmapTextures[] : register(t5, space1);
Texture2D DiffuseTextures[] : register(t6, space1);

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
    float2  f2Texcoords : TEXCOORD0;
    float2  f2DrawTexcoords : TEXCOORD1;
    uint    uInstanceID : SV_InstanceID;
#if ENABLE_TRIPLANAR_MAPPING
    float16x2_t UVs[3] : TEXCOORD2;
#endif
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

uint16x2_t UnpackUint32ToUint16(uint32_t data)
{
    uint16x2_t ret;
    ret.x = (uint16_t)( data >> 16u ) & 0xFFFF;
    ret.y = (uint16_t)( data & 0xFFFF );
    return ret;
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

int3 CalcHeightmapTexcoords(float3 f3VertexPos, float2 f2TextureOffset, float tileSize)
{
    // Convert object space position to texture space
    // Terrain tile vertices are placed along negative Z
    // Negative Z in object space in a positive Y in texture space
    float step = tileSize / BASE_TILE_SIZE;
    float2 f2Offset = float2( f3VertexPos.x, -f3VertexPos.z ) * step + (f2TextureOffset + float2(0,0));
    return int3(f2Offset,0);
}

float2 CalcOutputTexcoords(float3 f3VertexObjSpacePos, STileData TileData)
{
    // Convert object space position to texture space
    // Terrain tile vertices are placed along negative Z
    // Negative Z in object space in a positive Y in texture space
    float step = TileData.tileSize / BASE_TILE_SIZE;
    float2 f2Offset = float2( f3VertexObjSpacePos.x, -f3VertexObjSpacePos.z ) * step + (TileData.f2TexcoordOffset - float2(1,1));
    return f2Offset / float2(2048, 2048);
}

#if INSTANCING_MODE
void CalcPosition(float3 f3Pos, Texture2D Heightmap, STileData TileData, out float3 f3PosOut, out int3 i3TC)
{
    i3TC = CalcHeightmapTexcoords( f3Pos, TileData.f2TexcoordOffset, TileData.tileSize );
    f3PosOut = CalcVertexPositionXZ( f3Pos, uint( TileData.tileSize ), TileData.f4Position.xyz );

    f3PosOut.y = CalcPositionY( i3TC, FrameData.vec2TerrainHeight, Heightmap );
    //return f3Pos;
}
#else
void CalcPosition(float3 f3Pos, Texture2D Heightmap, out float3 f3PosOut, out int3 i3TC)
{
    i3TC = CalcHeightmapTexcoords( f3Pos, TileData.f2TexcoordOffset, TileData.tileSize );
    f3PosOut = CalcVertexPositionXZ( f3Pos, uint( TileData.tileSize ), TileData.f4Position.xyz );

    f3PosOut.y = CalcPositionY( i3TC, FrameData.vec2TerrainHeight, Heightmap );
    //return f3Pos;
}
#endif

void CalcPosition(float3 f3Pos, int3 i3Texcoords, Texture2D Heightmap, out float3 f3PosOut )
{
    //int3 i3TcOut = CalcHeightmapTexcoords( f3Pos, TileData.f2TexcoordOffset, TileData.tileSize );
    //f3PosOut = CalcVertexPositionXZ( f3Pos, uint( TileData.tileSize ), TileData.f4Position.xyz );
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

#if INSTANCING_MODE
SPositions2 CalcPositions(float3 f3WorldSpaceCenter, float3 f3ObjSpaceCenter, int3 i3CenterTC, Texture2D Heightmap, STileData TileData)
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
#else
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
#endif

#if INSTANCING_MODE
float3 CalcNormal2(float3 f3WorldSpaceCenter, float3 f3ObjSpaceCenter, int3 i3CenterTC, Texture2D Heightmap, STileData TileData)
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
#else
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
#endif // INSTANCING_MODE

#if INSTANCING_MODE
float3 CalcNormal3(float3 f3WorldSpaceCenter, float3 f3ObjSpaceCenter, int3 i3CenterTC, Texture2D Heightmap, STileData TileData)
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
#else
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
#endif

float3 CalcLight( int3 i3Texcoords, float3 f3PositionWS, inout float3 f3NormalWS)
{
    float3 ret;
    float3 f3Normal = HeightmapNormalTexture.Load( i3Texcoords ).rgb;
    //f4Normal = mul(SceneData.Camera.mtxViewProj, f4Normal);
    f3Normal = f3NormalWS;
    float3 lightDir = normalize( SceneData.MainLight.vec3Position - f3PositionWS );
    
    ret = saturate( dot( lightDir, f3Normal ) );
    f3NormalWS = f3Normal;
    return ret;
}

float3 CalcLight(float3 f3PositionWS, float3 f3Normal)
{
    float3 lightDir = normalize( SceneData.MainLight.vec3Position - f3PositionWS );
    float3 ret = saturate( dot( lightDir, f3Normal ) );
    return ret;
}

#if !INSTANCING_MODE
SPixelShaderInput vs_main(in SVertexShaderInput IN)
{
    SPixelShaderInput OUT;

    float4x4 mtxMVP = SceneData.Camera.mtxViewProj;
    Texture2D Heightmap = HeightmapTextures[TileData.textureIdx];
    //Texture2D Heightmap = HeightmapTextures[0];
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

    //iPos = CalcVertexPositionXZ(iPos, TileData.tileSize, TileData.f4Position.xyz);
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
    //OUT.f2Texcoords = float2( float2(Positions.i3CenterTC.xy) / texSize );
    OUT.f2Texcoords = (float2( i3CenterTC.xy ) - float2(1,1)) / float2(2048, 2048);
    
    //OUT.f3Normal = CalcNormal( Positions, f3ObjSpacePos );
    OUT.f3Normal = f3Normal;
    
    OUT.f4Color.rgb = CalcLight( i3CenterTC, OUT.f3Position, OUT.f3Normal );
    #if DEBUG
        OUT.f4Color *= TileData.vec4Color;
    #endif

    return OUT;
}
#endif // !INSTANCING_MODE

struct SHullShaderInput
{
    float4  f4Color : COLOR0;
    float3  f3PositionWS : POSITION0;
    float3  f3Normal : NORMAL0;
    float2  f2Texcoords : TEXCOORD0;
};

struct SDomainShaderInput
{
    float3  f3PositionWS : POSITION0;
    float3  f3Normal : NORMAL0;
    float2  f2Texcoords : TEXCOORD1;
    float4  f4Color : COLOR1;
};

#if !INSTANCING_MODE
SHullShaderInput vs_main_tess(in SVertexShaderInput IN)
{
    SHullShaderInput OUT;

    float4x4 mtxMVP = SceneData.Camera.mtxViewProj;
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

    //iPos = CalcVertexPositionXZ(iPos, TileData.tileSize, TileData.f4Position.xyz);
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
    //OUT.f2Texcoords = float2( float2(Positions.i3CenterTC.xy) / texSize );
    //OUT.f2Texcoords = float2( i3CenterTC.xy ) / float2(2049, 2049);
    
    //OUT.f3Normal = CalcNormal( Positions, f3ObjSpacePos );
    OUT.f3Normal = f3Normal;
    //OUT.i2Texcoords = i3CenterTC.xy;
    OUT.f2Texcoords = abs(IN.f3Position.xz / TILE_VERTEX_COUNT);
    
    //OUT.f4Color.rgb = CalcLight( i3CenterTC, OUT.f3PositionWS, OUT.f3Normal );
    //OUT.f4Color.rg = float2(i3CenterTC.xy) / 2049.0;
    #if DEBUG
        OUT.f4Color = TileData.vec4Color;
    #endif

    return OUT;
}
#endif
void CalcTriplanar(float3 positionWorld, float3 normalWorld, float tileSize, out float16x2_t UVs[3])
{
    float3 blending = abs(normalWorld);
    blending = normalize(max(blending, 0.000001));
    float b = blending.x + blending.y + blending.z;
    blending /= float3(b, b, b);

    float16x2_t yUV = (float16x2_t)(positionWorld.xz / tileSize);
    float16x2_t xUV = (float16x2_t)(positionWorld.zy / tileSize);
    float16x2_t zUV = (float16x2_t)(positionWorld.xy / tileSize);
    UVs[0] = xUV;
    UVs[1] = yUV;
    UVs[2] = zUV;
}
#if INSTANCING_MODE
SPixelShaderInput vs_main_instancing(in SVertexShaderInput IN, in uint instanceID : SV_InstanceID)
{
    SPixelShaderInput OUT;

    STileData TileData = g_InstanceData[ instanceID ];

    float4x4 mtxMVP = SceneData.Camera.mtxViewProj;
    Texture2D Heightmap = HeightmapTextures[TileData.textureIdx];
    //Texture2D Heightmap = HeightmapTextures[0];
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

    //iPos = CalcVertexPositionXZ(iPos, TileData.tileSize, TileData.f4Position.xyz);
    // Calc world space position height
    //iPos.y = CalcPositionY( Positions.i3CenterTC, FrameData.vec2TerrainHeight, Heightmap);
    int3 i3CenterTC;
    float3 f3WorldSpacePos;
    CalcPosition( f3ObjSpacePos, Heightmap, TileData, f3WorldSpacePos, i3CenterTC ); 

    // Calc texture offset in object space
    //SPositions2 Positions = CalcPositions(f3WorldSpacePos, f3ObjSpacePos, i3CenterTC, Heightmap);
    float3 f3Normal = CalcNormal3( f3WorldSpacePos, f3ObjSpacePos, i3CenterTC, Heightmap, TileData );

    // Calc world projection space position
    OUT.f4Position = mul(mtxMVP, float4(f3WorldSpacePos, 1.0));
    OUT.f3Position = f3WorldSpacePos;
    //OUT.f2Texcoords = float2( float2(Positions.i3CenterTC.xy) / texSize );
    //OUT.f2Texcoords = float2( i3CenterTC.xy ) / float2(2048, 2048);
    OUT.f2Texcoords = CalcOutputTexcoords(f3ObjSpacePos, TileData);
    OUT.f2DrawTexcoords = f3ObjSpacePos.xz / TileData.tileSize;
    
    //OUT.f3Normal = CalcNormal( Positions, f3ObjSpacePos );
    OUT.f3Normal = f3Normal;
    OUT.uInstanceID = instanceID;

#if ENABLE_TRIPLANAR_MAPPING
    CalcTriplanar(OUT.f3Position, OUT.f3Normal, TileData.tileSize, OUT.UVs);
#endif // ENABLE_TRIPLANAR_MAPPING
    
    OUT.f4Color.rgb = CalcLight( i3CenterTC, OUT.f3Position, OUT.f3Normal );
    #if DEBUG
        OUT.f4Color *= TileData.vec4Color;
    #endif

    return OUT;
}

SHullShaderInput vs_main_instancing_tess(in SVertexShaderInput IN, in uint instanceID : SV_InstanceID)
{
    SHullShaderInput OUT;

    float4x4 mtxMVP = SceneData.Camera.mtxViewProj;
    STileData TileData = g_InstanceData[ instanceID ];
    TileData.f4Position.xz = float2(0,0);
    TileData.tileSize = 128;
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

    //iPos = CalcVertexPositionXZ(iPos, TileData.tileSize, TileData.f4Position.xyz);
    // Calc world space position height
    //iPos.y = CalcPositionY( Positions.i3CenterTC, FrameData.vec2TerrainHeight, Heightmap);
    int3 i3CenterTC;
    float3 f3WorldSpacePos;
    CalcPosition( f3ObjSpacePos, Heightmap, TileData, f3WorldSpacePos, i3CenterTC ); 

    // Calc texture offset in object space
    //SPositions2 Positions = CalcPositions(f3WorldSpacePos, f3ObjSpacePos, i3CenterTC, Heightmap);
    float3 f3Normal = CalcNormal3( f3WorldSpacePos, f3ObjSpacePos, i3CenterTC, Heightmap, TileData );

    // Calc world projection space position
    //OUT.f4Position = mul(mtxMVP, float4(f3WorldSpacePos, 1.0));
    OUT.f3PositionWS = f3WorldSpacePos;
    //OUT.f2Texcoords = float2( float2(Positions.i3CenterTC.xy) / texSize );
    //OUT.f2Texcoords = float2( i3CenterTC.xy ) / float2(2049, 2049);
    
    //OUT.f3Normal = CalcNormal( Positions, f3ObjSpacePos );
    OUT.f3Normal = f3Normal;
    //OUT.i2Texcoords = i3CenterTC.xy;
    OUT.f2Texcoords = abs(IN.f3Position.xz / TILE_VERTEX_COUNT);
    
    //OUT.f4Color.rgb = CalcLight( i3CenterTC, OUT.f3PositionWS, OUT.f3Normal );
    //OUT.f4Color.rg = float2(i3CenterTC.xy) / 2049.0;
    #if DEBUG
        OUT.f4Color = TileData.vec4Color;
    #endif

    return OUT;
}
#endif

struct STrianglePatchConstants
{
    float edgeFactors[3] : SV_TessFactor;
    float insideFactor : SV_InsideTessFactor;

};
struct SQuadPatchConstants
{
    float edgeFactors[4] : SV_TessFactor;
    float insideFactors[2] : SV_InsideTessFactor;

};


#if CONTROL_POINTS == 4

float CalcTesselationFactorDistanceFromCamera(float distance)
{
    const float farClip = 10000;
    const float nearClip = 0.1;
    float scale = 1 - ( (distance - nearClip) / ( farClip - nearClip  ) );
    scale = (0.01 * scale) * 63 + 1;
    return clamp( scale, 1, 64);
}

float CalcTesselationFactorDistanceFromCameraLog(float distance)
{
    float d = ( (distance / 10) );
    d = clamp( log( d ) / 4, 0, 1 );
    return lerp( 64, 1, d );
}

float CalcTesselationFactorDistanceFromCameraPow(float distance)
{
    float d = 1 / distance;
    //d = clamp( pow( d, 0.2 ), 0, 1 );
    d = clamp( (1 - pow( d, TESS_FACTOR_DISTANCE_POW_REDUCTION_SPEED )) * TESS_FACTOR_DISTANCE_POW_REDUCTION_SPEED_MULTIPLIER, 0, 1);
    //return float2( lerp( TESS_FACTOR_MAX, TESS_FACTOR_MIN, d ), d );
    return lerp( TESS_FACTOR_MAX, TESS_FACTOR_MIN, d );
}

SQuadPatchConstants hs_PatchConstantFunc(InputPatch< SHullShaderInput, 4 > Patch, uint patchId : SV_PrimitiveID )
{
    SQuadPatchConstants OUT;
    float factor;
    //////////////////////////////////
    // Control point order
    //        OL3
    //      2 --- 3
    //  OL0 |     | OL2
    //      0 --- 1
    //        OL1
    //////////////////////////////////
#if TESS_FACTOR_MAX_DISTANCE > 0
    float3 f3Center0 = lerp(Patch[0].f3PositionWS, Patch[2].f3PositionWS, 0.5);
    float3 f3Center1 = lerp(Patch[1].f3PositionWS, Patch[0].f3PositionWS, 0.5);
    float3 f3Center2 = lerp(Patch[1].f3PositionWS, Patch[3].f3PositionWS, 0.5);
    float3 f3Center3 = lerp(Patch[3].f3PositionWS, Patch[2].f3PositionWS, 0.5);

    
    float distances[4];
    distances[0] = distance(f3Center0, SceneData.MainLight.vec3Position);
    distances[1] = distance(f3Center1, SceneData.MainLight.vec3Position);
    distances[2] = distance(f3Center2, SceneData.MainLight.vec3Position);
    distances[3] = distance(f3Center3, SceneData.MainLight.vec3Position);
    
    

    for(int i = 0; i < 4; ++i)
    {
        float factor = CalcTesselationFactorDistanceFromCameraPow(distances[i]);
        //distances[i]= factor;
        OUT.edgeFactors[i] = factor;
    }
    

#elif TESS_FACTOR_MAX_VIEW

#else
    factor = ( TESS_FACTOR_MAX + TESS_FACTOR_MIN ) * 0.5;
    OUT.edgeFactors[0] = factor;
    OUT.edgeFactors[1] = factor;
    OUT.edgeFactors[2] = factor;
    OUT.edgeFactors[3] = factor;
    OUT.insideFactors[0] = (OUT.edgeFactors[1] + OUT.edgeFactors[3]) * 0.5;
    OUT.insideFactors[1] = (OUT.edgeFactors[0] + OUT.edgeFactors[2]) * 0.5;
#endif


#if TESS_FACTOR_FLAT_SURFACE_REDUCTION

    float dots[4];
    dots[0] = dot(Patch[0].f3Normal, Patch[2].f3Normal);
    dots[1] = dot(Patch[0].f3Normal, Patch[1].f3Normal);
    dots[2] = dot(Patch[1].f3Normal, Patch[3].f3Normal);
    dots[3] = dot(Patch[2].f3Normal, Patch[3].f3Normal);
    for( i = 0; i < 4; ++i)
    {
        float v = clamp(dots[i], 0.1, 1 );
        //OUT.edgeFactors[i] *= v;
    }
    
#endif

    OUT.insideFactors[0] = (OUT.edgeFactors[1] + OUT.edgeFactors[3]) * 0.5;
    OUT.insideFactors[1] = (OUT.edgeFactors[0] + OUT.edgeFactors[2]) * 0.5;

    return OUT;
}
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("hs_PatchConstantFunc")]
SDomainShaderInput hs_main(in InputPatch< SHullShaderInput, 4 > Patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    SDomainShaderInput OUT;
    OUT.f3PositionWS = Patch[ pointId ].f3PositionWS;
    OUT.f4Color = Patch[ pointId ].f4Color;
    OUT.f3Normal = Patch[ pointId ].f3Normal;
    OUT.f2Texcoords = Patch[pointId].f2Texcoords;

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
    output = lerp( lerp( Patch[0].var, Patch[1].var, UV.x ), lerp( Patch[2].var, Patch[3].var, UV.x ), UV.y);



[domain("quad")]
SPixelShaderInput ds_main(
    in SQuadPatchConstants Constants,
    float2 f2Coords : SV_DomainLocation,
    const OutputPatch< SDomainShaderInput, 4 > QuadPatch)
{
    SPixelShaderInput OUT;
    float3 f3PositionWS;
    BILERP(QuadPatch, f3PositionWS, f2Coords, f3PositionWS);
    float2 f2Texcoords;
    BILERP(QuadPatch, f2Texcoords, f2Coords, f2Texcoords);
    float4 f4Color;
    BILERP(QuadPatch, f4Color, f2Coords, f4Color);
    float3 f3Normal;
    BILERP(QuadPatch, f3Normal, f2Coords, f3Normal);

    float fHeight = HeightmapTextures[0].SampleLevel( BilinearSampler, f2Texcoords, 0).r;
    //f3PositionWS.y += SampleToRange(fHeight, float2( -10, 10 ));

    //float2 f2Texcoords = float2(Constants.i2Texcoords / 2049);

    OUT.f4Position = mul(SceneData.Camera.mtxViewProj, float4(f3PositionWS, 1.0));
    //OUT.f4Position = mul( float4(f3Pos1, 1.0), SceneData.Camera.mtxViewProj);
    OUT.f4Color.rgb = CalcLight( f3PositionWS, f3Normal ) * f4Color.rgb;

    return OUT;
}
#endif

float4 Blend1(float4 a, float w1, float4 b, float w2)
{
    return a.a + w1 > b.a + w2 ? a : b;
}

float4 Blend2(float4 a, float w1, float4 b, float w2)
{
    return a * w1 + b * w2;
}

uint16x4_t Uint32ToUint8(uint32_t data)
{
    uint16x4_t s;
    s.x = (uint16_t)((data >> 24u) & 0xFF);
    s.y = (uint16_t)((data >> 16u) & 0xFF);
    s.z = (uint16_t)((data >> 8u) & 0xFF);
    s.w = (uint16_t)((data) & 0xFF);
    return s;
}

float4 LoadColor(SamplerState Sampler, Texture2D Texture, float2 uv)
{
    return Texture.Sample( Sampler, uv );
}

#if ENABLE_TRIPLANAR_MAPPING
#define UV_COUNT 3
#define SAMPLE_COUNT 3
#else
#define UV_COUNT 1
#define SAMPLE_COUNT 1
#endif
#define UV_SCALE float16x2_t(500,500)
#define UV_SCALE_TRIPLANAR float16x2_t(20,20)
#ifndef SLOPE_THRESHOLD
#   define SLOPE_THRESHOLD 0.5
#endif
#ifndef SNOW_HEIGHT_THRESHOLD
#define SNOW_HEIGHT_THRESHOLD 100
#endif

#define TEXTURE_GROUND 3
#define TEXTURE_ROCK 1
#define TEXTURE_ROAD 2
#define TEXTURE_SNOW 0

#ifndef ENABLE_AUTOTEXTURING
#define ENABLE_AUTOTEXTURING 0
#endif

#ifndef TRIPLANAR_ALWAYS
#define TRIPLANAR_ALWAYS 1
#endif

float4 LoadColor(SamplerState Sampler, float3 normal, Texture2D Texture, float16x2_t UVs[UV_COUNT], bool useTriplanar)
{
    float4 texColors[UV_COUNT];
    float4 ret = float4(0, 0, 0, 0);
    float3 bf = (float3)1;
#if ENABLE_TRIPLANAR_MAPPING
    if (useTriplanar)
    {
        bf = normalize(abs(normal));
        bf /= dot(bf, float3(1, 1, 1));
    }
#else
    bf = float3(1, 1, 1);
#endif
    const uint sampleCount = useTriplanar ? SAMPLE_COUNT :  1;
    uint i;
    for (i = 0; i < sampleCount; ++i)
    {
        texColors[i] = LoadColor(Sampler, Texture, UVs[i]);
    }
    for (i = 0; i < sampleCount; ++i)
    {
        ret += texColors[i] * bf[i];
    }
    return ret;
}

void CalcUV(SPixelShaderInput IN, STileData Tile, bool useTriplanar, out float16x2_t UVs[UV_COUNT])
{
#if ENABLE_TRIPLANAR_MAPPING
    if (useTriplanar)
    {
        UVs[0] = (float16x2_t)(IN.f3Position.yz / Tile.tileSize) * UV_SCALE_TRIPLANAR;
        UVs[1] = (float16x2_t)(IN.f3Position.zx / Tile.tileSize) * UV_SCALE_TRIPLANAR;
        UVs[2] = (float16x2_t)(IN.f3Position.xy / Tile.tileSize) * UV_SCALE_TRIPLANAR;
    }
    else
    {
        UVs[0] = (float16x2_t)(IN.f2Texcoords * UV_SCALE);    
    }
#else
    UVs[0] = (float16x2_t)(IN.f2Texcoords * UV_SCALE);
#endif
}

uint CalcTextureIndex(float slope, float height, uint currentIndex)
{
#if ENABLE_AUTOTEXTURING
    if (slope > SLOPE_THRESHOLD)
    {
        return TEXTURE_ROCK;
    }
    if(height > SNOW_HEIGHT_THRESHOLD)
    {
        return TEXTURE_SNOW;
    }
    return currentIndex;
#else
    return currentIndex;
#endif
}

float4 LoadColor(SPixelShaderInput IN)
{
    STileData TileData = g_InstanceData[IN.uInstanceID];
    float4 finalColor = float4(0, 0, 0, 1);
    float16x2_t UVs[UV_COUNT];

    float slope = 1 - IN.f3Normal.y;
    const bool useTriplanar = TRIPLANAR_ALWAYS ? true : slope > SLOPE_THRESHOLD;

    CalcUV(IN, TileData, useTriplanar, UVs);

    for(uint32_t splatmapSetIndex = 0; splatmapSetIndex < MAX_SPLATMAP_SET_COUNT; ++splatmapSetIndex)
    {
        uint16x4_t u4SplatIndices = Uint32ToUint8( TileData.aSplatMapIndices[ splatmapSetIndex ] );
        for(uint32_t splatmapIndex = 0; splatmapIndex < 4; ++splatmapIndex)
        {
            uint16_t splatmapTexIdx = u4SplatIndices[ splatmapIndex ];
            if( splatmapTexIdx == 0xFF )
            {
                break;
            }
                
            uint16x4_t texIndices;
            texIndices.xy = UnpackUint32ToUint16( TileData.aaTextureIndices[ splatmapIndex ][ 0 ] );
            texIndices.zw = UnpackUint32ToUint16( TileData.aaTextureIndices[ splatmapIndex ][ 1 ] );

            float4 splatmapColor = SplatmapTextures[splatmapTexIdx].Sample(BilinearSampler, IN.f2Texcoords);
            splatmapColor.a = 0;
            float4 diffuseColors[4];
            uint32_t i;
            for(i = 0; i < 4; ++i)
            {
                uint16_t texIdx = texIndices[ CalcTextureIndex( slope, IN.f3Position.y, i ) ];
                diffuseColors[i] = float4(1,0,0,0);
                if(texIdx != 0xFFFF)
                {
                    diffuseColors[i] = LoadColor( BilinearSampler, IN.f3Normal, DiffuseTextures[ texIdx ], UVs, useTriplanar );
                    diffuseColors[i].a = (diffuseColors[i].r + diffuseColors[i].g + diffuseColors[i].b) / 3;
                    //f4Diffuses[i].rgb *= splatmapColor[i];
                }
            }
            for(i = 0; i < 4; i += 2)
            {
                finalColor += Blend2( diffuseColors[i], splatmapColor[i], diffuseColors[i+1], splatmapColor[i+1] );
            }
        }
    }
    return finalColor;
}

float4 CalcLSunLight(in SPixelShaderInput IN)
{
    float4 color = float4(1, 1, 1, 1);
    float shininess = 1;
    float3 lightDir = normalize(SceneData.MainLight.vec3Position - IN.f3Position);
    float3 viewDir = normalize(SceneData.Camera.position - IN.f3Position);
    float3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(IN.f3Normal, halfDir), 0.0), shininess);
    float3 specular = spec;
    color.rgb = specular * SceneData.MainLight.vec3Color;
    return color ;
}

float4 CalcColor(in SPixelShaderInput IN)
{
    return /*CalcLSunLight(IN) */ LoadColor( IN );
}

float4 ps_main0(in SPixelShaderInput IN) : SV_TARGET0
{
    return CalcColor( IN );
}

float4 ps_main1(in SPixelShaderInput IN) : SV_TARGET0
{
    return CalcColor( IN );
}

float4 ps_main2(in SPixelShaderInput IN) : SV_TARGET0
{
    return CalcColor( IN );
}

float4 ps_main3(in SPixelShaderInput IN) : SV_TARGET0
{
    return CalcColor( IN );
}
