#pragma stage : vertex

struct VertexOutput
{
    float2 TexCoord : TexCoord;
    float4 Position : SV_Position;
};

VertexOutput main(float2 Position : Position)
{
    VertexOutput output;
    output.TexCoord = Position.xy * 2.0f - 1.0f;
    output.Position = float4(Position, 0.0f, 1.0f);
    return output;
}

#pragma stage : pixel
#include "Core/Samplers.hlslh"

Texture2D u_Input : register(t0, space1);

float4 main(float2 TexCoord : TexCoord) : SV_Target
{
    float3 color = u_Input.Sample(u_LinearClamp, TexCoord).rgb;
	
    float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
    float3 tv = color / (1.0f + color);
    float3 mappedColor = lerp(color / (1.0 + luminance), tv, tv);
    
    return float4(mappedColor, 1.0);
}
