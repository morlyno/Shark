
struct SKTexture2D
{
    Texture2D HLSLTexture;
    SamplerState HLSLSampler;
};

float4 SampleTexture(SKTexture2D tex, float2 uv)
{
    return tex.HLSLTexture.Sample(tex.HLSLSampler, uv);
}
