
#type Vertex
#version vs_4_0

cbuffer SceneData : register(b0)
{
    matrix ViewProjection;
}

struct Output
{
    float4 Position : SV_POSITION;
};

Output main(
    float3 Position : Position,
    float4 Color : Color,
    int ID : ID
)
{
    Output output;
    output.Position = mul(ViewProjection, float4(Position, 1.0f));
    return output;
}

#type Pixel
#version ps_4_0

void main()
{
}