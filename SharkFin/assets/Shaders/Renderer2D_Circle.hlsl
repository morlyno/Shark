#type Vertex
#version vs_4_0

cbuffer SceneData : register(b0)
{
    matrix ViewProjection;
}

struct Output
{
    float2 LocalPosition : LocalPosition;
    float4 Color : Color;
    float Thickness : Thickness;
    int ID : ID;
    float4 WorldPosition : SV_POSITION;
};

Output main(
    float3 WorldPosition : WorldPosition,
    float2 LocalPosition : LocalPosition,
    float4 Color : Color,
    float Thickness : Thickness,
    int ID : ID
)
{
    Output output;
    output.WorldPosition = mul(ViewProjection, float4(WorldPosition, 1.0f));
    output.LocalPosition = LocalPosition;
    output.Color = Color;
    output.Thickness = Thickness;
    output.ID = ID;

    return output;
}


#type Pixel
#version ps_4_0

struct Input
{
    float2 LocalPosition : LocalPosition;
    float4 Color : Color;
    float Thickness : Thickness;
    int ID : ID;
};

struct Targets
{
    float4 Color : SV_TARGET0;
    int ID : SV_Target1;
};

Targets main(Input input)
{
    Targets targets;
    
    float fade = 0.004f;
    float dist = length(input.LocalPosition) * 2.0f;
    if (dist > 1.0f || dist < 1.0f - input.Thickness)
        discard;
    
    float alpha = 1.0f - smoothstep(1.0f - fade, 1.0f, dist);
    alpha *= smoothstep(1.0f - input.Thickness, 1.0f - input.Thickness + fade, dist);
    
    targets.Color = input.Color;
    targets.Color.a *= alpha;
    
    targets.ID = input.ID;
    
    return targets;
}