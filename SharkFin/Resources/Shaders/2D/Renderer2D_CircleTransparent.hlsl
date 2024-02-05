#pragma stage : Vertex

// begin_metadata
// set u_SceneData PerScene
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

struct PSIN
{
    float2 LocalPosition : LocalPosition;
    float4 Color : Color;
    float Thickness : Thickness;
    float Fade : Fade;
    int ID : ID;
    float4 Position : SV_Position;
};

struct PSOUT
{
    float4 Accumulation : SV_TARGET0;
    float Reveal : SV_Target1;
    int ID : SV_Target2;
};

PSOUT main(PSIN psin)
{
    PSOUT psout;
    
    float dist = 1.0 - length(psin.LocalPosition);
    //if (dist > 1.0f || dist < 1.0f - input.Thickness)
    //    discard;
    
    float alpha = smoothstep(0.0, psin.Fade, dist);
    alpha *= smoothstep(psin.Thickness + psin.Fade, psin.Thickness, dist);
    
    if (alpha == 0.0)
        discard;
    
    float4 color = psin.Color;
    color.a *= alpha;
    
    float depth = psin.Position.z;
    float weight = clamp(pow(min(1.0f, color.a * 10.0f) + 0.01f, 3.0f) * 1e8 * pow(1.0f - depth * 0.9f, 3.0f), 1e-2, 3e3);
    weight = 1.0f;
    
    psout.Accumulation = float4(color.rgb * color.a, color.a) * weight;
    psout.Reveal = color.a;
    
    psout.ID = psin.ID;
    
    return psout;
}