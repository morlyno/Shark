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
#include "Core/Samplers.hlslh"

uniform Texture2D u_Texture : register(t0, space1);

float4 main(float2 TexCoord : TexCoord) : SV_Target
{
    float2 texelSize;
    u_Texture.GetDimensions(texelSize.x, texelSize.y);
    texelSize = 1.0f / texelSize;
    
    float4 color = u_Texture.Sample(u_LinearClamp, TexCoord);
    
    float4 result;
    result.xy = float2(100.0f, 100.0f);
    result.z = ScreenDistance(result.xy, texelSize);
    result.a = color.a > 0.5f ? 1.0f : 0.0f;
    return result;
}
