#pragma stage : vertex

//   !!!   !!!   !!!   !!!   !!!
// Out of date
// Use Renderer2D_Quad.glsl instead
//   !!!   !!!   !!!   !!!   !!!

struct Camera
{
    matrix ViewProjection;
};

[[vk::binding(0, 1)]] ConstantBuffer<Camera> u_Camera;

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
    vsout.Pos = mul(u_Camera.ViewProjection, float4(vsin.Pos, 1.0f));
    vsout.Color = vsin.Color;
    vsout.TexCoord = vsin.TexCoord;
    vsout.TexIndex = vsin.TexIndex;
    vsout.TilingFactor = vsin.TilingFactor;
    vsout.ID = vsin.ID;
    return vsout;
}

#pragma stage : Pixel

[[vk::binding(0, 0)]][[vk::combinedImageSampler]] Texture2D u_Textures[16];
[[vk::binding(0, 0)]][[vk::combinedImageSampler]] SamplerState u_Samplers[16];

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
        case  0: psout.Color = u_Textures[ 0].Sample(u_Samplers[ 0], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  1: psout.Color = u_Textures[ 1].Sample(u_Samplers[ 1], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  2: psout.Color = u_Textures[ 2].Sample(u_Samplers[ 2], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  3: psout.Color = u_Textures[ 3].Sample(u_Samplers[ 3], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  4: psout.Color = u_Textures[ 4].Sample(u_Samplers[ 4], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  5: psout.Color = u_Textures[ 5].Sample(u_Samplers[ 5], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  6: psout.Color = u_Textures[ 6].Sample(u_Samplers[ 6], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  7: psout.Color = u_Textures[ 7].Sample(u_Samplers[ 7], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  8: psout.Color = u_Textures[ 8].Sample(u_Samplers[ 8], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case  9: psout.Color = u_Textures[ 9].Sample(u_Samplers[ 9], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 10: psout.Color = u_Textures[10].Sample(u_Samplers[10], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 11: psout.Color = u_Textures[11].Sample(u_Samplers[11], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 12: psout.Color = u_Textures[12].Sample(u_Samplers[12], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 13: psout.Color = u_Textures[13].Sample(u_Samplers[13], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 14: psout.Color = u_Textures[14].Sample(u_Samplers[14], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
        case 15: psout.Color = u_Textures[15].Sample(u_Samplers[15], psin.TexCoord * psin.TilingFactor) * psin.Color; break;
    }
    psout.ID = psin.ID;
    psout.Color = float4(psout.Color.xyz, 1.0f);
    
    return psout;
}