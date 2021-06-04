#type Vertex
#version vs_4_0

cbuffer SceneData : register(b0)
{
    matrix c_ViewProjection;
}

cbuffer Material : register(b1)
{
    matrix c_Transform;
    float4 c_Color;
    float c_TilingFactor;
    int c_ID;
}

struct VSIN
{
    float3 Pos : Position;
    float2 TexCoord : TexCoord;
};

struct VSOUT
{
    float4 Color : Color;
    float2 TexCoord : TexCoord;
    float TilingFactor : TilingFactor;
    int ID : ID;
    float4 Pos : SV_POSITION;
};

VSOUT main(VSIN vsin)
{
    VSOUT vsout;
	vsout.Pos = float4(vsin.Pos, 1.0f);
    vsout.Pos = mul(c_Transform, vsout.Pos);
    vsout.Pos = mul(c_ViewProjection, vsout.Pos);
    vsout.Color = c_Color;
    vsout.TexCoord = vsin.TexCoord;
    vsout.TilingFactor = c_TilingFactor;
    vsout.ID = c_ID;
    return vsout;
}


#type Pixel
#version ps_4_0

Texture2D in_Texture : register(t0);
SamplerState in_SamplerState : register(s0);

struct PSIN
{
    float4 Color : Color;
    float2 TexCoord : TexCoord;
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

    psout.Color = in_Texture.Sample(in_SamplerState, psin.TexCoord * psin.TilingFactor) * psin.Color;
	//psout.Color = float4(0.2f, 0.8f, 0.2f, 1.0f);
    psout.ID = psin.ID;

    return psout;
}