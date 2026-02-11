
struct VertexInput
{
    float3 Position : Position;
    float4 Color : Color;
};

struct VertexToPixel
{
    float4 Color : Color;
};

struct PixelOutput
{
    float4 Color : SV_Target0;
};

#pragma stage : Vertex

struct Camera
{
    matrix ViewProjection;
};

ConstantBuffer<Camera> u_Camera : register(b0, space1);

void main(in VertexInput input, out VertexToPixel output, out float4 outPosition : SV_Position)
{
    outPosition = mul(u_Camera.ViewProjection, float4(input.Position, 1.0f));
    output.Color = input.Color;
}

#pragma stage : Pixel

PixelOutput main(VertexToPixel input)
{
    PixelOutput output;
    output.Color = input.Color;
    return output;
}
