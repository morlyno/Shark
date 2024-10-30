
#pragma stage : vertex

struct VertexOutput
{
    float2 TexelSize : TexelSize;
    float2 UV[9] : UV;
    float4 Position : SV_Position;
};

struct PushConstant
{
    float2 TexelSize;
    int Step;
};
[[vk::push_constant]] ConstantBuffer<PushConstant> u_Uniforms;

VertexOutput main(float3 Position : Position, float2 TexCoord : TexCoord)
{
    VertexOutput output;
    output.TexelSize = u_Uniforms.TexelSize;
    
    float2 dx = float2(u_Uniforms.TexelSize.x, 0.0f) * u_Uniforms.Step;
    float2 dy = float2(0.0f, u_Uniforms.TexelSize.y) * u_Uniforms.Step;
    
    output.UV[0] = TexCoord;
    output.UV[1] = TexCoord + dx;
    output.UV[2] = TexCoord - dx;
    output.UV[3] = TexCoord + dy;
    output.UV[4] = TexCoord - dy;
    output.UV[5] = TexCoord + dx + dy;
    output.UV[6] = TexCoord + dx - dy;
    output.UV[7] = TexCoord - dx + dy;
    output.UV[8] = TexCoord - dx - dy;
    
    output.Position = float4(Position, 1.0f);
    return output;
}

#pragma stage : pixel

#include "JumpFlood/JumpFloodCommon.hlsl"

struct VertexOutput
{
    float2 TexelSize : TexelSize;
    float2 UV[9] : UV;
};

[[vk::binding(0, 1), vk::combinedImageSampler]] uniform Texture2D u_Texture;
[[vk::binding(0, 1), vk::combinedImageSampler]] uniform SamplerState u_Sampler;

float4 main(VertexOutput input) : SV_Target
{
    float4 pixel = u_Texture.Sample(u_Sampler, input.UV[0]);
    
    for (int i = 1; i <= 8; i++)
    {
        float4 n = u_Texture.Sample(u_Sampler, input.UV[i]);
        if (n.w != pixel.w)
            n.xyz = (float3)0.0f;
        
        n.xy += input.UV[i] - input.UV[0];
        
        BoundsCheck(n.xy, input.UV[i]);
        
        float dist = ScreenDistance(n.xy, input.TexelSize);
        if (dist < pixel.z)
            pixel.xyz = float3(n.xy, dist);
    }
    
    return pixel;
}
