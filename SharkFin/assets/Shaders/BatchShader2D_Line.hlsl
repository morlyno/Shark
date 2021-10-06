
#type Vertex
#version vs_4_0

cbuffer SceneData : register(b0)
{
    matrix ViewProjection;
}

struct Output
{
    float4 Color : Color;
    int ID : ID;
    float4 Position : SV_POSITION;
};

Output main(
    float3 Position : Position,
    float4 Color : Color,
    int ID : ID
)
{
    Output output;
    output.Position = mul(ViewProjection, float4(Position, 1.0f));
    output.Color = Color;
    output.ID = ID;
    return output;
}

#type Pixel
#version ps_4_0

struct Targets
{
    float4 Color : SV_Target0;
    int ID : SV_Target1;
};

struct Input
{
    float4 Color : Color;
    int ID : ID;
};

Targets main(Input input)
{
    Targets targets;
    targets.Color = input.Color;
    targets.ID = input.ID;
    return targets;
}