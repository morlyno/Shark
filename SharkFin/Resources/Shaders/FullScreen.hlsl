#pragma stage : vertex

struct VSOUT
{
    float2 TexCoords : TexCoords;
    float4 Position : SV_Position;
};

VSOUT main(float2 pos : Position)
{
    VSOUT vsout;
    vsout.Position = float4(pos, 0.0f, 1.0f);
    vsout.TexCoords = float2((pos.x + 1.0f) * 0.5f, 1.0f - ((pos.y + 1.0f) * 0.5f));
    return vsout;
}

#pragma stage : pixel

[[vk::binding(0, 1), vk::combinedImageSampler]] Texture2D u_SourceImage;
[[vk::binding(0, 1), vk::combinedImageSampler]] SamplerState u_SourceImageSampler;

float4 main(float2 texCoords : TexCoords) : SV_Target0
{
    return u_SourceImage.Sample(u_SourceImageSampler, texCoords);
}
