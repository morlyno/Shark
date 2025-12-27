#pragma stage : vertex

struct Constant
{
    float2 Translation;
    float2 Scale;
};

[[vk::push_constant]]
ConstantBuffer<Constant> u_Constants : register(b0);

struct VertexInput
{
    float2 pos : POSITION;
    float2 uv : TEXCOORD;
    float4 col : COLOR;
};

struct VertexOutput
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv : TEXCOORD;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    output.pos = float4(input.pos * u_Constants.Scale + u_Constants.Translation, 0, 1);
    output.pos.y = -output.pos.y;
    //output.pos = mul(u_Constants.ProjectionMatrix, float4(input.pos, 0.0f, 1.0f));
    output.col = input.col;
    output.uv = input.uv;
    return output;
}

#pragma stage : pixel

struct PixelInput
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv : TEXCOORD;
};

Texture2D u_Texture : register(t0);
sampler u_Sampler : register(s0);

float4 main(PixelInput input) : SV_Target
{
    return input.col * u_Texture.Sample(u_Sampler, input.uv);
}
