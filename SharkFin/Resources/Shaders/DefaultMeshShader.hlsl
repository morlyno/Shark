#pragma stage : Vertex

cbuffer c_SceneData : register(b0)
{
    matrix ViewProjection;
}

cbuffer c_MeshData : register(b1)
{
    matrix Transform;
    int ID;
};

struct VSOutput
{
    float4 Color : Color;
    int ID : ID;
    float4 Position : SV_POSITION;
};

VSOutput main(float3 position : Position, float4 color : Color)
{
    VSOutput output;
    output.Position = mul(Transform, float4(position, 1.0f));
    output.Position = mul(ViewProjection, output.Position);
    output.Color = color;
    output.ID = ID;
    return output;
}

#pragma stage : Pixel

struct PSOutput
{
    float4 Color : SV_Target0;
    int ID : SV_Target1;
};

PSOutput main(float4 color : Color, int id : ID)
{
    PSOutput output;
    output.Color = color;
    output.ID = id;
    return output;
}
