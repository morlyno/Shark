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

Texture2D Frame : register(t0);
Texture2D DepthImage : register(t1);
SamplerState Sampler : register(s0);

struct PSOUT
{
    float4 Color : SV_Target;
    float Depth : SV_Depth;
};

PSOUT main(float2 texCoords : TexCoords)
{
    PSOUT psout;
    psout.Color = Frame.Sample(Sampler, texCoords);
    psout.Depth = DepthImage.Sample(Sampler, texCoords);
    return psout;
}
