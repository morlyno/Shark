#pragma stage : vertex

// begin_metadata
// set u_SceneData PerScene
// end_metadata

cbuffer u_SceneData : register(b0)
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
    int ID : ID;
};

struct VSOUT
{
    float4 Color : Color;
    float2 TexCoord : TexCoord;
    int TexIndex : TextureIndex;
    float TilingFactor : TilingFactor;
    int ID : ID;
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
    vsout.ID = vsin.ID;
    return vsout;
}

#pragma stage : Pixel

Texture2D g_Textures[16] : register(t0);
SamplerState g_SamplerState[16];

struct PSIN
{
    float4 Color : Color;
    float2 TexCoord : TexCoord;
    int TexIndex : TextureIndex;
    float TilingFactor : TilingFactor;
    int ID : ID;
};

struct PSOUT
{
    float4 Color : SV_TARGET0;
    int ID : SV_Target1;
};

PSOUT main(PSIN psin)
{
    PSOUT psout;
    
    switch (psin.TexIndex)
    {
        case  0: psout.Color = g_Textures[ 0].Sample(g_SamplerState[ 0], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  1: psout.Color = g_Textures[ 1].Sample(g_SamplerState[ 1], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  2: psout.Color = g_Textures[ 2].Sample(g_SamplerState[ 2], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  3: psout.Color = g_Textures[ 3].Sample(g_SamplerState[ 3], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  4: psout.Color = g_Textures[ 4].Sample(g_SamplerState[ 4], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  5: psout.Color = g_Textures[ 5].Sample(g_SamplerState[ 5], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  6: psout.Color = g_Textures[ 6].Sample(g_SamplerState[ 6], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  7: psout.Color = g_Textures[ 7].Sample(g_SamplerState[ 7], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  8: psout.Color = g_Textures[ 8].Sample(g_SamplerState[ 8], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  9: psout.Color = g_Textures[ 9].Sample(g_SamplerState[ 9], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 10: psout.Color = g_Textures[10].Sample(g_SamplerState[10], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 11: psout.Color = g_Textures[11].Sample(g_SamplerState[11], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 12: psout.Color = g_Textures[12].Sample(g_SamplerState[12], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 13: psout.Color = g_Textures[13].Sample(g_SamplerState[13], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 14: psout.Color = g_Textures[14].Sample(g_SamplerState[14], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 15: psout.Color = g_Textures[15].Sample(g_SamplerState[15], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
    }
    psout.ID = psin.ID;
    psout.Color = float4(psout.Color.xyz, 1.0f);
    
    return psout;
}