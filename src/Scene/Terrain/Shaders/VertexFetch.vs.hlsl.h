
VKE_TO_STRING(
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

//layout(set = 1, binding = 1) uniform sampler VertexFetchSampler;
//layout(set = 1, binding = 2) uniform texture2D HeightmapTextures[10];
//SamplerState VertexFetchSampler : register(s1, space1);
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

float CalcPositionY(int2 i2UnnormalizedTexcoords, float2 f2HeightMinMax, Texture2D Heightmap)
{
    float fHeight = Heightmap.Load( int3( i2UnnormalizedTexcoords, 0 ) ).r;
    float fRet = SampleToRange(fHeight, f2HeightMinMax);
    return fRet;
}

int2 CalcTexcoords(float3 f3VertexPos, float2 f2TextureOffset, float tileSize)
{
    // Convert object space position to texture space
    // Terrain tile vertices are placed along negative Z
    // Negative Z in object space in a positive Y in texture space
    float step = tileSize / BASE_TILE_SIZE;
    //f2TextureOffset.x = max(0, f2TextureOffset.x - 1) * step;
    //f2TextureOffset.y = max(0, f2TextureOffset.y - 1) * step;
    float2 f2Offset = float2( f3VertexPos.x, -f3VertexPos.z ) * step + f2TextureOffset*1;
    return int2(f2Offset);
}

void main(in SIn IN, out SOut OUT)
{
    float4x4 mtxMVP = FrameData.mtxViewProj;
    float3 iPos = IN.f3Position;
    Texture2D Heightmap = HeightmapTexture;
    //float2 texSize = textureSize(sampler2D(HeightmapTextures[0], VertexFetchSampler), 0);
    uint2 texSize;
    Heightmap.GetDimensions(texSize.x, texSize.y);
    float4 c = float4(1, 1, 1, 1);
    // Vertex shift is packed in order: top, bottom, left, right
    SVertexShift Shift = UnpackVertexShift(TileData.vertexShift);
    Shift.top = TileData.topVertexShift;
    Shift.bottom = TileData.bottomVertexShift;
    Shift.left = TileData.leftVertexShift;
    Shift.right = TileData.rightVertexShift;

    iPos = CalcStitches(IN.f3Position, Shift);
    iPos = CalcVertexPositionXZ(iPos, TileData.tileSize, TileData.vec4Position.xyz);

    int2 i2Texcoords = CalcTexcoords(IN.f3Position, TileData.f2TexcoordOffset, TileData.tileSize);
    iPos.y = CalcPositionY(i2Texcoords, FrameData.vec2TerrainHeight, Heightmap);

    OUT.f4Position = mul(mtxMVP, float4(iPos, 1.0));
    OUT.f2Texcoord = float2( float2(i2Texcoords) / texSize );
    //OUT.f4Color = TileData.vec4Color;
    OUT.f4Color = Heightmap.Load(int3(i2Texcoords, 0));
}

void main2(in SIn IN, out SOut OUT)
{
    float4x4 mtxMVP = FrameData.mtxViewProj;
    float3 iPos = IN.f3Position;
    Texture2D Heightmap = HeightmapTexture;
    //float2 texSize = textureSize(sampler2D(HeightmapTextures[0], VertexFetchSampler), 0);
    uint2 texSize;
    Heightmap.GetDimensions(texSize.x, texSize.y);
    float4 c = float4(1, 1, 1, 1 );
    // Vertex shift is packed in order: top, bottom, left, right
    SVertexShift Shift = UnpackVertexShift(TileData.vertexShift);
    Shift.top = TileData.topVertexShift;
    Shift.bottom = TileData.bottomVertexShift;
    Shift.left = TileData.leftVertexShift;
    Shift.right = TileData.rightVertexShift;

    iPos = CalcStitches(IN.f3Position, Shift);
    iPos = CalcVertexPositionXZ(iPos, TileData.tileSize, TileData.vec4Position.xyz);

    int2 i2UnnormalizedTexcoords = CalcUnnormalizedTexcoords(iPos, texSize);
    float2 f2NormalizedTexcoords = CalcNormalizedTexcoords(i2UnnormalizedTexcoords, texSize);
    iPos.y = CalcPositionY( i2UnnormalizedTexcoords, FrameData.vec2TerrainHeight, Heightmap );

    OUT.f4Position = mul(mtxMVP, float4(iPos, 1.0));
    OUT.f4Color = TileData.vec4Color;
    OUT.f2Texcoord = f2NormalizedTexcoords;
}
);