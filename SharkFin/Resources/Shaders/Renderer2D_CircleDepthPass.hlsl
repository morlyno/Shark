#pragma stage : Vertex

cbuffer SceneData : register(b0)
{
    matrix ViewProjection;
}

struct Output
{
    float2 LocalPosition : LocalPosition;
    float Thickness : Thickness;
    float Fade : Fade;
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
    output.Thickness = Thickness;
    output.Fade = Fade;
    return output;
}

#pragma stage : Pixel

struct Input
{
    float2 LocalPosition : LocalPosition;
    float Thickness : Thickness;
    float Fade : Fade;
};

void main(Input input)
{
    float dist = 1.0 - length(input.LocalPosition);
    //if (dist > 1.0f || dist < 1.0f - input.Thickness)
    //    discard;
    
    float alpha = smoothstep(0.0, input.Fade, dist);
    alpha *= smoothstep(input.Thickness + input.Fade, input.Thickness, dist);
    
    if (alpha == 0.0)
        discard;
}