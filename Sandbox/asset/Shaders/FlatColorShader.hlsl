#type Vertex

cbuffer SceanData : register(b0)
{
    matrix ViewProjection;
}

cbuffer ObjectData : register(b1)
{
    matrix Translation;
}

struct VSOUT
{
    float4 color : Color;
    float4 pos : SV_POSITION;
};

VSOUT main(float3 pos : Position, float4 color : Color)
{
    VSOUT vsout;
    matrix mat = mul(ViewProjection, Translation);
    vsout.pos = mul(mat, float4(pos, 1.0f));
    vsout.color = color;
    return vsout;
}


#type Pixel

float4 main(float4 color : Color) : SV_TARGET
{
    return color;
}
