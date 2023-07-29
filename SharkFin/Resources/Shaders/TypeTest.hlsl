#pragma stage : Vertex

cbuffer c_SceneData : register(b0)
{
    matrix ViewProjection;
}

cbuffer c_MeshData : register(b1)
{
    matrix Transform;
    int ID;
    int3 test;
};

struct VSOutput
{
    float4 Color : Color;
    int ID : ID;
    float4 Position : SV_POSITION;
};

VSOutput main(float3 position : Position)
{
    VSOutput output;
    output.Position = mul(Transform, float4(position, 1.0f));
    output.Position = mul(ViewProjection, output.Position);
    //output.Position = mul(Transform, float4(position, 1.0f));
    //output.Position = mul(ViewProjection, output.Position);
    float3 color = position;
    color += float3(1.0f, 1.0f, 1.0f);
    color *= 0.5f;
    output.Color = float4(color, 1.0f);
    output.ID = ID;
    return output;
}

#pragma stage : Pixel

#include "SKTexture2D.hlsl"

SKTexture2D u_Texture;

struct PSOutput
{
    float4 Color0 : SV_Target0;
    int ID : SV_Target3;
};

PSOutput main(float4 color : Color, int id : ID, float2 uv : UV)
{
    PSOutput output;
    //output.Color0 = u_Texture.Sample(uv);
    output.Color0 = SampleTexture(u_Texture, uv);
    //output.Color0 = u_Texture.Sample(u_Sampler, uv);
    output.ID = id;
    return output;
}
