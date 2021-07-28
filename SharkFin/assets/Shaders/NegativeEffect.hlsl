#type vertex
#version vs_4_0

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

#type pixel
#version ps_4_0

Texture2D Frame;
SamplerState Sampler;

float4 main(float2 texCoords : TexCoords) : SV_Target0
{
    return 1.0f - Frame.Sample(Sampler, texCoords);
}
