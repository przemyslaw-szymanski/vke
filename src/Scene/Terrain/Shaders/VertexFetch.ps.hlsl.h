
VKE_TO_STRING(
//SamplerState VertexFetchSampler : register(s3, space1);
//Texture2D HeightmapTexture : register(t4, space1);

struct SIn
{
    float4  f4Color : COLOR0;
    float3  f3Position : POSITION1;
    float3  f3Normal : NORMAL0;
    float2  f2Texcoord : TEXCOORD0;
};

static float3 g_f3LightPos = float3(1000,100,1000);

float4 main0(in SIn IN) : SV_TARGET0
{
    //float4 color = HeightmapTexture.Sample(VertexFetchSampler, IN.f2Texcoord);
    //color *= (IN.f4Color * 0.5);
    //return lerp(color, IN.f4Color, 0.3);
    //return float4( 1, 0,0,0 ) * IN.f4Color;
    //return float4(IN.f2Texcoord.x, IN.f2Texcoord.y, 0, 0);
    return IN.f4Color;
    //float3 dir = g_f3LightPos - IN.f4Position.xyz;
    //float3 dir = normalize( float3(0,0,0) - g_f3LightPos );
    //return saturate( dot(-dir, IN.f3Normal) );
}

float4 main1(in SIn IN) : SV_TARGET0
{
    //float4 color = HeightmapTexture.Sample(VertexFetchSampler, IN.f2Texcoord);
    //color *= (IN.f4Color * 0.5);
    //return lerp(color, IN.f4Color, 0.3);
    //return float4(0, 1, 0, 0) * IN.f4Color;
    //return float4(IN.f2Texcoord.x, IN.f2Texcoord.y, 0, 0);
    return IN.f4Color;
}

float4 main2(in SIn IN) : SV_TARGET0
{
    //float4 color = HeightmapTexture.Sample(VertexFetchSampler, IN.f2Texcoord);
    //color *= (IN.f4Color * 0.5);
    //return lerp(color, IN.f4Color, 0.3);
    //return float4(0, 0, 1, 0) * IN.f4Color;
    //return float4(IN.f2Texcoord.x, IN.f2Texcoord.y, 0, 0);
    return IN.f4Color;
}

float4 main3(in SIn IN) : SV_TARGET0
{
    //float4 color = HeightmapTexture.Sample(VertexFetchSampler, IN.f2Texcoord);
    //color *= (IN.f4Color * 0.5);
    //return lerp(color, IN.f4Color, 0.3);
    //return float4( 1, 1, 0, 0 ) * IN.f4Color;
    //return float4(IN.f2Texcoord.x, IN.f2Texcoord.y, 0, 0);
    return IN.f4Color;
}
);