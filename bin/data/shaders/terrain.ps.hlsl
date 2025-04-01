
struct Input
{
    float4 f4Position : SV_Position;
    float3 f3Color : COLOR0;
};

float4 TerrainPS(in Input In) : SV_TARGET
{
    float4 oColor = float4(In.f3Color.rgb, 0.0f);
    return oColor;
}