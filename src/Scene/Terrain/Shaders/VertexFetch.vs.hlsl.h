
R"(
struct SPerFrameTerrainConstantBuffer
{
    float4x4    mtxViewProj;
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
    uint    topVertexShift; // vertex move to highest lod to create stitches
    uint    bottomVertexShift;
    uint    leftVertexShift;
    uint    rightVertexShift;
    uint    heightmapIndex;
};
ConstantBuffer<SPerTileConstantBuffer> TileData : register(b0, space1);


Texture2D HeightmapTexture : register(t1, space1);
Texture2D HeightmapNormalTexture : register(t2, space1);

//layout(location = 0) in float3 iPosition;
struct SIn
{
    float3 f3Position : SV_Position;
};
//layout(location = 0) out float4 oColor;
//layout(location = 1) out float2 oTexcoord;
struct SOut
{
    float4  f4Position : SV_Position;
    float4  f4Color : COLOR0;
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
    float range = vec2Range.y - vec2Range.x;
    return vec2Range.x + v * range;
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
    return fRet;
}

int3 CalcTexcoords(float3 f3VertexPos, float2 f2TextureOffset, float tileSize)
{
    // Convert object space position to texture space
    // Terrain tile vertices are placed along negative Z
    // Negative Z in object space in a positive Y in texture space
    float step = tileSize / BASE_TILE_SIZE;
    float2 f2Offset = float2( f3VertexPos.x, -f3VertexPos.z ) * step + f2TextureOffset*1;
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

void main2(in SIn IN, out SOut OUT)
{
    float4x4 mtxMVP = FrameData.mtxViewProj;
    float3 iPos = IN.f3Position;
    Texture2D Heightmap = HeightmapTexture;
    
    uint2 texSize;
    Heightmap.GetDimensions(texSize.x, texSize.y);
    float4 c = float4(1, 1, 1, 1);
    // Vertex shift is packed in order: top, bottom, left, right
    SVertexShift Shift = UnpackVertexShift(TileData.vertexShift);
    Shift.top = TileData.topVertexShift;
    Shift.bottom = TileData.bottomVertexShift;
    Shift.left = TileData.leftVertexShift;
    Shift.right = TileData.rightVertexShift;

    // Calc vertex offset in object space
    iPos = CalcStitches(IN.f3Position, Shift);
    // Calc texture offset in object space
    int3 i3Texcoords = CalcTexcoords(iPos, TileData.f2TexcoordOffset, TileData.tileSize);
    float4 Normal = HeightmapNormalTexture.Load( i3Texcoords );
    // Calc world space position
    iPos = CalcVertexPositionXZ(iPos, TileData.tileSize, TileData.vec4Position.xyz);
    // Calc world space position height
    iPos.y = CalcPositionY(i3Texcoords, FrameData.vec2TerrainHeight, Heightmap);
    // Calc world projection space position
    OUT.f4Position = mul(mtxMVP, float4(iPos, 1.0));
    OUT.f2Texcoord = float2( float2(i3Texcoords.xy) / texSize );
    //OUT.f4Color = TileData.vec4Color;
    OUT.f4Color = Normal;
}

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

float3 CalcNormal(float3 f3V1, float3 f3V2, float3 f3V3)
{
    float3 f3Ret;
    float3 f3U = f3V2 - f3V1;
    float3 f3V = f3V3 - f3V1;
    f3Ret.x = f3U.y * f3V.z - f3U.z * f3V.y;
    f3Ret.y = f3U.z * f3V.x - f3U.x * f3V.z;
    f3Ret.z = f3U.x * f3V.y - f3U.y * f3V.x;
    return normalize( f3Ret );
}

float3 CalcNormal(SPositions Positions)
{
    float3 f3Ret;
    float3 f3N1 = CalcNormal( Positions.f3Center, Positions.f3Top, Positions.f3LeftTop );
    float3 f3N2 = CalcNormal( Positions.f3Center, Positions.f3LeftTop, Positions.f3Left );
    float3 f3N3 = CalcNormal( Positions.f3Center, Positions.f3Left, Positions.f3Bottom );
    float3 f3N4 = CalcNormal( Positions.f3Center, Positions.f3Bottom, Positions.f3RightBottom );
    float3 f3N5 = CalcNormal( Positions.f3Center, Positions.f3RightBottom, Positions.f3Right );
    float3 f3N6 = CalcNormal( Positions.f3Center, Positions.f3Right, Positions.f3Top );
    //f3Ret = normalize( f3N1 * f3N2 * f3N3 * f3N4 * f3N5 * f3N6 );
    //f3Ret = float3(Positions.f3Top.x, 0.0, 0.0 );
    f3Ret = Positions.f3Top;
    //f3Ret = f3N1;
    return f3Ret;
}

int3 CalcTexOffset(float3 f3VertexPos, float step, float2 f2TextureOffset)
{
    return int3( float2( f3VertexPos.x, -f3VertexPos.z ) * step + f2TextureOffset, 0 );
}

SPositions CalcPositions(float3 f3Center, int3 i3CenterTC, Texture2D Heightmap)
{
    SPositions Ret;
    float vstep = (TileData.tileSize / TILE_VERTEX_COUNT) * BASE_VERTEX_DISTANCE_MULTIPLIER;
    float tstep = TileData.tileSize / BASE_TILE_SIZE;
    Ret.f3Center = f3Center;
    CalcPosition( f3Center + float3( -vstep, 0, 0 ), i3CenterTC + int3( -tstep, 0, 0 ), Heightmap, Ret.f3Left );
    CalcPosition( f3Center + float3( -vstep, 0, +vstep ), i3CenterTC + int3( -tstep, 0, -tstep ), Heightmap, Ret.f3LeftTop );
    CalcPosition( f3Center + float3( +vstep, 0, 0 ), i3CenterTC + int3( +tstep, 0, 0 ), Heightmap, Ret.f3Right );
    CalcPosition( f3Center + float3( +vstep, 0, -vstep ), i3CenterTC + int3( +tstep, 0, +tstep ), Heightmap, Ret.f3RightBottom );
    CalcPosition( f3Center + float3( 0, 0, +vstep ), i3CenterTC + int3( 0, 0, -tstep ), Heightmap, Ret.f3Top );
    CalcPosition( f3Center + float3( 0, 0, -vstep ), i3CenterTC + int3( -tstep, 0, +tstep ), Heightmap, Ret.f3Bottom );

    return Ret;
}

void main(in SIn IN, out SOut OUT)
{
    float4x4 mtxMVP = FrameData.mtxViewProj;
    float3 iPos = IN.f3Position;
    Texture2D Heightmap = HeightmapTexture;
    
    uint2 texSize;
    Heightmap.GetDimensions(texSize.x, texSize.y);
    float4 c = float4(1, 1, 1, 1);
    // Vertex shift is packed in order: top, bottom, left, right
    SVertexShift Shift = UnpackVertexShift(TileData.vertexShift);
    Shift.top = TileData.topVertexShift;
    Shift.bottom = TileData.bottomVertexShift;
    Shift.left = TileData.leftVertexShift;
    Shift.right = TileData.rightVertexShift;

    // Calc vertex offset in object space
    iPos = CalcStitches(IN.f3Position, Shift);

    //iPos = CalcVertexPositionXZ(iPos, TileData.tileSize, TileData.vec4Position.xyz);
    // Calc world space position height
    //iPos.y = CalcPositionY( Positions.i3CenterTC, FrameData.vec2TerrainHeight, Heightmap);
    int3 i3CenterTC;
    CalcPosition( iPos, Heightmap, iPos, i3CenterTC ); 

    // Calc texture offset in object space
    SPositions Positions = CalcPositions(iPos, i3CenterTC, Heightmap);
    

    // Calc world projection space position
    OUT.f4Position = mul(mtxMVP, float4(iPos, 1.0));
    //OUT.f2Texcoord = float2( float2(Positions.i3CenterTC.xy) / texSize );
    OUT.f2Texcoord = float2( i3CenterTC.x, i3CenterTC.y );
    //OUT.f4Color = TileData.vec4Color;
    OUT.f4Color.rgb = CalcNormal( Positions );
}
)";