#pragma once

// PBR Material Bindings

struct MaterialUniforms
{
    float3 Albedo;
    float Metalness;
    float Roughness;
    float AmbientOcclusion;
    bool UsingNormalMap;
    float P0;
};

ConstantBuffer<MaterialUniforms> u_MaterialUniforms : register(b0, space0);

uniform Texture2D u_AlbedoMap : register(t0, space0);
uniform Texture2D u_NormalMap : register(t1, space0);
uniform Texture2D u_MetalnessMap : register(t2, space0);
uniform Texture2D u_RoughnessMap : register(t3, space0);

uniform SamplerState u_AlbedoSampler : register(s0, space0);
uniform SamplerState u_LinearClamp : register(s1, space0);
