#type Vertex
#version vs_4_0

cbuffer SceneData : register(b0)
{
    matrix ViewProjection;
}

struct VSIN
{
    float3 Pos : Position;
    float4 Color : Color;
    float2 TexCoord : TexCoord;
};

struct VSOUT
{
    float4 Color : Color;
    float2 TexCoord : TexCoord;
    float4 Pos : SV_POSITION;
};

VSOUT main(VSIN vsin)
{
    VSOUT vsout;
    vsout.Pos = mul(ViewProjection, float4(vsin.Pos, 1.0f));
    vsout.Color = vsin.Color;
    vsout.TexCoord = vsin.TexCoord;
    return vsout;
}


#type Pixel
#version ps_4_0

Texture2D g_FontAtlas : register(t0);
SamplerState g_Sampler;

struct PSIN
{
    float4 Color : Color;
    float2 TexCoord : TexCoord;
};

struct PSOUT
{
    float4 Color : SV_TARGET0;
    int ID : SV_Target1;
};

float median(float r, float g, float b)
{
    return max(min(r, g), min(max(r, g), b));
}

float screenPxRange(float2 texCoord, float pxRange)
{
    float2 textureSize;
    g_FontAtlas.GetDimensions(textureSize.x, textureSize.y);
    
    float2 unitRange = float2(pxRange, pxRange) / textureSize;
    float2 screenTexSize = float2(1.0f, 1.0f) / fwidth(texCoord);
    return max(0.5 * dot(unitRange, screenTexSize), 1.0);
}

PSOUT main(PSIN psin)
{
    PSOUT psout;
    
    float3 msd = g_FontAtlas.Sample(g_Sampler, psin.TexCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = screenPxRange(psin.TexCoord, 2) * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    //color = mix(bgColor, fgColor, opacity);
    
    if (opacity == 0.0f)
        discard;
    
    psout.Color = float4(psin.Color.rgb, opacity);
    psout.Color = float4(0.2f, 0.8f, 0.6f, opacity);
    psout.ID = -1;
    
    return psout;
}