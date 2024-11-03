#pragma stage : Vertex

struct Camera
{
    matrix ViewProjection;
};

[[vk::binding(0, 1)]] ConstantBuffer<Camera> u_Camera;

struct VertexOutput
{
    [[vk::location(0)]] float2 LocalPosition : LocalPosition;
    [[vk::location(1)]] float4 Color : Color;
    [[vk::location(2)]] float Thickness : Thickness;
    [[vk::location(3)]] float Fade : Fade;
    [[vk::location(4)]] int ID : ID;
    float4 WorldPosition : SV_POSITION;
};

VertexOutput main(
    [[vk::location(0)]] float3 WorldPosition : WorldPosition,
    [[vk::location(1)]] float2 LocalPosition : LocalPosition,
    [[vk::location(2)]] float4 Color : Color,
    [[vk::location(3)]] float Thickness : Thickness,
    [[vk::location(4)]] float Fade : Fade,
    [[vk::location(5)]] int ID : ID
)
{
    VertexOutput output;
    output.WorldPosition = mul(u_Camera.ViewProjection, float4(WorldPosition, 1.0f));
    output.LocalPosition = LocalPosition;
    output.Color = Color;
    output.Thickness = Thickness;
    output.Fade = Fade;
    output.ID = ID;

    return output;
}

#pragma stage : Pixel

struct PixelInput
{
    [[vk::location(0)]] float2 LocalPosition : LocalPosition;
    [[vk::location(1)]] float4 Color : Color;
    [[vk::location(2)]] float Thickness : Thickness;
    [[vk::location(3)]] float Fade : Fade;
    [[vk::location(4)]] int ID : ID;
};

struct PixelOutput
{
    float4 Color : SV_TARGET0;
    int ID : SV_Target1;
};

PixelOutput main(PixelInput input)
{
    PixelOutput output;
    
    float dist = 1.0 - length(input.LocalPosition);
    //if (dist > 1.0f || dist < 1.0f - input.Thickness)
    //    discard;
    
    float alpha = smoothstep(0.0, input.Fade, dist);
    alpha *= smoothstep(input.Thickness + input.Fade, input.Thickness, dist);
    
    if (alpha == 0.0)
        discard;
    
    output.Color = input.Color;
    output.Color.a *= alpha;
    
    output.ID = input.ID;
    
    return output;
}