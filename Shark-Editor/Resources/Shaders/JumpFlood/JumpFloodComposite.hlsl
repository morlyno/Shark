#pragma layout : renderpass

#pragma stage : vertex

struct VertexOutput
{
    float2 TexCoord : TexCoord;
    float4 Position : SV_POSITION;
};

VertexOutput main(float3 Position : Position, float2 TexCoord : TexCoord)
{
    VertexOutput output;
    output.TexCoord = TexCoord;
    output.Position = float4(Position, 1.0f);
    return output;
}

#pragma stage : pixel

#include "JumpFloodCommon.hlslh"
#include "Core/Texture.hlslh"
#include "Core/Samplers.hlslh"

struct OutlineSettings
{
    float3 Color;
    float PixelWidth;
    float2 TexelSize;
};

ConstantBuffer<OutlineSettings> u_Settings : register(b1, space1);
Texture2D u_Texture : register(t0, space1);

float ScreenDistanceFromPixels(float pixels, float2 texelSize)
{
    return ScreenDistance(pixels * texelSize, texelSize);
}

float4 main(float2 TexCoord : TexCoord) : SV_Target
{
    float4 pixel = u_Texture.Sample(u_LinearClamp, TexCoord);
    
    float fadeoutStart = ScreenDistanceFromPixels(u_Settings.PixelWidth - 3, u_Settings.TexelSize);
    float maxDist = ScreenDistanceFromPixels(u_Settings.PixelWidth, u_Settings.TexelSize);
    
    float4 result;
    result.xyz = u_Settings.Color;
    result.a = smoothstep(maxDist, fadeoutStart, pixel.z);
    result.a *= 1.0 - pixel.a;
    
    // only needed for u_settings.PixelWidth == 0
    if (pixel.z > maxDist)
        result.a = 0.0f;
    
    return result;
}
