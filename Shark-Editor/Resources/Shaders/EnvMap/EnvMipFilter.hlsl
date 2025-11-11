#pragma stage : compute

#include "EnvMapCommon.hlsl"

static const uint SampleCount = 1024;
static const float InvSampleCount = 1.0f / float(SampleCount);
static const int MipLevelCount = 1;

struct Uniforms
{
    float Roughness;
};

[[vk::push_constant]]
ConstantBuffer<Uniforms> u_Push : register(b0);

RWTexture2DArray<float4> o_Filtered : register(u0, space0);
TextureCube<float4> u_Unfiltered : register(t0, space1);
SamplerState u_Sampler : register(s0, space1);

[numthreads(32, 32, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    int3 outputSize;
    o_Filtered.GetDimensions(outputSize.x, outputSize.y, outputSize.z);
    if (dispatchID.x >= outputSize.x || dispatchID.y >= outputSize.y)
        return;
    
    float2 inputSize;
    u_Unfiltered.GetDimensions(inputSize.x, inputSize.y);
    float wt = 4.0f * PI / (6 * inputSize.x * inputSize.y);
    
    float3 N = GetCubeMapTexCoord(dispatchID, outputSize.xy);
    float3 Lo = N;
    
    float3 S, T;
    ComputeBasisVectors(N, S, T);
    
    float3 color = 0.0f;
    float weight = 0.0f;
    
    for (uint i = 0; i < SampleCount; i++)
    {
        float2 u = SampleHammersley(i, SampleCount);
        float3 Lh = TangentToWorld(SampleGGX(u.x, u.y, u_Push.Roughness), N, S, T);

        float3 Li = 2.0f * dot(Lo, Lh) * Lh - Lo;
        
        float cosLi = dot(N, Li);
        if (cosLi > 0.0f)
        {
            float cosLh = max(dot(N, Lh), 0.0f);
            float pdf = NDFGGX(cosLh, u_Push.Roughness) * 0.25f;
            float ws = 1.0f / (SampleCount * pdf);
            float mipLevel = max(0.5f * log2(ws / wt) + 1.0f, 0.0f);
            
            color += u_Unfiltered.SampleLevel(u_Sampler, Li, mipLevel).rgb * cosLi;
            weight += cosLi;
        }
    }
    color /= weight;

    o_Filtered[dispatchID] = float4(color, 1.0f);
}
