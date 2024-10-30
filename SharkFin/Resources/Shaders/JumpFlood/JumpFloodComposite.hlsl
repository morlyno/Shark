
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

#include "HLSLCommon.hlsli"
#include "JumpFlood/JumpFloodCommon.hlsl"

[[vk::binding(0, 1), vk::combinedImageSampler]] uniform Texture2D u_Texture;
[[vk::binding(0, 1), vk::combinedImageSampler]] uniform SamplerState u_Sampler;

struct OutlineSettings
{
    float3 Color;
    float PixelWidth;
};
[[vk::binding(1, 1)]] ConstantBuffer<OutlineSettings> u_Settings;

float ScreenDistanceFromPixels(float pixels, float2 texelSize)
{
    return ScreenDistance(pixels * texelSize, texelSize);
}

float4 main(float2 TexCoord : TexCoord) : SV_Target
{
    float4 pixel = u_Texture.Sample(u_Sampler, TexCoord);
    
    float2 texelSize = GetTexelSize(u_Texture);
    
    float fadeoutStart = ScreenDistanceFromPixels(u_Settings.PixelWidth - 3, texelSize);
    float maxDist = ScreenDistanceFromPixels(u_Settings.PixelWidth, texelSize);
    
    float4 result;
    result.xyz = u_Settings.Color;
    result.a = smoothstep(maxDist, fadeoutStart, pixel.z);
    result.a *= 1.0 - pixel.a;
    
    // only needed for u_settings.PixelWidth == 0
    if (pixel.z > maxDist)
        result.a = 0.0f;
    
    return result;
}
