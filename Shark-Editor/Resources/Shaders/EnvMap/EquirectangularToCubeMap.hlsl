#pragma layout : renderpass
#pragma stage : compute

#include "EnvMapCommon.hlslh"
#include "Core/Samplers.hlslh"

RWTexture2DArray<float4> o_CubeMap : register(u0);
Texture2D<float4> u_Equirect : register(t0);

[numthreads(32, 32, 1)]
void main(uint3 dispatchID : SV_DispatchThreadID)
{
    float3 cubeSize;
    o_CubeMap.GetDimensions(cubeSize.x, cubeSize.y, cubeSize.z);
    float3 cubeTC = GetCubeMapTexCoord(dispatchID, cubeSize.xy);
    
    float phi = atan2(cubeTC.z, cubeTC.x);
    float theta = acos(cubeTC.y);
    float2 uv = float2(phi / TwoPI + 0.5f, theta / PI);
    
    float4 color = u_Equirect.SampleLevel(u_LinearClamp, uv, 0);
    color = min(color, (float4)100.0f);
    
    o_CubeMap[dispatchID] = color;
}
