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
    float2 texCoord : TexCoord;
    float4 pos : SV_POSITION;
};

VSOUT main(float3 pos : Position, float2 texCoord : TexCoord)
{
    VSOUT vsout;
    matrix mat = mul(ViewProjection, Translation);
    vsout.pos = mul(mat, float4(pos, 1.0f));
    vsout.texCoord = texCoord;
    return vsout;
}


#type Pixel

Texture2D t2d;
SamplerState smpl;

float4 main(float2 texCoord : TexCoord) : SV_TARGET
{
    return t2d.Sample(smpl, texCoord);
}
