#pragma layout : renderpass
#pragma stage : compute

#include "Core/Texture.hlslh"

struct BlitParams
{
    float2 src0;
    float2 src1;
    float2 dst0;
    float2 dst1;
};

Texture2DArray u_Input : register(t0);
RWTexture2DArray<float4> o_Output : register(u0);
SamplerState u_Sampler : register(s0);

[[vk::push_constant]]
ConstantBuffer<BlitParams> u_Params;

[numthreads(8, 8, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    float2 dstSize = u_Params.dst1 - u_Params.dst0;
    if (any(dispatchID.xy >= dstSize))
        return;
    
    float2 srcSize = u_Params.src1 - u_Params.src0;
    float2 scale = srcSize / dstSize;
    
    float2 t = float2(dispatchID.xy) * scale / srcSize;
    float2 uv = lerp(u_Params.src0, u_Params.src1, t) / TextureSize(u_Input);
    
    float4 color = u_Input.SampleLevel(u_Sampler, float3(uv, dispatchID.z), 0);
    o_Output[dispatchID + float3(u_Params.dst0, 0)] = color;
}
