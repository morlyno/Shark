#pragma stage : Vertex

// begin_metadata
// set u_SceneData RenderPass
// end_metadata

cbuffer u_SceneData : register(b0)
{
    matrix ViewProjection;
}

struct Output
{
    float2 LocalPosition : LocalPosition;
    float4 Color : Color;
    float Thickness : Thickness;
    float Fade : Fade;
    int ID : ID;
    float4 WorldPosition : SV_POSITION;
};

Output main(
    float3 WorldPosition : WorldPosition,
    float2 LocalPosition : LocalPosition,
    float4 Color : Color,
    float Thickness : Thickness,
    float Fade : Fade,
    int ID : ID
)
{
    Output output;
    output.WorldPosition = mul(ViewProjection, float4(WorldPosition, 1.0f));
    output.LocalPosition = LocalPosition;
    output.Color = Color;
    output.Thickness = Thickness;
    output.Fade = Fade;
    output.ID = ID;

    return output;
}


#pragma stage : Pixel

struct Input
{
    float2 LocalPosition : LocalPosition;
    float4 Color : Color;
    float Thickness : Thickness;
    float Fade : Fade;
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
    
    float dist = 1.0 - length(input.LocalPosition);
    //if (dist > 1.0f || dist < 1.0f - input.Thickness)
    //    discard;
    
    float alpha = smoothstep(0.0, input.Fade, dist);
    alpha *= smoothstep(input.Thickness + input.Fade, input.Thickness, dist);
    
    if (alpha == 0.0)
        discard;
    
    targets.Color = input.Color;
    targets.Color.a *= alpha;
    
    targets.ID = input.ID;
    
    return targets;
}