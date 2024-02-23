#pragma stage : Vertex

struct Camera
{
    matrix ViewProjection;
};

[[vk::binding(0, 1)]] ConstantBuffer<Camera> u_Camera;

struct VertexInput
{
    [[vk::location(0)]] float3 Position : Position;
    [[vk::location(1)]] float4 Color : Color;
};

struct VertexOutput
{
    [[vk::location(0)]] float4 Color : Color;
    float4 Position : SV_POSITION;
};

VertexOutput main(VertexInput Input)
{
    VertexOutput Output;
    Output.Position = mul(u_Camera.ViewProjection, float4(Input.Position, 1.0f));
    Output.Color = Input.Color;
    return Output;
}

#pragma stage : Pixel

struct PixelInput
{
    [[vk::location(0)]] float4 Color : Color;
};

struct PixelOutput
{
    float4 Color : SV_Target0;
};

PixelOutput main(PixelInput Input)
{
    PixelOutput Output;
    Output.Color = Input.Color;
    return Output;
}