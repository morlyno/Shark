#pragma once

struct Camera
{
    matrix ViewProjection;
    float3 Position;
    float Padding;
};

struct MeshData
{
    matrix Transform;
    int ID;
    float P0, P1, P2;
};

[[vk::push_constant]]
ConstantBuffer<MeshData> u_MeshData;
ConstantBuffer<Camera> u_Camera : register(b0, space1);

struct PointLight
{
    float3 Position;
    float Intensity;
    float3 Radiance;
    float Radius;
    float Falloff;
    float P0, P1, P2;
};

struct DirectionalLight
{
    float4 Radiance;
    float3 Direction;
    float Intensity;
};

struct Scene
{
    uint PointLightCount;
    uint DirectionalLightCount;
    float EnvironmentMapIntensity;
    float P0;
};

ConstantBuffer<Scene> u_Scene : register(b1, space1);
StructuredBuffer<PointLight> u_PointLights : register(t0, space1);
StructuredBuffer<DirectionalLight> u_DirectionalLights : register(t1, space1);