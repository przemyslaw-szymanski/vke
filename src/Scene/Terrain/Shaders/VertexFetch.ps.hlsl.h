
VKE_TO_STRING(
SamplerState VertexFetchSampler : register(s3, space1);
Texture2D HeightmapTexture : register(t2, space1);

struct SIn
{
    float4  f4Color : COLOR0;
    float3  f3Position : POSITION1;
    float3  f3Normal : NORMAL0;
    float2  f2Texcoord : TEXCOORD0;
};

static float3 g_f3LightPos = float3(1000,100,1000);
float4 LoadColor(SIn IN)
{
    float4 color = HeightmapTexture.Sample(VertexFetchSampler, IN.f2Texcoord);
    //return float4( IN.f2Texcoord.x, IN.f2Texcoord.y, 0, 1 );
    return IN.f4Color;
    //return float4(IN.f3Normal, 1);
}

float4 main0(in SIn IN) : SV_TARGET0
{
    return LoadColor( IN );
}

float4 main1(in SIn IN) : SV_TARGET0
{
    return LoadColor( IN );
}

float4 main2(in SIn IN) : SV_TARGET0
{
    return LoadColor( IN );
}

float4 main3(in SIn IN) : SV_TARGET0
{
    return LoadColor( IN );
}
);