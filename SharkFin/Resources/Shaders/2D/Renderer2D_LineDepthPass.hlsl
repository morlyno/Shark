#pragma stage : Vertex

// begin_metadata
// set u_SceneData RenderPass
// end_metadata

cbuffer u_SceneData : register(b0)
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

#pragma stage : Pixel

void main()
{
}