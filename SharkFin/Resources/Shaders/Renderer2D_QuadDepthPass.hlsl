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
    int TexIndex : TextureIndex;
    float TilingFactor : TilingFactor;
    int ID : ID;
};

struct VSOUT
{
    float4 Pos : SV_POSITION;
};

VSOUT main(VSIN vsin) : SV_POSITION
{
    VSOUT vsout;
    vsout.Pos = mul(ViewProjection, float4(vsin.Pos, 1.0f));
    return vsout;
}


#type Pixel
#version ps_4_0

void main()
{
}