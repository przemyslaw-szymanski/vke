
VKE_TO_STRING(
//SamplerState VertexFetchSampler : register(s3, space1);
//Texture2D HeightmapTexture : register(t4, space1);

struct SIn
{
    float4 f4Color : COLOR0;
    float2 f2Texcoord : TEXCOORD0;
};

float4 main0(in SIn IN) : SV_TARGET0
{
    //float4 color = HeightmapTexture.Sample(VertexFetchSampler, IN.f2Texcoord);
    //color *= (IN.f4Color * 0.5);
    //return lerp(color, IN.f4Color, 0.3);
    return float4( 1, 0,0,0 );
}

float4 main1(in SIn IN) : SV_TARGET0
{
    //float4 color = HeightmapTexture.Sample(VertexFetchSampler, IN.f2Texcoord);
    //color *= (IN.f4Color * 0.5);
    //return lerp(color, IN.f4Color, 0.3);
    return float4(0, 1, 0, 0);
}

float4 main2(in SIn IN) : SV_TARGET0
{
    //float4 color = HeightmapTexture.Sample(VertexFetchSampler, IN.f2Texcoord);
    //color *= (IN.f4Color * 0.5);
    //return lerp(color, IN.f4Color, 0.3);
    return float4(0, 0, 1, 0);
}

float4 main3(in SIn IN) : SV_TARGET0
{
    //float4 color = HeightmapTexture.Sample(VertexFetchSampler, IN.f2Texcoord);
    //color *= (IN.f4Color * 0.5);
    //return lerp(color, IN.f4Color, 0.3);
    return float4( 1, 1, 0, 0 );
}
);