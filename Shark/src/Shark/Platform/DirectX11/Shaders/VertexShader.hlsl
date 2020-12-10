float4 main( float3 pos : Position ) : SV_POSITION
{
    return float4(pos, 1.0f);
}