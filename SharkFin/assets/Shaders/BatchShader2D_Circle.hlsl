#type Vertex
#version vs_4_0

cbuffer SceneData : register(b0)
{
    matrix ViewProjection;
}

struct Output
{
    float2 LocalPosition : LocalPosition;
    float4 Color : Color;
    float HalthThickness : HalthThickness;
    float2 TexCoord : TexCoord;
    float TilingFactor : TilingFactor;
    int TextureIndex : TextureIndex;
    int ID : ID;
    float4 Position : SV_POSITION;
};

Output main(
    float3 Position : Position,
    float2 LocalPosition : LocalPosition,
    float4 Color : Color,
    float Thickness : Thickness,
    float2 TexCoord : TexCoord,
    float TilingFactor : TilingFactor,
    int TextureIndex : TextureIndex,
    int ID : ID
)
{
    Output output;
    output.Position = mul(ViewProjection, float4(Position, 1.0f));
    output.LocalPosition = LocalPosition;
    output.Color = Color;
    output.TexCoord = TexCoord;
    output.TextureIndex = TextureIndex;
    output.TilingFactor = TilingFactor;
    output.HalthThickness = Thickness * 0.5f;
    output.ID = ID;

    return output;
}


#type Pixel
#version ps_4_0

Texture2D g_Textures[16] : register(t0);
SamplerState g_SamplerState[16];

struct Input
{
    float2 LocalPosition : LocalPosition;
    float4 Color : Color;
    float HalthThickness : HalthThickness;
    float2 TexCoord : TexCoord;
    float TilingFactor : TilingFactor;
    int TextureIndex : TextureIndex;
    int ID : ID;
};

struct Targets
{
    float4 Color : SV_TARGET0;
    int ID : SV_Target1;
};

Targets main(Input input)
{
    Targets targets;
    
    float dist = length(input.LocalPosition);
    if (dist > 0.5f || dist < 0.5f - input.HalthThickness)
        discard;
    
    switch (input.TextureIndex)
    {
        case  0: targets.Color = g_Textures[ 0].Sample(g_SamplerState[ 0], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  1: targets.Color = g_Textures[ 1].Sample(g_SamplerState[ 1], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  2: targets.Color = g_Textures[ 2].Sample(g_SamplerState[ 2], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  3: targets.Color = g_Textures[ 3].Sample(g_SamplerState[ 3], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  4: targets.Color = g_Textures[ 4].Sample(g_SamplerState[ 4], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  5: targets.Color = g_Textures[ 5].Sample(g_SamplerState[ 5], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  6: targets.Color = g_Textures[ 6].Sample(g_SamplerState[ 6], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  7: targets.Color = g_Textures[ 7].Sample(g_SamplerState[ 7], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  8: targets.Color = g_Textures[ 8].Sample(g_SamplerState[ 8], input.TexCoord * input.TilingFactor) * input.Color; break;
        case  9: targets.Color = g_Textures[ 9].Sample(g_SamplerState[ 9], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 10: targets.Color = g_Textures[10].Sample(g_SamplerState[10], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 11: targets.Color = g_Textures[11].Sample(g_SamplerState[11], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 12: targets.Color = g_Textures[12].Sample(g_SamplerState[12], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 13: targets.Color = g_Textures[13].Sample(g_SamplerState[13], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 14: targets.Color = g_Textures[14].Sample(g_SamplerState[14], input.TexCoord * input.TilingFactor) * input.Color; break;
        case 15: targets.Color = g_Textures[15].Sample(g_SamplerState[15], input.TexCoord * input.TilingFactor) * input.Color; break;
    }
    
    targets.ID = input.ID;
    
    return targets;
}