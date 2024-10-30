
// Implemented based on this article
// https://bgolus.medium.com/the-quest-for-very-wide-outlines-ba82ed442cd9

#pragma stage : vertex

struct Camera
{
    matrix ViewProjection;
    float3 Position;
    float Padding;
};

struct Mesh
{
    matrix Transform;
};

[[vk::binding(0, 1)]] ConstantBuffer<Camera> u_Camera;
[[vk::push_constant]] ConstantBuffer<Mesh> u_Mesh;

struct VertexInput
{
    float3 Position : Position;
    float3 Normal : Normal;
    float3 Tangent : Tangent;
    float3 Bitangent : Bitangent;
    float2 Texcoord : Texcoord;
};

float4 main(VertexInput Input) : SV_Position
{
    return mul(u_Camera.ViewProjection, mul(u_Mesh.Transform, float4(Input.Position, 1.0f)));
}

#pragma stage : pixel

float4 main() : SV_Target
{
    return (float4)1.0f;
}
