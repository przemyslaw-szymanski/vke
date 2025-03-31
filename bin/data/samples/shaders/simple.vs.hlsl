
float4 main( float3 position : POSITION ) : SV_POSITION
{
    float4 output = float4( position, 1.0f );
    return output;
}