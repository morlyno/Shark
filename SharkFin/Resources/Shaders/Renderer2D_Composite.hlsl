#type vertex
#version vs_4_0

struct VSOUT
{
    float2 TexCoords : TexCoords;
    float4 Position : SV_Position;
};

VSOUT main(float2 pos : Position)
{
    VSOUT vsout;
    vsout.Position = float4(pos, 0.0f, 1.0f);
    vsout.TexCoords = float2((pos.x + 1.0f) * 0.5f, 1.0f - ((pos.y + 1.0f) * 0.5f));
    return vsout;
}

#type pixel
#version ps_4_0

Texture2D AccumulationImage : register(t0);
Texture2D<float> RevealImage : register(t1);
Texture2D<int> IDImage : register(t2);
Texture2D DepthImage : register(t3);

struct PSOUT
{
    float4 Color : SV_Target0;
    int ID : SV_Target1;
    float Depth : SV_Depth;
};

float maxComponent(float4 v4)
{
    return max(v4.x, max(v4.y, max(v4.z, v4.w)));
}

PSOUT main(float2 texCoords : TexCoords)
{
    PSOUT psout;
    
    uint2 dimension;
    AccumulationImage.GetDimensions(dimension.x, dimension.y);
    uint2 pixel = dimension * texCoords;
    
    float revealage = RevealImage.Load(int3(pixel, 0));
    if (revealage == 1.0f)
        discard;
    
    float4 accum = AccumulationImage.Load(int3(pixel, 0));
    
    if (isinf(maxComponent(abs(accum))))
        accum.rgb = float3(accum.a, accum.a, accum.a);
    
    float3 averageColor = accum.rgb / max(accum.a, 0.00001f);
    psout.Color = float4(averageColor, 1.0f - revealage);
    
    psout.ID = IDImage.Load(int3(pixel, 0));
    psout.Depth = DepthImage.Load(int3(pixel, 0));
    
    return psout;
}
