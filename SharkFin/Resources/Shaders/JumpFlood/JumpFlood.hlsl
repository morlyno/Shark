
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

struct PushConstant
{
    int2 AxisWidth;
    float2 Padding;
};

[[vk::push_constant]] ConstantBuffer<PushConstant> u_PushConstant;

half2 main(float4 Position : SV_POSITION) : SV_Target
{
    // integer pixel position
    int2 uvInt = int2(Position.xy);

    // initialize best distance at infinity
    float bestDist = 1.#INF;
    float2 bestCoord;

    // jump samples
    // only one loop
    [unroll]
    for (int u = -1; u <= 1; u++)
    {
        // calculate offset sample position
        int2 offsetUV = uvInt + u_PushConstant.AxisWidth * u;

        // .Load() acts funny when sampling outside of bounds, so don't
        offsetUV = clamp(offsetUV, int2(0, 0), (int2) u_Uniforms.TexelSize.zw - 1);

        // decode position from buffer
        float2 offsetPos = (u_MainTexture.Load(int3(offsetUV, 0)).rg + FLOOD_ENCODE_OFFSET) * u_Uniforms.TexelSize.zw / FLOOD_ENCODE_SCALE;

        // the offset from current position
        float2 disp = Position.xy - offsetPos;

        // square distance
        float dist = dot(disp, disp);

        // if offset position isn't a null position or is closer than the best
        // set as the new best and store the position
        if (offsetPos.x != -1.0 && dist < bestDist)
        {
            bestDist = dist;
            bestCoord = offsetPos;
        }
    }

    // if not valid best distance output null position, otherwise output encoded position
    return isinf(bestDist) ? FLOOD_NULL_POS_FLOAT2 : bestCoord * u_Uniforms.TexelSize.xy * FLOOD_ENCODE_SCALE - FLOOD_ENCODE_OFFSET;
}
