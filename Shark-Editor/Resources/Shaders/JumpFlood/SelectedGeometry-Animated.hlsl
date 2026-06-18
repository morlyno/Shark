#pragma layout : renderpass

// Implemented based on this article
// https://bgolus.medium.com/the-quest-for-very-wide-outlines-ba82ed442cd9

#pragma stage : vertex

#include "Core/Bindings/Camera.hlslh"

struct Mesh
{
    matrix Transform;
    
    uint BoneBase;
    uint BoneStride;
    float P0, P1;
};

struct BoneTransforms
{
    matrix Transform;
};

[[vk::push_constant]]
ConstantBuffer<Mesh> u_Mesh;

StructuredBuffer<BoneTransforms> u_BoneTransforms : register(t2, space1);

struct VertexInput
{
    float3 Position : Position;
    float3 Normal : Normal;
    float3 Tangent : Tangent;
    float3 Bitangent : Bitangent;
    float2 Texcoord : Texcoord;
    
    int4 BoneIDs : BoneIDs;
    float4 BoneWeights : BoneWeights;
};

float4 main(VertexInput Input) : SV_Position
{
    matrix boneTransform = mul(u_BoneTransforms[u_Mesh.BoneBase + Input.BoneIDs.x].Transform, Input.BoneWeights.x);
    boneTransform += mul(u_BoneTransforms[u_Mesh.BoneBase + Input.BoneIDs.y].Transform, Input.BoneWeights.y);
    boneTransform += mul(u_BoneTransforms[u_Mesh.BoneBase + Input.BoneIDs.z].Transform, Input.BoneWeights.z);
    boneTransform += mul(u_BoneTransforms[u_Mesh.BoneBase + Input.BoneIDs.w].Transform, Input.BoneWeights.w);
    
    matrix transform = mul(u_Mesh.Transform, boneTransform);
    return mul(u_Camera.ViewProjection, mul(transform, float4(Input.Position, 1.0f)));
}

#pragma stage : pixel

float4 main() : SV_Target
{
    return (float4) 1.0f;
}
