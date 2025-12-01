#pragma stage : compute

#include "EnvMapCommon.hlslh"

struct Uniforms
{
    uint Samples;
};

[[vk::push_constant]]
ConstantBuffer<Uniforms> u_Push : register(b0);

RWTexture2DArray<float4> o_Irradiance : register(u0);
TextureCube<float4> u_Radiance : register(t0);
SamplerState u_Sampler : register(s0);

[numthreads(32, 32, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    float3 irradianceSize;
    o_Irradiance.GetDimensions(irradianceSize.x, irradianceSize.y, irradianceSize.z);
    float3 N = GetCubeMapTexCoord(dispatchID, irradianceSize.xy);
    
    float3 S, T;
    ComputeBasisVectors(N, S, T);
    
    uint samples = 64 * u_Push.Samples;
    
    float3 irradiance = 0.0f;
    for (uint i = 0; i < samples; i++)
    {
        float2 u = SampleHammersley(i, samples);
        float3 Li = TangentToWorld(SampleHemisphere(u.x, u.y), N, S, T);
        float cosTheta = max(dot(Li, N), 0.0f);
        
        irradiance += 2.0f * u_Radiance.SampleLevel(u_Sampler, Li, 0).rgb * cosTheta;
    }

    irradiance /= samples;
    o_Irradiance[dispatchID] = float4(irradiance, 1.0f);
}
