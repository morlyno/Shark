#pragma stage : compute

#define GROUP_SIZE 16
#define NUM_LODS 4

float4 reduce(float4 a, float4 b)
{
    return lerp(a, b, 0.5);
}
float4 reduce(float4 a, float4 b, float4 c, float4 d)
{
    return reduce(reduce(a, b), reduce(c, d));
}

struct Settings
{
    uint Dispatch;
    uint LODs;
};

[[vk::push_constant]]
ConstantBuffer<Settings> u_Settings;

Texture2D<float4> u_Source : register(t0);
RWTexture2D<float4> o_Mips[NUM_LODS] : register(u0);

groupshared float4 s_ReductionData[GROUP_SIZE][GROUP_SIZE];

[numthreads(16, 16, 1)]void main(
    uint2 groupIdx : SV_GroupID,
    uint2 globalIdx : SV_DispatchThreadID,
    uint2 threadIdx : SV_GroupThreadID)
{
    uint texWidth, texHeight;
    u_Source.GetDimensions(texWidth, texHeight);

    float4 value = u_Source.mips[0][min(globalIdx.xy, uint2(texWidth - 1, texHeight - 1))];


    [unroll]
    for (uint level = 1; level <= NUM_LODS; level++)
    {
        if (level == u_Settings.LODs + 1)
            break;
        
        uint outGroupSize = uint(GROUP_SIZE) >> level;
        uint inGroupSize = outGroupSize << 1;

        if (all(threadIdx.xy < inGroupSize))
        {
            s_ReductionData[threadIdx.y][threadIdx.x] = value;
        }

        GroupMemoryBarrierWithGroupSync();

        if (all(threadIdx.xy < outGroupSize))
        {
            float4 a = s_ReductionData[threadIdx.y * 2 + 0][threadIdx.x * 2 + 0];
            float4 b = s_ReductionData[threadIdx.y * 2 + 0][threadIdx.x * 2 + 1];
            float4 c = s_ReductionData[threadIdx.y * 2 + 1][threadIdx.x * 2 + 0];
            float4 d = s_ReductionData[threadIdx.y * 2 + 1][threadIdx.x * 2 + 1];

            value = reduce(a, b, c, d);
            o_Mips[level - 1][groupIdx.xy * outGroupSize + threadIdx.xy] = value;
        }
        
        GroupMemoryBarrierWithGroupSync();
    }
}