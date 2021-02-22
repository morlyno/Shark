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

Texture2D g_Textures[32] : register(t0);
SamplerState g_SamplerState;

struct PSIN
{
    float4 Color : Color;
    float2 TexCoord : TexCoord;
    int TexIndex : TextureIndex;
    float TilingFactor : TilingFactor;
};

float4 main(PSIN psin) : SV_TARGET
{
    switch (psin.TexIndex)
    {
        case 0: return g_Textures[0].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 1: return g_Textures[1].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 2: return g_Textures[2].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 3: return g_Textures[3].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 4: return g_Textures[4].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 5: return g_Textures[5].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 6: return g_Textures[6].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 7: return g_Textures[7].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 8: return g_Textures[8].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 9: return g_Textures[9].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 10: return g_Textures[10].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 11: return g_Textures[11].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 12: return g_Textures[12].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 13: return g_Textures[13].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 14: return g_Textures[14].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 15: return g_Textures[15].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 16: return g_Textures[16].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 17: return g_Textures[17].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 18: return g_Textures[18].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 19: return g_Textures[19].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 20: return g_Textures[20].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 21: return g_Textures[21].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 22: return g_Textures[22].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 23: return g_Textures[23].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 24: return g_Textures[24].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 25: return g_Textures[25].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 26: return g_Textures[26].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 27: return g_Textures[27].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 28: return g_Textures[28].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 29: return g_Textures[29].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 30: return g_Textures[30].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
        case 31: return g_Textures[31].Sample(g_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
    }
    return float4(0.2f, 0.4f, 0.6f, 0.8f);
}
