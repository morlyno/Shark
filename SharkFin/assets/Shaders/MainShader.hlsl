#type Vertex

cbuffer SceanData : register(b0)
{
    matrix ViewProjection;
}

struct VSIN
{
    float3 Pos : Position;
    float4 Color : Color;
    float2 TexCoord : TexCoord;
    int TexIndex : TextureIndex;
    float TilingFactor : TilingFactor;
};

struct VSOUT
{
    float4 Color : Color;
    float2 TexCoord : TexCoord;
    int TexIndex : TextureIndex;
    float TilingFactor : TilingFactor;
    float4 Pos : SV_POSITION;
};

VSOUT main(VSIN vsin)
{
    VSOUT vsout;
    vsout.Pos = mul(ViewProjection, float4(vsin.Pos, 1.0f));
    vsout.Color = vsin.Color;
    vsout.TexCoord = vsin.TexCoord;
    vsout.TexIndex = vsin.TexIndex;
    vsout.TilingFactor = vsin.TilingFactor;
    return vsout;
}


#type Pixel

Texture2D g_Textures[16] : register(t0);
SamplerState g_SamplerState[16];

struct PSIN
{
    float4 Color : Color;
    float2 TexCoord : TexCoord;
    int TexIndex : TextureIndex;
    float TilingFactor : TilingFactor;
};

float4 main(PSIN psin) : SV_TARGET
{
    float4 color = { 0.2f, 0.4f, 0.6f, 0.8f };
    switch (psin.TexIndex)
    {
        case 0: color = g_Textures[0].Sample(g_SamplerState[0], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 1: color = g_Textures[1].Sample(g_SamplerState[1], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 2: color = g_Textures[2].Sample(g_SamplerState[2], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 3: color = g_Textures[3].Sample(g_SamplerState[3], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 4: color = g_Textures[4].Sample(g_SamplerState[4], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 5: color = g_Textures[5].Sample(g_SamplerState[5], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 6: color = g_Textures[6].Sample(g_SamplerState[6], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 7: color = g_Textures[7].Sample(g_SamplerState[7], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 8: color = g_Textures[8].Sample(g_SamplerState[8], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 9: color = g_Textures[9].Sample(g_SamplerState[9], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 10: color = g_Textures[10].Sample(g_SamplerState[10], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 11: color = g_Textures[11].Sample(g_SamplerState[11], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 12: color = g_Textures[12].Sample(g_SamplerState[12], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 13: color = g_Textures[13].Sample(g_SamplerState[13], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 14: color = g_Textures[14].Sample(g_SamplerState[14], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 15: color = g_Textures[15].Sample(g_SamplerState[15], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
    }
    return color;
}