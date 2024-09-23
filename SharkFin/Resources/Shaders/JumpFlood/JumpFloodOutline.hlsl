
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

float4 main(float4 Position : SV_POSITION) : SV_Target
{
    // integer pixel position
    int2 uvInt = int2(Position.xy);

    // load encoded position
    float2 encodedPos = u_MainTexture.Load(int3(uvInt, 0)).rg;

    // early out if null position
    if (encodedPos.y == -1)
        discard;
        //return half4(0, 0, 0, 0);

    // decode closest position
    float2 nearestPos = (encodedPos + FLOOD_ENCODE_OFFSET) * abs(u_Uniforms.FrameBufferSize.xy) / FLOOD_ENCODE_SCALE;

    // current pixel position
    float2 currentPos = Position.xy;

    // distance in pixels to closest position
    half dist = length(nearestPos - currentPos);

    // calculate outline
    // + 1.0 is because encoded nearest position is half a pixel inset
    // not + 0.5 because we want the anti-aliased edge to be aligned between pixels
    // distance is already in pixels, so this is already perfectly anti-aliased!
    half outline = saturate(u_Uniforms.OutlineWidth - dist + 1.0);

    // apply outline to alpha
    half4 col = u_Uniforms.OutlineColor;
    col.a *= outline;

    // profit!
    return col;
}
