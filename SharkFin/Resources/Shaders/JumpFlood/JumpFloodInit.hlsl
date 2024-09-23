
// Implemented based on this article
// https://bgolus.medium.com/the-quest-for-very-wide-outlines-ba82ed442cd9

#pragma stage : vertex

#include "JumpFlood/JumpFloodCommon.hlsl"

float4 main(float2 Position : Position) : SV_POSITION
{
    return float4(Position, 0.0f, 1.0f);
}

#pragma stage : pixel

#include "JumpFlood/JumpFloodCommon.hlsl"

half2 main(float4 Position : SV_POSITION) : SV_Target
{
    // integer pixel position
    int2 uvInt = Position.xy;

    // sample silhouette texture for sobel
    half3x3 values;
    [unroll]
    for (int u = 0; u < 3; u++)
    {
        [unroll]
        for (int v = 0; v < 3; v++)
        {
            uint2 sampleUV = clamp(uvInt + int2(u - 1, v - 1), int2(0, 0), (int2) u_Uniforms.TexelSize.zw - 1);
            values[u][v] = u_MainTexture.Load(int3(sampleUV, 0)).r;
        }
    }

    // calculate output position for this pixel
    float2 outPos = Position.xy * abs(u_Uniforms.TexelSize.xy) * FLOOD_ENCODE_SCALE - FLOOD_ENCODE_OFFSET;

    // interior, return position
    if (values._m11 > 0.99)
        return outPos;

    // exterior, return no position
    if (values._m11 < 0.01)
        return FLOOD_NULL_POS_FLOAT2;

    // sobel to estimate edge direction
    float2 dir = -float2(values[0][0] + values[0][1] * 2.0 + values[0][2] - values[2][0] - values[2][1] * 2.0 - values[2][2],
                         values[0][0] + values[1][0] * 2.0 + values[2][0] - values[0][2] - values[1][2] * 2.0 - values[2][2]);

    // if dir length is small, this is either a sub pixel dot or line
    // no way to estimate sub pixel edge, so output position
    if (abs(dir.x) <= 0.005 && abs(dir.y) <= 0.005)
        return outPos;

    // normalize direction
    dir = normalize(dir);

    // sub pixel offset
    float2 offset = dir * (1.0 - values._m11);

    // output encoded offset position
    return (Position.xy + offset) * abs(u_Uniforms.TexelSize.xy) * FLOOD_ENCODE_SCALE - FLOOD_ENCODE_OFFSET;
}
